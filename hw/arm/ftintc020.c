/*
 * Faraday FTINTC020 Programmable Interrupt Controller.
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#include "hw/hw.h"
#include "hw/sysbus.h"

#include "qemu/bitops.h"
#include "hw/ftintc020.h"

#define TYPE_FTINTC020  "ftintc020"

#define CFG_REGSIZE     (0x100 / 4)

typedef struct Ftintc020State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;

    qemu_irq irq;
    qemu_irq fiq;

    uint32_t irq_ps[2]; /* IRQ pin state */
    uint32_t fiq_ps[2]; /* FIQ pin state */

    /* HW register caches */
    uint32_t regs[CFG_REGSIZE];
} Ftintc020State;

#define FTINTC020(obj) \
    OBJECT_CHECK(Ftintc020State, obj, TYPE_FTINTC020)

#define PIC_REG32(s, off) \
    ((s)->regs[(off) / 4])

#define IRQ_REG32(s, n, off) \
    ((s)->regs[(REG_IRQ(n) + ((off) & REG_MASK)) / 4])

#define FIQ_REG32(s, n, off) \
    ((s)->regs[(REG_FIQ(n) + ((off) & REG_MASK)) / 4])

static void
ftintc020_update_irq(Ftintc020State *s)
{
    uint32_t mask[2];

    /* FIQ */
    mask[0] = PIC_REG32(s, REG_FIQ32SRC) & PIC_REG32(s, REG_FIQ32ENA);
    mask[1] = PIC_REG32(s, REG_FIQ64SRC) & PIC_REG32(s, REG_FIQ64ENA);
    qemu_set_irq(s->fiq, !!(mask[0] || mask[1]));

    /* IRQ */
    mask[0] = PIC_REG32(s, REG_IRQ32SRC) & PIC_REG32(s, REG_IRQ32ENA);
    mask[1] = PIC_REG32(s, REG_IRQ64SRC) & PIC_REG32(s, REG_IRQ64ENA);
    qemu_set_irq(s->irq, !!(mask[0] || mask[1]));
}

/* Note: Here level means state of the signal on a pin */
static void
ftintc020_set_irq(void *opaque, int irq, int level)
{
    Ftintc020State *s = FTINTC020(opaque);
    uint32_t i = irq / 32;
    uint32_t mask = BIT(irq & 0x1f);

    switch (irq) {
    case 0  ... 63:
        /* IRQ */
        if (IRQ_REG32(s, irq, REG_MDR) & mask) {
            /* Edge Triggered */
            if (IRQ_REG32(s, irq, REG_LVR) & mask) {
                /* Falling Active */
                if ((s->irq_ps[i] & mask) && !level) {
                    IRQ_REG32(s, irq, REG_SRC) |= mask;
                }
            } else {
                /* Rising Active */
                if (!(s->irq_ps[i] & mask) && level) {
                    IRQ_REG32(s, irq, REG_SRC) |= mask;
                }
            }
        } else {
            /* Level Triggered */
            if (IRQ_REG32(s, irq, REG_LVR) & mask) {
                /* Low Active */
                if (level) {
                    IRQ_REG32(s, irq, REG_SRC) &= ~mask;
                } else {
                    IRQ_REG32(s, irq, REG_SRC) |= mask;
                }
            } else {
                /* High Active */
                if (level) {
                    IRQ_REG32(s, irq, REG_SRC) |= mask;
                } else {
                    IRQ_REG32(s, irq, REG_SRC) &= ~mask;
                }
            }
        }

        /* FIQ */
        if (FIQ_REG32(s, irq, REG_MDR) & mask) {
            /* Edge Triggered */
            if (FIQ_REG32(s, irq, REG_LVR) & mask) {
                /* Falling Active */
                if ((s->fiq_ps[i] & mask) && !level) {
                    FIQ_REG32(s, irq, REG_SRC) |= mask;
                }
            } else {
                /* Rising Active */
                if (!(s->fiq_ps[i] & mask) && level) {
                    FIQ_REG32(s, irq, REG_SRC) |= mask;
                }
            }
        } else {
            /* Level Triggered */
            if (FIQ_REG32(s, irq, REG_LVR) & mask) {
                /* Low Active */
                if (level) {
                    FIQ_REG32(s, irq, REG_SRC) &= ~mask;
                } else {
                    FIQ_REG32(s, irq, REG_SRC) |= mask;
                }
            } else {
                /* High Active */
                if (level) {
                    FIQ_REG32(s, irq, REG_SRC) |= mask;
                } else {
                    FIQ_REG32(s, irq, REG_SRC) &= ~mask;
                }
            }
        }
        /* update IRQ/FIQ pin states */
        if (level) {
            s->irq_ps[i] |= mask;
            s->fiq_ps[i] |= mask;
        } else {
            s->irq_ps[i] &= ~mask;
            s->fiq_ps[i] &= ~mask;
        }
        break;

    default:
        fprintf(stderr, "ftintc020: undefined irq=%d\n", irq);
        abort();
    }

    ftintc020_update_irq(s);
}

