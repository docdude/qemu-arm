/*
 * Faraday FTPWMTMR010 Timer.
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

#include "hw/ftpwmtmr010.h"

#define TYPE_FTPWMTMR010        "ftpwmtmr010"
#define TYPE_FTPWMTMR010_TIMER  "ftpwmtmr010_timer"

typedef struct Ftpwmtmr010State Ftpwmtmr010State;

typedef struct Ftpwmtmr010Timer {
    uint32_t ctrl;
    uint32_t cntb;
    int id;
    uint64_t timeout;
    uint64_t countdown;
    qemu_irq irq;
    QEMUTimer *qtimer;
    Ftpwmtmr010State *chip;
} Ftpwmtmr010Timer;

struct Ftpwmtmr010State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    Ftpwmtmr010Timer timer[8];
    uint32_t freq;        /* desired source clock */
    uint64_t step;        /* get_ticks_per_sec() / freq */
    uint32_t stat;
};

#define FTPWMTMR010(obj) \
    OBJECT_CHECK(Ftpwmtmr010State, obj, TYPE_FTPWMTMR010)

static uint64_t
ftpwmtmr010_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftpwmtmr010State *s = FTPWMTMR010(opaque);
    Ftpwmtmr010Timer *t;
    uint64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    uint64_t ret = 0;

    switch (addr) {
    case REG_SR:
        ret = s->stat;
        break;
    case REG_REVR:
        ret = 0x00000000;   /* Rev. 0.0.0 (no rev. id) */
        break;
    case REG_TIMER_BASE(1) ... REG_TIMER_BASE(8) + 0x0C:
        t = s->timer + (addr >> 4) - 1;
        switch (addr & 0x0f) {
        case REG_TIMER_CTRL:
            return t->ctrl;
        case REG_TIMER_CNTB:
            return t->cntb;
        case REG_TIMER_CNTO:
            if ((t->ctrl & TIMER_CTRL_START) && (t->timeout > now)) {
                ret = (t->timeout - now) / s->step;
            }
            break;
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftpwmtmr010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftpwmtmr010_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftpwmtmr010State *s = FTPWMTMR010(opaque);
    Ftpwmtmr010Timer *t;
    int i;

    switch (addr) {
    case REG_SR:
        s->stat &= ~((uint32_t)val);
        for (i = 0; i < 8; ++i) {
            if (val & BIT(i)) {
                qemu_irq_lower(s->timer[i].irq);
            }
        }
        break;
    case REG_TIMER_BASE(1) ... REG_TIMER_BASE(8) + 0x0C:
        t = s->timer + (addr >> 4) - 1;
        switch (addr & 0x0f) {
        case REG_TIMER_CTRL:
            t->ctrl = (uint32_t)val;
            if (t->ctrl & TIMER_CTRL_UPDATE) {
                t->countdown = (uint64_t)t->cntb * s->step;
            }
            if (t->ctrl & TIMER_CTRL_START) {
                t->timeout = t->countdown + qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
                timer_mod(t->qtimer, t->timeout);
            }
            break;
        case REG_TIMER_CNTB:
            t->cntb = (uint32_t)val;
            break;
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftpwmtmr010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftpwmtmr010_mem_read,
    .write = ftpwmtmr010_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void ftpwmtmr010_timer_tick(void *opaque)
{
    Ftpwmtmr010Timer *t = opaque;
    Ftpwmtmr010State *s = t->chip;

    /* if the timer has been enabled/started */
    if (!(t->ctrl & TIMER_CTRL_START)) {
        return;
    }

    /* if the interrupt enabled */
    if (t->ctrl & TIMER_CTRL_INTR) {
        s->stat |= BIT(t->id);
        if (t->ctrl & TIMER_CTRL_INTR_EDGE) {
            qemu_irq_pulse(t->irq);
        } else {
            qemu_irq_raise(t->irq);
        }
    }

    /* if auto-reload is enabled */
    if (t->ctrl & TIMER_CTRL_AUTORELOAD) {
        t->timeout = t->countdown + qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        timer_mod(t->qtimer, t->timeout);
    } else {
        t->ctrl &= ~TIMER_CTRL_START;
    }
}

static void ftpwmtmr010_reset(DeviceState *ds)
{
    Ftpwmtmr010State *s = FTPWMTMR010(SYS_BUS_DEVICE(ds));
    int i;

    s->stat = 0;

    for (i = 0; i < 8; ++i) {
        s->timer[i].ctrl = 0;
        s->timer[i].cntb = 0;
        s->timer[i].timeout = 0;
        qemu_irq_lower(s->timer[i].irq);
        timer_del(s->timer[i].qtimer);
    }
}

static void ftpwmtmr010_realize(DeviceState *dev, Error **errp)
{
    Ftpwmtmr010State *s = FTPWMTMR010(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    int i;

    s->step = (uint64_t)get_ticks_per_sec() / (uint64_t)s->freq;

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTPWMTMR010,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    for (i = 0; i < 8; ++i) {
        s->timer[i].id = i;
        s->timer[i].chip = s;
        s->timer[i].qtimer = timer_new_ns(QEMU_CLOCK_VIRTUAL,
                                               ftpwmtmr010_timer_tick,
                                               &s->timer[i]);
        sysbus_init_irq(sbd, &s->timer[i].irq);
    }
}

static const VMStateDescription vmstate_ftpwmtmr010_timer = {
    .name = TYPE_FTPWMTMR010_TIMER,
    .version_id = 2,
    .minimum_version_id = 2,
    .minimum_version_id_old = 2,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(ctrl, Ftpwmtmr010Timer),
        VMSTATE_UINT32(cntb, Ftpwmtmr010Timer),
        VMSTATE_END_OF_LIST(),
    },
};

static const VMStateDescription vmstate_ftpwmtmr010 = {
    .name = TYPE_FTPWMTMR010,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(stat, Ftpwmtmr010State),
        VMSTATE_UINT32(freq, Ftpwmtmr010State),
        VMSTATE_UINT64(step, Ftpwmtmr010State),
        VMSTATE_STRUCT_ARRAY(timer, Ftpwmtmr010State, 8, 1,
                        vmstate_ftpwmtmr010_timer, Ftpwmtmr010Timer),
        VMSTATE_END_OF_LIST(),
    }
};

static Property ftpwmtmr010_properties[] = {
    DEFINE_PROP_UINT32("freq", Ftpwmtmr010State, freq, 66000000),
    DEFINE_PROP_END_OF_LIST(),
};

static void ftpwmtmr010_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftpwmtmr010;
    dc->props   = ftpwmtmr010_properties;
    dc->reset   = ftpwmtmr010_reset;
    dc->realize = ftpwmtmr010_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftpwmtmr010_info = {
    .name          = TYPE_FTPWMTMR010,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftpwmtmr010State),
    .class_init    = ftpwmtmr010_class_init,
};

static void ftpwmtmr010_register_types(void)
{
    type_register_static(&ftpwmtmr010_info);
}

type_init(ftpwmtmr010_register_types)
