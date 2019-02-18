/*
 * Faraday AHB controller
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/devices.h"
#include "sysemu/sysemu.h"

#include "hw/arm/faraday.h"

#define REG_SLAVE(n)    ((n) * 4)   /* Slave config (base & size) */
#define REG_PRIR        0x80        /* Priority register */
#define REG_IDLECR      0x84        /* IDLE count register */
#define REG_CR          0x88        /* Control register */
#define REG_REVR        0x8c        /* Revision register */

#define CR_REMAP        0x01        /* Enable AHB remap for slave 4 & 6 */

#define TYPE_FTAHBC020  "ftahbc020"

typedef struct Ftahbc020State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;

    FaradaySoCState *soc;

    /* HW register cache */
    uint32_t prir;  /* Priority register */
    uint32_t cr;    /* Control register */
} Ftahbc020State;

#define FTAHBC020(obj) \
    OBJECT_CHECK(Ftahbc020State, obj, TYPE_FTAHBC020)

static uint64_t
ftahbc020_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftahbc020State *s = FTAHBC020(opaque);
    FaradaySoCState *soc = s->soc;
    bool remapped = (soc->ram_base != (soc->ahb_slave[6] & 0xfff00000));
    uint64_t ret = 0;

    switch (addr) {
    /* slave address & window configuration */
    case REG_SLAVE(0) ... REG_SLAVE(3):
    /* fall-through - skip slave4 */
    case REG_SLAVE(5):
    /* fall-through - skip slave6 */
    case REG_SLAVE(7) ... REG_SLAVE(31):
        ret = soc->ahb_slave[addr / 4];
        break;
    case REG_SLAVE(4):
        ret = soc->rom_base | (soc->ahb_slave[4] & 0x000f0000);
        break;
    case REG_SLAVE(6):
        ret = soc->ram_base | (soc->ahb_slave[6] & 0x000f0000);
        break;
    /* priority register */
    case REG_PRIR:
        ret = s->prir;
        break;
    /* idle counter register */
    case REG_IDLECR:
        break;
    /* control register */
    case REG_CR:
        if (remapped) {
            s->cr |= CR_REMAP;
        }
        ret = s->cr;
        break;
    /* revision register */
    case REG_REVR:
        ret = 0x00010301;   /* rev. 1.3.1 */
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftahbc020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftahbc020_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftahbc020State *s = FTAHBC020(opaque);
    FaradaySoCState *soc = s->soc;
    bool remapped = (soc->ram_base != (soc->ahb_slave[6] & 0xfff00000));

    switch (addr) {
    case REG_CR:    /* control register */
        s->cr = (uint32_t)val;
        if (remapped && !(s->cr & CR_REMAP)) {
            fprintf(stderr,
                    "ftahbc020: "
                    "AHB remap could only be disabled via system reset!\n");
            abort();
        }
        if (!remapped && (s->cr & CR_REMAP)) {
      //      faraday_soc_ahb_remap(soc, true);
        }
        break;
    case REG_PRIR:
        s->prir = (uint32_t)val;
        break;
    case REG_SLAVE(0) ... REG_SLAVE(31):
    case REG_IDLECR:
    case REG_REVR:
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftahbc020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftahbc020_mem_read,
    .write = ftahbc020_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void ftahbc020_reset(DeviceState *ds)
{
    Ftahbc020State *s = FTAHBC020(SYS_BUS_DEVICE(ds));
    Error *local_errp = NULL;

    s->soc = FARADAY_SOC(object_property_get_link(OBJECT(s),
                                                  "soc",
                                                  &local_errp));
    if (local_errp) {
        fprintf(stderr, "ftahbc020: Unable to get soc link\n");
        abort();
    }

    s->cr = 0;
    s->prir = 0;
    faraday_soc_ahb_remap(s->soc, false);
}

static void ftahbc020_realize(DeviceState *dev, Error **errp)
{
    Ftahbc020State *s = FTAHBC020(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTAHBC020,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription vmstate_ftahbc020 = {
    .name = TYPE_FTAHBC020,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(prir, Ftahbc020State),
        VMSTATE_UINT32(cr, Ftahbc020State),
        VMSTATE_END_OF_LIST(),
    }
};

static void ftahbc020_instance_init(Object *obj)
{
    Ftahbc020State *s = FTAHBC020(obj);

    object_property_add_link(obj,
                             "soc",
                             TYPE_FARADAY_SOC,
                             (Object **) &s->soc,
                             object_property_allow_set_link,
                             OBJ_PROP_LINK_UNREF_ON_RELEASE,
                             &error_abort);
}

static void ftahbc020_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc  = TYPE_FTAHBC020;
    dc->vmsd  = &vmstate_ftahbc020;
    dc->reset = ftahbc020_reset;
    dc->realize = ftahbc020_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftahbc020_info = {
    .name          = TYPE_FTAHBC020,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftahbc020State),
    .instance_init = ftahbc020_instance_init,
    .class_init    = ftahbc020_class_init,
};

static void ftahbc020_register_types(void)
{
    type_register_static(&ftahbc020_info);
}

type_init(ftahbc020_register_types)
