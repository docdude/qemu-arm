/*
 * Faraday A369 SCU
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * The system control unit (SCU) is responsible for
 * power, clock and pinmux management. Since most of
 * the features are useless to QEMU, only partial clock
 * and pinmux management are implemented as a set of R/W values.
 *
 * This code is licensed under GNU GPL v2+
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/devices.h"
#include "sysemu/sysemu.h"

#define REG_CHIPID      0x000   /* SoC chip id */
#define REG_REVISON     0x004   /* SCU revision id */
#define REG_HWCFG       0x008   /* HW configuration strap */
#define REG_CPUMFCR     0x00C   /* CPUM (master) freq. control */
#define REG_SCUCR       0x010   /* SCU control register */
#define REG_SCUSR       0x014   /* SCU status register */
#define REG_OSCCR       0x01C   /* OSC control register */
#define REG_PLL1CR      0x020   /* PLL1 control register */
#define REG_DLLCR       0x024   /* DLL control register */
#define REG_SPR(n)      (0x100 + ((n) << 2)) /* Scratchpad register 0 - 15 */
#define REG_GPINMUX     0x200   /* General PINMUX */
#define REG_EXTHWCFG    0x204   /* Extended HW configuration strap */
#define REG_CLKCFG0     0x228   /* Clock configuration 0 */
#define REG_CLKCFG1     0x22C   /* Clock configuration 1 */
#define REG_SCER        0x230   /* Special clock enable register */
#define REG_MFPINMUX0   0x238   /* Multi-function pinmux 0 */
#define REG_MFPINMUX1   0x23C   /* Multi-function pinmux 1 */
#define REG_DCSRCR0     0x240   /* Driving cap. & Slew rate control 0 */
#define REG_DCSRCR1     0x244   /* Driving cap. & Slew rate control 1 */
#define REG_DCCR        0x254   /* Delay chain control register */
#define REG_PCR         0x258   /* Power control register */

#define TYPE_A369SCU    "a369-scu"
#define CFG_REGSIZE     (0x260 / 4)

typedef struct A369SCUState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;

    /* HW registers */
    uint32_t regs[CFG_REGSIZE];
} A369SCUState;

#define A369SCU(obj) \
    OBJECT_CHECK(A369SCUState, obj, TYPE_A369SCU)

#define SCU_REG32(s, off) \
    ((s)->regs[(off) / 4])

static uint64_t
a369scu_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    A369SCUState *s = A369SCU(opaque);
    uint64_t ret = 0;

    switch (addr) {
    case 0x000 ... (CFG_REGSIZE - 1) * 4:
        ret = s->regs[addr / 4];
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "a369scu: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
a369scu_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    A369SCUState *s = A369SCU(opaque);

    switch (addr) {
    case REG_GPINMUX:
    case REG_CLKCFG0:
    case REG_CLKCFG1:
    case REG_MFPINMUX0:
    case REG_MFPINMUX1:
        s->regs[addr / 4] = (uint32_t)val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "a369scu: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = a369scu_mem_read,
    .write = a369scu_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void a369scu_reset(DeviceState *ds)
{
    A369SCUState *s = A369SCU(SYS_BUS_DEVICE(ds));

    memset(s->regs, 0, sizeof(s->regs));

    SCU_REG32(s, REG_CHIPID)    = 0x00003369; /* A369 */
    SCU_REG32(s, REG_REVISON)   = 0x00010000; /* Rev. = 1.0.0 */
    SCU_REG32(s, REG_HWCFG)     = 0x00000c10; /* CPU = 4 * HCLK */
    SCU_REG32(s, REG_CPUMFCR)   = 0x00000230; /* CPU = 4 * HCLK */
    SCU_REG32(s, REG_SCUCR)     = 0x00000083; /* no low power detect */
    SCU_REG32(s, REG_SCUSR)     = 0x00000100; /* CPU freq. stable */
    SCU_REG32(s, REG_OSCCR)     = 0x00000003; /* OSCH disabled */
    SCU_REG32(s, REG_PLL1CR)    = 0x20010003; /* PLL_NS = 32 */
    SCU_REG32(s, REG_DLLCR)     = 0x00000003; /* DLL enabled & stable */
    SCU_REG32(s, REG_GPINMUX)   = 0x00001078; /* Pinmux */
    SCU_REG32(s, REG_EXTHWCFG)  = 0x00001cc8; /* NAND flash boot */
    SCU_REG32(s, REG_CLKCFG0)   = 0x26877330; /* LCD = HCLK */
    SCU_REG32(s, REG_CLKCFG1)   = 0x000a0a0a; /* SD = HCLK, SPI=PCLK */
    SCU_REG32(s, REG_SCER)      = 0x00003fff; /* All clock enabled */
    SCU_REG32(s, REG_MFPINMUX0) = 0x00000241; /* Pinmux */
    SCU_REG32(s, REG_MFPINMUX1) = 0x00000000; /* Pinmux */
    SCU_REG32(s, REG_DCSRCR0)   = 0x11111111; /* Slow slew rate */
    SCU_REG32(s, REG_DCSRCR1)   = 0x11111111; /* Slow slew rate */
    SCU_REG32(s, REG_DCCR)      = 0x00000303; /* All delay chain = 3 */
    SCU_REG32(s, REG_PCR)       = 0x8000007f; /* High performance mode */
}

static void a369scu_realize(DeviceState *dev, Error **errp)
{
    A369SCUState *s = A369SCU(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_A369SCU,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription vmstate_a369scu = {
    .name = TYPE_A369SCU,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, A369SCUState, CFG_REGSIZE),
        VMSTATE_END_OF_LIST(),
    }
};

static void a369scu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc  = TYPE_A369SCU;
    dc->vmsd  = &vmstate_a369scu;
    dc->reset = a369scu_reset;
    dc->realize = a369scu_realize;
//    dc->no_user = 1;
}

static const TypeInfo a369scu_info = {
    .name          = TYPE_A369SCU,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(A369SCUState),
    .class_init    = a369scu_class_init,
};

static void a369scu_register_types(void)
{
    type_register_static(&a369scu_info);
}

type_init(a369scu_register_types)