static uint64_t
ftintc020_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftintc020State *s = FTINTC020(opaque);
    uint32_t ret = 0;

    if (addr > REG_FIQ64SR) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftintc020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        return ret;
    }

    switch (addr) {
    case REG_IRQ32SR:
        ret = PIC_REG32(s, REG_IRQ32SRC) & PIC_REG32(s, REG_IRQ32ENA);
        break;
    case REG_IRQ64SR:
        ret = PIC_REG32(s, REG_IRQ64SRC) & PIC_REG32(s, REG_IRQ64ENA);
        break;
    case REG_FIQ32SR:
        ret = PIC_REG32(s, REG_FIQ32SRC) & PIC_REG32(s, REG_FIQ32ENA);
        break;
    case REG_FIQ64SR:
        ret = PIC_REG32(s, REG_FIQ64SRC) & PIC_REG32(s, REG_FIQ64ENA);
        break;
    default:
        ret = s->regs[addr / 4];
        break;
    }

    return ret;
}

static void
ftintc020_mem_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    Ftintc020State *s = FTINTC020(opaque);

    if (addr > REG_FIQ64SR) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftintc020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        return;
    }

    switch (addr) {
    case REG_IRQ32SCR:
        /* clear edge triggered interrupts only */
        value = ~(value & PIC_REG32(s, REG_IRQ32MDR));
        PIC_REG32(s, REG_IRQ32SRC) &= (uint32_t)value;
        break;
    case REG_IRQ64SCR:
        /* clear edge triggered interrupts only */
        value = ~(value & PIC_REG32(s, REG_IRQ64MDR));
        PIC_REG32(s, REG_IRQ64SRC) &= (uint32_t)value;
        break;
    case REG_FIQ32SCR:
        /* clear edge triggered interrupts only */
        value = ~(value & PIC_REG32(s, REG_FIQ32MDR));
        PIC_REG32(s, REG_FIQ32SRC) &= (uint32_t)value;
        break;
    case REG_FIQ64SCR:
        /* clear edge triggered interrupts only */
        value = ~(value & PIC_REG32(s, REG_FIQ64MDR));
        PIC_REG32(s, REG_FIQ64SRC) &= (uint32_t)value;
        break;
    default:
        s->regs[addr / 4] = (uint32_t)value;
        break;
    }

    ftintc020_update_irq(s);
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftintc020_mem_read,
    .write = ftintc020_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void ftintc020_reset(DeviceState *ds)
{
    Ftintc020State *s = FTINTC020(SYS_BUS_DEVICE(ds));

    s->irq_ps[0] = 0;
    s->irq_ps[1] = 0;
    s->fiq_ps[0] = 0;
    s->fiq_ps[1] = 0;
    memset(s->regs, 0, sizeof(s->regs));

    ftintc020_update_irq(s);
}

static VMStateDescription vmstate_ftintc020 = {
    .name = TYPE_FTINTC020,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, Ftintc020State, CFG_REGSIZE),
        VMSTATE_UINT32_ARRAY(irq_ps, Ftintc020State, 2),
        VMSTATE_UINT32_ARRAY(fiq_ps, Ftintc020State, 2),
        VMSTATE_END_OF_LIST(),
    },
};

static void ftintc020_realize(DeviceState *dev, Error **errp)
{
    Ftintc020State *s = FTINTC020(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    /* Enable IC memory-mapped registers access.  */
    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTINTC020,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);

    qdev_init_gpio_in(dev, ftintc020_set_irq, 64);
    sysbus_init_irq(sbd, &s->irq);
    sysbus_init_irq(sbd, &s->fiq);
}

static void ftintc020_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftintc020;
    dc->reset   = ftintc020_reset;
    dc->realize = ftintc020_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftintc020_info = {
    .name          = TYPE_FTINTC020,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftintc020State),
    .class_init    = ftintc020_class_init,
};

static void ftintc020_register_types(void)
{
    type_register_static(&ftintc020_info);
}

type_init(ftintc020_register_types)
