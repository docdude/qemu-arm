/*
 * Faraday FTTMR010 Timer.
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "sysemu/sysemu.h"

#include "hw/fttmr010.h"

#define TYPE_FTTMR010       "fttmr010"
#define TYPE_FTTMR010_TIMER "fttmr010_timer"

typedef struct Fttmr010State Fttmr010State;

typedef struct Fttmr010Timer {
    int id;
    int up;
    Fttmr010State *chip;
    qemu_irq irq;
    QEMUTimer *qtimer;
    uint64_t start;
    uint32_t intr_match1:1;
    uint32_t intr_match2:1;

    /* HW register caches */
    uint64_t counter;
    uint64_t reload;
    uint32_t match1;
    uint32_t match2;

} Fttmr010Timer;

struct Fttmr010State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    qemu_irq irq;
    Fttmr010Timer timer[3];
    uint32_t freq;        /* desired source clock */
    uint64_t step;        /* get_ticks_per_sec() / freq */

    /* HW register caches */
    uint32_t cr;
    uint32_t isr;
    uint32_t imr;
};

#define FTTMR010(obj) \
    OBJECT_CHECK(Fttmr010State, obj, TYPE_FTTMR010)

static void fttmr010_timer_restart(Fttmr010Timer *t)
{
    Fttmr010State *s = t->chip;
    uint64_t interval;
    int pending = 0;

    t->intr_match1 = 0;
    t->intr_match2 = 0;

    /* check match1 */
    if (t->up && t->match1 <= t->counter) {
        t->intr_match1 = 1;
    }
    if (!t->up && t->match1 >= t->counter) {
        t->intr_match1 = 1;
    }
    if (t->match1 == t->counter) {
        s->isr |= ISR_MATCH1(t->id);
        ++pending;
    }

    /* check match2 */
    if (t->up && t->match2 <= t->counter) {
        t->intr_match2 = 1;
    }
    if (!t->up && t->match2 >= t->counter) {
        t->intr_match2 = 1;
    }
    if (t->match2 == t->counter) {
        s->isr |= ISR_MATCH2(t->id);
        ++pending;
    }

    /* determine delay interval */
    if (t->up) {
        if ((t->match1 > t->counter) && (t->match2 > t->counter)) {
            interval = MIN(t->match1, t->match2) - t->counter;
        } else if (t->match1 > t->counter) {
            interval = t->match1 - t->counter;
        } else if (t->match2 > t->reload) {
            interval = t->match2 - t->counter;
        } else {
            interval = 0xffffffffULL - t->counter;
        }
    } else {
        if ((t->match1 < t->counter) && (t->match2 < t->counter)) {
            interval = t->counter - MAX(t->match1, t->match2);
        } else if (t->match1 < t->reload) {
            interval = t->counter - t->match1;
        } else if (t->match2 < t->reload) {
            interval = t->counter - t->match2;
        } else {
            interval = t->counter;
        }
    }

    if (pending) {
        qemu_irq_pulse(s->irq);
        qemu_irq_pulse(t->irq);
    }
    t->start = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    timer_mod(t->qtimer, t->start + interval * s->step);
}

