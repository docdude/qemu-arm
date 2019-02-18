/*
 * QEMU model of the FTI2C010 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
#include "hw/i2c/i2c.h"
#include "sysemu/sysemu.h"

#include "hw/fti2c010.h"

#define I2C_RD  1
#define I2C_WR  0

#define TYPE_FTI2C010   "fti2c010"

typedef struct Fti2c010State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion mmio;

    qemu_irq irq;
    I2CBus *bus;

    bool     recv;    /* I2C RD = true; I2C WR = false */

    /* HW register cache */
    uint32_t cr;
    uint32_t sr;
    uint32_t cdr;
    uint32_t dr;
    uint32_t tgsr;
} Fti2c010State;

#define FTI2C010(obj) \
    OBJECT_CHECK(Fti2c010State, obj, TYPE_FTI2C010)

static void
fti2c010_update_irq(Fti2c010State *s)
{
    uint32_t sr = extract32(s->sr, 4, 8);
    uint32_t cr = extract32(s->cr, 8, 8);
    qemu_set_irq(s->irq, (sr & cr) ? 1 : 0);
}

static uint64_t
fti2c010_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Fti2c010State *s = FTI2C010(opaque);
    uint32_t ret = 0;

    switch (addr) {
    case REG_CR:
        return s->cr;
    case REG_SR:
        ret = s->sr | (i2c_bus_busy(s->bus) ? SR_BB : 0);
        s->sr &= 0xfffff00f;    /* clear RC status bits */
        fti2c010_update_irq(s);
        break;
    case REG_CDR:
        return s->cdr;
    case REG_DR:
        return s->dr;
    case REG_TGSR:
        return s->tgsr;
    case REG_BMR:
        return 0x00000003;  /* Slave mode: SCL=1, SDA=1 */
    case REG_REVR:
        return 0x00011000;  /* REV. 1.10.0 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "fti2c010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
fti2c010_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Fti2c010State *s = FTI2C010(opaque);

    switch (addr) {
    case REG_CR:
        s->cr = (uint32_t)val;
        if (s->cr & CR_SLAVE_MASK) {
            qemu_log_mask(LOG_UNIMP,
                "fti2c010: slave mask deteced!\n");
            s->cr &= ~CR_SLAVE_MASK;
        }
        if (s->cr & CR_I2CRST) {
            s->dr = 0;
            s->sr = 0;
        } else if ((s->cr & CR_MASTER_EN) && (s->cr & CR_TBEN)) {
            s->sr &= ~SR_ACK;
            if (s->cr & CR_START) {
                s->recv = !!(s->dr & I2C_RD);
                if (!i2c_start_transfer(s->bus,
                                        extract32(s->dr, 1, 7),
                                        s->recv)) {
                    s->sr |= SR_DT | SR_ACK;
                } else {
                    s->sr &= ~SR_DT;
                }
            } else {
                if (s->recv) {
                    s->dr = i2c_recv(s->bus);
                    s->sr |= SR_DR;
                } else {
                    i2c_send(s->bus, (uint8_t)s->dr);
                    s->sr |= SR_DT;
                }
                if (s->cr & CR_NACK) {
                    i2c_nack(s->bus);
                }
                s->sr |= SR_ACK;
                if (s->cr & CR_STOP) {
                    i2c_end_transfer(s->bus);
                }
            }
        }
        s->cr &= ~(CR_TBEN | CR_I2CRST);
        fti2c010_update_irq(s);
        break;
    case REG_CDR:
        s->cdr = (uint32_t)val & 0x3ffff;
        break;
    case REG_DR:
        s->dr  = (uint32_t)val & 0xff;
        break;
    case REG_TGSR:
        s->tgsr = (uint32_t)val & 0x1fff;
        break;
        /* slave mode is useless to QEMU, ignore it. */
    case REG_SAR:
        qemu_log_mask(LOG_UNIMP,
            "fti2c010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "fti2c010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = fti2c010_mem_read,
    .write = fti2c010_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void fti2c010_reset(DeviceState *ds)
{
    Fti2c010State *s = FTI2C010(SYS_BUS_DEVICE(ds));

    s->cr   = 0;
    s->sr   = 0;
    s->cdr  = 0;
    s->tgsr = TGSR_TSR(1) | TGSR_GSR(1);

    qemu_set_irq(s->irq, 0);
}

static void fti2c010_realize(DeviceState *dev, Error **errp)
{
    Fti2c010State *s = FTI2C010(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    s->bus = i2c_init_bus(dev, "i2c");

    memory_region_init_io(&s->mmio, OBJECT(s), &mmio_ops, s, TYPE_FTI2C010, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);
}

static const VMStateDescription vmstate_fti2c010 = {
    .name = TYPE_FTI2C010,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cr, Fti2c010State),
        VMSTATE_UINT32(sr, Fti2c010State),
        VMSTATE_UINT32(cdr, Fti2c010State),
        VMSTATE_UINT32(dr, Fti2c010State),
        VMSTATE_UINT32(tgsr, Fti2c010State),
        VMSTATE_END_OF_LIST()
    }
};

static void fti2c010_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_fti2c010;
    dc->reset   = fti2c010_reset;
    dc->realize = fti2c010_realize;
 //   dc->no_user = 1;
}

static const TypeInfo fti2c010_info = {
    .name           = TYPE_FTI2C010,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(Fti2c010State),
    .class_init     = fti2c010_class_init,
};

static void fti2c010_register_types(void)
{
    type_register_static(&fti2c010_info);
}

type_init(fti2c010_register_types)
