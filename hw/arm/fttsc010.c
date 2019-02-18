/*
 * Faraday FTTSC010 emulator.
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/devices.h"
#include "ui/console.h"
#include "qemu/timer.h"
#include "sysemu/sysemu.h"

#include "hw/fttsc010.h"

#define X_AXIS_DMAX     3470
#define X_AXIS_MIN      290
#define Y_AXIS_DMAX     3450
#define Y_AXIS_MIN      200

#define ADS_XPOS(x, y)  \
    (X_AXIS_MIN + ((X_AXIS_DMAX * (x)) >> 15))
#define ADS_YPOS(x, y)  \
    (Y_AXIS_MIN + ((Y_AXIS_DMAX * (y)) >> 15))
#define ADS_Z1POS(x, y) \
    (8)
#define ADS_Z2POS(x, y) \
    ((1600 + ADS_XPOS(x, y)) * ADS_Z1POS(x, y) / ADS_XPOS(x, y))

#define TYPE_FTTSC010   "fttsc010"

#define CFG_REGSIZE     (0x3c / 4)

typedef struct Fttsc010State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    qemu_irq irq;

    uint64_t interval;
    QEMUTimer *qtimer;

    int x, y;
    int z1, z2;
    uint32_t freq;

    /* HW registers */
    uint32_t regs[CFG_REGSIZE];
} Fttsc010State;

#define FTTSC010(obj) \
    OBJECT_CHECK(Fttsc010State, obj, TYPE_FTTSC010)

#define TSC_REG32(s, off) \
    ((s)->regs[(off) / 4])

static void fttsc010_update_irq(Fttsc010State *s)
{
    qemu_set_irq(s->irq, !!(TSC_REG32(s, REG_IMR) & TSC_REG32(s, REG_ISR)));
}

static uint64_t
fttsc010_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    uint32_t ret = 0;
    Fttsc010State *s = FTTSC010(opaque);

    switch (addr) {
    case REG_CR ... REG_DCR:
        ret = s->regs[addr / 4];
        break;
    case REG_XYR:
        ret = deposit32(ret,  0, 12, s->x);
        ret = deposit32(ret, 16, 12, s->y);
        break;
    case REG_ZR:
        ret = deposit32(ret,  0, 12, s->z1);
        ret = deposit32(ret, 16, 12, s->z2);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "fttsc010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
fttsc010_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    uint32_t dly, sdiv, mdiv;
    Fttsc010State *s = FTTSC010(opaque);

    switch (addr) {
    case REG_CR:
        TSC_REG32(s, REG_CR) = (uint32_t)val;
        if (TSC_REG32(s, REG_CR) & (CR_AS | CR_RD1)) {
            /* ADC conversion delay with frame number */
            dly = extract32(TSC_REG32(s, REG_DCR), 0, 16);
            /* ADC sample clock divider */
            sdiv = extract32(TSC_REG32(s, REG_CSR), 8, 8);
            /* ADC main clock divider */
            mdiv = extract32(TSC_REG32(s, REG_CSR), 0, 8);
            /* Calculate sample rate/timer interval */
            s->interval = s->freq / ((mdiv + 1) * (sdiv + 1) * (dly + 1) * 64);
            s->interval = MAX(1ULL, s->interval);
            timer_mod(s->qtimer,
                s->interval + qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL));
        } else {
            timer_del(s->qtimer);
        }
        break;
    case REG_ISR:
        TSC_REG32(s, REG_ISR) &= ~((uint32_t)val);
        fttsc010_update_irq(s);
        break;
    case REG_IMR:
        TSC_REG32(s, REG_IMR) = (uint32_t)val;
        fttsc010_update_irq(s);
        break;
    case REG_CSR ... REG_DCR:
        s->regs[addr / 4] = (uint32_t)val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "fttsc010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = fttsc010_mem_read,
    .write = fttsc010_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void fttsc010_timer_tick(void *opaque)
{
    Fttsc010State *s = FTTSC010(opaque);

    /* update isr for auto-scan */
    if (TSC_REG32(s, REG_CR) & CR_AS) {
        TSC_REG32(s, REG_ISR) |= ISR_AS;
    }

    /* turn it off, when it's under one-shot mode */
    TSC_REG32(s, REG_CR) &= ~CR_RD1;

    /* update irq signal */
    fttsc010_update_irq(s);

    /* reschedule for next auto-scan */
    if (TSC_REG32(s, REG_CR) & CR_AS) {
        timer_mod(s->qtimer, s->interval + qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL));
    }
}

static void
fttsc010_touchscreen_event(void *opaque, int x, int y, int z, int bt)
{
    Fttsc010State *s = FTTSC010(opaque);

    if (bt) {
        /* button pressed */
        x = 0x7fff - x;
        s->x  = ADS_XPOS(x, y);
        s->y  = ADS_YPOS(x, y);
        s->z1 = ADS_Z1POS(x, y);
        s->z2 = ADS_Z2POS(x, y);
    } else {
        /* button released */
        s->z1 = 0;
        s->z2 = 0;
    }
}

static void fttsc010_reset(DeviceState *ds)
{
    Fttsc010State *s = FTTSC010(SYS_BUS_DEVICE(ds));

    memset(s->regs, 0, sizeof(s->regs));
    TSC_REG32(s, REG_REVR) = 0x00010000;    /* rev. 1.0.0 */

    /* initialize touch sample interval to 10 ms */
    TSC_REG32(s, REG_CSR) = CSR_SDIV(16) | CSR_MDIV(5);
    TSC_REG32(s, REG_DCR) = DCR_DLY(100);

    s->x  = 0;
    s->y  = 0;
    s->z1 = 0;
    s->z2 = 0;

    qemu_set_irq(s->irq, 0);
}

static void fttsc010_realize(DeviceState *dev, Error **errp)
{
    Fttsc010State *s = FTTSC010(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    s->qtimer   = timer_new_ms(QEMU_CLOCK_VIRTUAL, fttsc010_timer_tick, s);

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTTSC010,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    qemu_add_mouse_event_handler(fttsc010_touchscreen_event, s, 1,
                    "QEMU FTTSC010-driven Touchscreen");
}

static const VMStateDescription vmstate_fttsc010 = {
    .name = TYPE_FTTSC010,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, Fttsc010State, CFG_REGSIZE),
        VMSTATE_END_OF_LIST(),
    }
};

static Property properties_fttsc010[] = {
    DEFINE_PROP_UINT32("freq", Fttsc010State, freq, 66000000),
    DEFINE_PROP_END_OF_LIST(),
};

static void fttsc010_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset   = fttsc010_reset;
    dc->realize = fttsc010_realize;
    dc->vmsd    = &vmstate_fttsc010;
    dc->props   = properties_fttsc010;
//    dc->no_user = 1;
}

static const TypeInfo fttsc010_info = {
    .name          = TYPE_FTTSC010,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Fttsc010State),
    .class_init    = fttsc010_class_init,
};

static void fttsc010_register_types(void)
{
    type_register_static(&fttsc010_info);
}

type_init(fttsc010_register_types)