static uint64_t fttmr010_update_counter(Fttmr010Timer *t)
{
    Fttmr010State *s = t->chip;
    uint64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    uint64_t elapsed;
    int pending = 0;

    if (s->cr & CR_TMR_EN(t->id)) {
        /* get elapsed time */
        elapsed = (now - t->start) / s->step;

        /* convert to count-up/count-down value */
        if (t->up) {
            t->counter = t->counter + elapsed;
        } else {
            if (t->counter > elapsed) {
                t->counter -= elapsed;
            } else {
                t->counter = 0;
            }
        }
        t->start = now;

        /* check match1 */
        if (!t->intr_match1) {
            if (t->up && t->match1 <= t->counter) {
                t->intr_match1 = 1;
                s->isr |= ISR_MATCH1(t->id);
                ++pending;
            }
            if (!t->up && t->match1 >= t->counter) {
                t->intr_match1 = 1;
                s->isr |= ISR_MATCH1(t->id);
                ++pending;
            }
        }

        /* check match2 */
        if (!t->intr_match2) {
            if (t->up && t->match2 <= t->counter) {
                t->intr_match2 = 1;
                s->isr |= ISR_MATCH2(t->id);
                ++pending;
            }
            if (!t->up && t->match2 >= t->counter) {
                t->intr_match2 = 1;
                s->isr |= ISR_MATCH2(t->id);
                ++pending;
            }
        }

        /* check overflow/underflow */
        if (t->up && t->counter >= 0xffffffffULL) {
            if (s->cr & CR_TMR_OFEN(t->id)) {
                s->isr |= ISR_OF(t->id);
                ++pending;
            }
            t->counter = t->reload;
            fttmr010_timer_restart(t);
        }
        if (!t->up && t->counter == 0) {
            if (s->cr & CR_TMR_OFEN(t->id)) {
                s->isr |= ISR_OF(t->id);
                ++pending;
            }
            t->counter = t->reload;
            fttmr010_timer_restart(t);
        }
    }

    if (pending) {
        qemu_irq_pulse(s->irq);
        qemu_irq_pulse(t->irq);
    }

    return t->counter;
}

static uint64_t
fttmr010_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Fttmr010State *s = FTTMR010(opaque);
    Fttmr010Timer *t;
    uint64_t ret = 0;

    switch (addr) {
    case REG_TMR_BASE(0) ... REG_TMR_BASE(2) + 0x0C:
        t = s->timer + REG_TMR_ID(addr);
        switch (addr & 0x0f) {
        case REG_TMR_COUNTER:
            return fttmr010_update_counter(t);
        case REG_TMR_RELOAD:
            return t->reload;
        case REG_TMR_MATCH1:
            return t->match1;
        case REG_TMR_MATCH2:
            return t->match2;
        }
        break;
    case REG_CR:
        return s->cr;
    case REG_ISR:
        return s->isr;
    case REG_IMR:
        return s->imr;
    case REG_REVR:
        return 0x00010801;  /* rev. 1.8.1 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "fttmr010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
fttmr010_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Fttmr010State *s = FTTMR010(opaque);
    Fttmr010Timer *t;
    int i;

    switch (addr) {
    case REG_TMR_BASE(0) ... REG_TMR_BASE(2) + 0x0C:
        t = s->timer + REG_TMR_ID(addr);
        switch (addr & 0x0f) {
        case REG_TMR_COUNTER:
            t->counter = (uint32_t)val;
            break;
        case REG_TMR_RELOAD:
            t->reload = (uint32_t)val;
            break;
        case REG_TMR_MATCH1:
            t->match1 = (uint32_t)val;
            break;
        case REG_TMR_MATCH2:
            t->match2 = (uint32_t)val;
            break;
        }
        break;
    case REG_CR:
        s->cr = (uint32_t)val;
        for (i = 0; i < 3; ++i) {
            t = s->timer + i;
            if (s->cr & CR_TMR_COUNTUP(t->id)) {
                t->up = 1;
            } else {
                t->up = 0;
            }
            if (s->cr & CR_TMR_EN(t->id)) {
                fttmr010_timer_restart(t);
            } else {
                timer_del(t->qtimer);
            }
        }
        break;
    case REG_ISR:
        s->isr &= ~((uint32_t)val);
        break;
    case REG_IMR:
        s->imr = (uint32_t)val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "fttmr010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = fttmr010_mem_read,
    .write = fttmr010_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void fttmr010_timer_tick(void *opaque)
{
    Fttmr010Timer *t = opaque;
    Fttmr010State *s = t->chip;
    uint64_t now;

    /* if the timer has been enabled/started */
    if (!(s->cr & CR_TMR_EN(t->id))) {
        return;
    }

    fttmr010_update_counter(t);

    if (t->reload == t->counter) {
        return;
    }

    now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    if (t->up) {
        if (!t->intr_match1 && t->match1 > t->counter) {
            timer_mod(t->qtimer,
                now + (t->match1 - t->counter) * s->step);
        } else if (!t->intr_match2 && t->match2 > t->counter) {
            timer_mod(t->qtimer,
                now + (t->match2 - t->counter) * s->step);
        } else {
            timer_mod(t->qtimer,
                now + (0xffffffffULL - t->counter) * s->step);
        }
    } else {
        if (!t->intr_match1 && t->match1 < t->counter) {
            timer_mod(t->qtimer,
                now + (t->counter - t->match1) * s->step);
        } else if (!t->intr_match2 && t->match2 < t->counter) {
            timer_mod(t->qtimer,
                now + (t->counter - t->match2) * s->step);
        } else {
            timer_mod(t->qtimer,
                now + t->counter * s->step);
        }
    }
}

