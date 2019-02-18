/*
 * Faraday DDRII controller
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

#define REG_MCR             0x00    /* memory configuration register */
#define REG_MSR             0x04    /* memory status register */
#define REG_REVR            0x50    /* revision register */

#define MSR_INIT_OK         BIT(8)  /* initialization finished */
#define MSR_CMD_MRS         BIT(0)  /* start MRS command (init. seq.) */

#define CFG_REGSIZE         (REG_REVR / 4)

#define TYPE_FTDDRII030     "ftddrii030"

typedef struct Ftddrii030State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;

    FaradaySoCState *soc;

    /* HW register cache */
    uint32_t regs[CFG_REGSIZE];
} Ftddrii030State;

#define FTDDRII030(obj) \
    OBJECT_CHECK(Ftddrii030State, obj, TYPE_FTDDRII030)

#define DDR_REG32(s, off) \
    ((s)->regs[(off) / 4])

static uint64_t
ftddrii030_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftddrii030State *s = FTDDRII030(opaque);
    FaradaySoCState *soc = s->soc;
    uint64_t ret = 0;

    if (soc->ram_visible) {
        DDR_REG32(s, REG_MSR) |= MSR_INIT_OK;
    } else {
        DDR_REG32(s, REG_MSR) &= ~MSR_INIT_OK;
    }

    switch (addr) {
    case REG_MCR ... REG_REVR - 4:
        ret = DDR_REG32(s, addr);
        break;
    case REG_REVR:
        ret = 0x100;    /* rev. = 0.1.0 */
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftddrii030: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftddrii030_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftddrii030State *s = FTDDRII030(opaque);
    FaradaySoCState *soc = s->soc;

    switch (addr) {
    case REG_MCR:
        DDR_REG32(s, addr) = (uint32_t)val & 0xffff;
        break;
    case REG_MSR:
        if (!soc->ram_visible && (val & MSR_CMD_MRS)) {
            val &= ~MSR_CMD_MRS;
            faraday_soc_ram_setup(soc, true);
        }
        DDR_REG32(s, addr) = (uint32_t)val;
        break;
    /* SDRAM Timing, ECC ...etc. */
    case REG_MSR + 4 ... REG_REVR - 4:
        DDR_REG32(s, addr) = (uint32_t)val;
        break;
    case REG_REVR:
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftddrii030: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftddrii030_mem_read,
    .write = ftddrii030_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void ftddrii030_reset(DeviceState *ds)
{
    Ftddrii030State *s = FTDDRII030(SYS_BUS_DEVICE(ds));
    Error *local_errp = NULL;

    s->soc = FARADAY_SOC(object_property_get_link(OBJECT(s),
                                                  "soc",
                                                  &local_errp));
    if (local_errp) {
        fprintf(stderr, "ftahbc020: Unable to get soc link\n");
        abort();
    }

    faraday_soc_ram_setup(s->soc, false);
    memset(s->regs, 0, sizeof(s->regs));
}

static void ftddrii030_realize(DeviceState *dev, Error **errp)
{
    Ftddrii030State *s = FTDDRII030(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->iomem,
	 		  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTDDRII030,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription vmstate_ftddrii030 = {
    .name = TYPE_FTDDRII030,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, Ftddrii030State, CFG_REGSIZE),
        VMSTATE_END_OF_LIST(),
    }
};

static void ftddrii030_instance_init(Object *obj)
{
    Ftddrii030State *s = FTDDRII030(obj);

    object_property_add_link(obj,
                             "soc",
                             TYPE_FARADAY_SOC,
                             (Object **) &s->soc,
                             object_property_allow_set_link,
                             OBJ_PROP_LINK_UNREF_ON_RELEASE,
                             &error_abort);
}

static void ftddrii030_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc  = TYPE_FTDDRII030;
    dc->vmsd  = &vmstate_ftddrii030;
    dc->reset = ftddrii030_reset;
    dc->realize = ftddrii030_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftddrii030_info = {
    .name          = TYPE_FTDDRII030,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftddrii030State),
    .instance_init = ftddrii030_instance_init,
    .class_init    = ftddrii030_class_init,
};

static void ftddrii030_register_types(void)
{
    type_register_static(&ftddrii030_info);
}

type_init(ftddrii030_register_types)
