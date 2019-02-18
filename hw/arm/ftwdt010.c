/*
 * QEMU model of the FTWDT010 WatchDog Timer
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
#include "sysemu/watchdog.h"
#include "sysemu/sysemu.h"
#include "qemu/timer.h"

#include "hw/ftwdt010.h"

#define TYPE_FTWDT010   "ftwdt010"

typedef struct Ftwdt010State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion mmio;

    qemu_irq irq;

    QEMUTimer *qtimer;

    uint64_t timeout;
    uint64_t freq;        /* desired source clock */
    uint64_t step;        /* get_ticks_per_sec() / freq */
    bool running;

    /* HW register cache */
    uint32_t load;
    uint32_t cr;
    uint32_t sr;
} Ftwdt010State;

#define FTWDT010(obj) \
    OBJECT_CHECK(Ftwdt010State, obj, TYPE_FTWDT010)

static uint64_t
ftwdt010_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftwdt010State *s = FTWDT010(opaque);
    uint32_t ret = 0;

    switch (addr) {
    case REG_COUNTER:
        if (s->cr & CR_EN) {
            ret = s->timeout - qemu_clock_get_ms(QEMU_CLOCK_REALTIME);
            ret = MIN(s->load, ret * 1000000ULL / s->step);
        } else {
            ret = s->load;
        }
        break;
    case REG_LOAD:
        return s->load;
    case REG_CR:
        return s->cr;
    case REG_SR:
        return s->sr;
    case REG_REVR:
        return 0x00010601;  /* rev. 1.6.1 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftwdt010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftwdt010_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftwdt010State *s = FTWDT010(opaque);

    switch (addr) {
    case REG_LOAD:
        s->load = (uint32_t)val;
        break;
    case REG_RESTART:
        if ((s->cr & CR_EN) && (val == WDT_MAGIC)) {
            s->timeout = (s->step * (uint64_t)s->load) / 1000000ULL;
            s->timeout = qemu_clock_get_ms(QEMU_CLOCK_REALTIME) + MAX(s->timeout, 1);
            timer_mod(s->qtimer, s->timeout);
        }
        break;
    case REG_CR:
        s->cr = (uint32_t)val;
        if (s->cr & CR_EN) {
            if (s->running) {
                break;
            }
            s->running = true;
            s->timeout = (s->step * (uint64_t)s->load) / 1000000ULL;
            s->timeout = qemu_clock_get_ms(QEMU_CLOCK_REALTIME) + MAX(s->timeout, 1);
            timer_mod(s->qtimer, s->timeout);
        } else {
            s->running = false;
            timer_del(s->qtimer);
        }
        break;
    case REG_SCR:
        s->sr &= ~(uint32_t)val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftwdt010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftwdt010_mem_read,
    .write = ftwdt010_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftwdt010_timer_tick(void *opaque)
{
    Ftwdt010State *s = FTWDT010(opaque);

    s->sr = SR_SRST;

    /* send interrupt signal */
    qemu_set_irq(s->irq, (s->cr & CR_INTR) ? 1 : 0);

    /* send system reset */
    if (s->cr & CR_SRST) {
        watchdog_perform_action();
    }
}

static void ftwdt010_reset(DeviceState *ds)
{
    Ftwdt010State *s = FTWDT010(SYS_BUS_DEVICE(ds));

    s->cr      = 0;
    s->sr      = 0;
    s->load    = 0x3ef1480;
    s->timeout = 0;
}

static void ftwdt010_realize(DeviceState *dev, Error **errp)
{
    Ftwdt010State *s = FTWDT010(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    s->step = (uint64_t)get_ticks_per_sec() / s->freq;
    s->qtimer = timer_new_ms(QEMU_CLOCK_REALTIME, ftwdt010_timer_tick, s);

    memory_region_init_io(&s->mmio,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTWDT010,
                          0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);
}

static const VMStateDescription vmstate_ftwdt010 = {
    .name = TYPE_FTWDT010,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(timeout, Ftwdt010State),
        VMSTATE_UINT64(freq, Ftwdt010State),
        VMSTATE_UINT64(step, Ftwdt010State),
        VMSTATE_UINT32(load, Ftwdt010State),
        VMSTATE_UINT32(cr, Ftwdt010State),
        VMSTATE_UINT32(sr, Ftwdt010State),
        VMSTATE_END_OF_LIST()
    }
};

static Property ftwdt010_properties[] = {
    DEFINE_PROP_UINT64("freq", Ftwdt010State, freq, 66000000ULL),
    DEFINE_PROP_END_OF_LIST(),
};

static void ftwdt010_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftwdt010;
    dc->props   = ftwdt010_properties;
    dc->reset   = ftwdt010_reset;
    dc->realize = ftwdt010_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftwdt010_info = {
    .name           = TYPE_FTWDT010,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(Ftwdt010State),
    .class_init     = ftwdt010_class_init,
};

static void ftwdt010_register_types(void)
{
    type_register_static(&ftwdt010_info);
}

type_init(ftwdt010_register_types)