static void fttmr010_reset(DeviceState *ds)
{
    Fttmr010State *s = FTTMR010(SYS_BUS_DEVICE(ds));
    int i;

    s->cr  = 0;
    s->isr = 0;
    s->imr = 0;
    qemu_irq_lower(s->irq);

    for (i = 0; i < 3; ++i) {
        s->timer[i].counter = 0;
        s->timer[i].reload  = 0;
        s->timer[i].match1  = 0;
        s->timer[i].match2  = 0;
        qemu_irq_lower(s->timer[i].irq);
        timer_del(s->timer[i].qtimer);
    }
}

static void fttmr010_realize(DeviceState *dev, Error **errp)
{
    Fttmr010State *s = FTTMR010(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    int i;

    s->step = (uint64_t)get_ticks_per_sec() / (uint64_t)s->freq;

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTTMR010,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
    for (i = 0; i < 3; ++i) {
        s->timer[i].id = i;
        s->timer[i].chip = s;
        s->timer[i].qtimer = timer_new_ns(QEMU_CLOCK_VIRTUAL,
                                        fttmr010_timer_tick, &s->timer[i]);
        sysbus_init_irq(sbd, &s->timer[i].irq);
    }
}

static const VMStateDescription vmstate_fttmr010_timer = {
    .name = TYPE_FTTMR010_TIMER,
    .version_id = 2,
    .minimum_version_id = 2,
    .minimum_version_id_old = 2,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(counter, Fttmr010Timer),
        VMSTATE_UINT64(reload, Fttmr010Timer),
        VMSTATE_UINT32(match1, Fttmr010Timer),
        VMSTATE_UINT32(match2, Fttmr010Timer),
        VMSTATE_END_OF_LIST(),
    },
};

static const VMStateDescription vmstate_fttmr010 = {
    .name = TYPE_FTTMR010,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cr, Fttmr010State),
        VMSTATE_UINT32(isr, Fttmr010State),
        VMSTATE_UINT32(imr, Fttmr010State),
        VMSTATE_STRUCT_ARRAY(timer, Fttmr010State, 3, 1,
                        vmstate_fttmr010_timer, Fttmr010Timer),
        VMSTATE_END_OF_LIST(),
    }
};

static Property fttmr010_properties[] = {
    DEFINE_PROP_UINT32("freq", Fttmr010State, freq, 66000000),
    DEFINE_PROP_END_OF_LIST(),
};

static void fttmr010_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_fttmr010;
    dc->props   = fttmr010_properties;
    dc->reset   = fttmr010_reset;
    dc->realize = fttmr010_realize;
 //   dc->no_user = 1;
}

static const TypeInfo fttmr010_info = {
    .name          = TYPE_FTTMR010,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Fttmr010State),
    .class_init    = fttmr010_class_init,
};

static void fttmr010_register_types(void)
{
    type_register_static(&fttmr010_info);
}

type_init(fttmr010_register_types)
