/*
 * ARM Versatile/PB PCI host controller
 *
 * Copyright (c) 2006-2009 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the LGPL.
 */

#include "hw/sysbus.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_bus.h"
#include "hw/pci/pci_host.h"
#include "hw/pci/pcie_host.h"
#include "exec/address-spaces.h"
#include "hw/loader.h"


/* Old and buggy versions of QEMU used the wrong mapping from
 * PCI IRQs to system interrupt lines. Unfortunately the Linux
 * kernel also had the corresponding bug in setting up interrupts
 * (so older kernels work on QEMU and not on real hardware).
 * We automatically detect these broken kernels and flip back
 * to the broken irq mapping by spotting guest writes to the
 * PCI_INTERRUPT_LINE register to see where the guest thinks
 * interrupts are going to be routed. So we start in state
 * ASSUME_OK on reset, and transition to either BROKEN or
 * FORCE_OK at the first write to an INTERRUPT_LINE register for
 * a slot where broken and correct interrupt mapping would differ.
 * Once in either BROKEN or FORCE_OK we never transition again;
 * this allows a newer kernel to use the INTERRUPT_LINE
 * registers arbitrarily once it has indicated that it isn't
 * broken in its init code somewhere.
 *
 * Unfortunately we have to cope with multiple different
 * variants on the broken kernel behaviour:
 *  phase I (before kernel commit 1bc39ac5d) kernels assume old
 *   QEMU behaviour, so they use IRQ 27 for all slots
 *  phase II (1bc39ac5d and later, but before e3e92a7be6) kernels
 *   swizzle IRQs between slots, but do it wrongly, so they
 *   work only for every fourth PCI card, and only if (like old
 *   QEMU) the PCI host device is at slot 0 rather than where
 *   the h/w actually puts it
 *  phase III (e3e92a7be6 and later) kernels still swizzle IRQs between
 *   slots wrongly, but add a fixed offset of 64 to everything
 *   they write to PCI_INTERRUPT_LINE.
 *
 * We live in hope of a mythical phase IV kernel which might
 * actually behave in ways that work on the hardware. Such a
 * kernel should probably start off by writing some value neither
 * 27 nor 91 to slot zero's PCI_INTERRUPT_LINE register to
 * disable the autodetection. After that it can do what it likes.
 *
 * Slot % 4 | hw | I  | II | III
 * -------------------------------
 *   0      | 29 | 27 | 27 | 91
 *   1      | 30 | 27 | 28 | 92
 *   2      | 27 | 27 | 29 | 93
 *   3      | 28 | 27 | 30 | 94
 *
 * Since our autodetection is not perfect we also provide a
 * property so the user can make us start in BROKEN or FORCE_OK
 * on reset if they know they have a bad or good kernel.
 */

#define PCIREGS_ERR_DEBUG
#ifdef PCIREGS_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif
enum {
    PCI_VPB_IRQMAP_ASSUME_OK,
    PCI_VPB_IRQMAP_BROKEN,
    PCI_VPB_IRQMAP_FORCE_OK,
};

typedef struct {
    PCIHostState parent_obj;

    qemu_irq irq[4];
    MemoryRegion controlregs;
    MemoryRegion mem_config;
    MemoryRegion mem_config2;
    /* Containers representing the PCI address spaces */
    MemoryRegion pci_io_space;
    MemoryRegion pci_mem_space;
    /* Alias regions into PCI address spaces which we expose as sysbus regions.
     * The offsets into pci_mem_space are controlled by the imap registers.
     */
    MemoryRegion pci_io_window;
    MemoryRegion pci_mem_window[3];
    PCIBus pci_bus;
    PCIDevice pci_dev;

    /* Constant for life of device: */
    int realview;
    uint32_t mem_win_size[3];
    uint8_t irq_mapping_prop;

    /* Variable state: */
    uint32_t imap[3];
    uint32_t smap[3];
    uint32_t selfid;
    uint32_t flags;
    uint8_t irq_mapping;
    uint32_t config_baseaddr;
    uint32_t configindaddr;	/* pcie config space access: Address field: 0x120 */
    uint32_t configinddata;	/* pcie config space access: Data field: 0x124 */
    uint32_t func0_imap1;	/* 0xC80 */
    uint32_t iarr0_lower;	/* 0xD00 */
    uint32_t iarr0_upper;	/* 0xD04 */
    uint32_t iarr1_lower;	/* 0xD08 */
    uint32_t iarr1_upper;	/* 0xD0C */
    uint32_t iarr2_lower;	/* 0xD10 */
    uint32_t iarr2_upper;	/* 0xD14 */
    uint32_t oarr0;		/* 0xD20 */
    uint32_t oarr1;		/* 0xD28 */
    uint32_t oarr2;		/* 0xD30 */;
    uint32_t omap0_lower;	/* 0xD40 */
    uint32_t omap0_upper;	/* 0xD44 */
    uint32_t omap1_lower;	/* 0xD48 */
} PCIVPBState;

static const uint64_t pci_conf_data[] = {
0x801214E4,0x6801214,0x68012,0x10000680,0x100006,0x1001000,0x10010,0x100,0x2000001,0x10020000,0x100200,
0x1001002,0x10010,0x100,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1000000,0x1010000,0x10100,0x101,0x1,0x0,0x0,0x0,
0x8000000,0x50080000,0x8500800,0xF1085008,0xFFF10850,0x1FFF108,0x1FFF1,0x1FF,0x1,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x48000000,0x480000,0x4800,0x48,0x0,0x0,0x0,0x0,0xA3000000,0x1A30000,0x101A300,0x101A3,
0x101,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x1000000,0xAC010000,0x3AC0100,0xC803AC01,0x8C803AC,0x2008C803,0x2008C8,
0x2008,0x20,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10000000,0x100000,0x42001000,0x420010,0x2004200,0x80020042,
0x800200,0x8002,0x50000080,0x2C500000,0x102C5000,0x102C50,0x1200102C,0x5C120010,0x655C1200,0x655C12,0x655C,0x65,
0x12000000,0x90120000,0x901200,0x9012,0x90,0x0,0x0,0x0,0x40000000,0x400000,0x4000,0x40,0x1000000,0x10000,0x100,
0x1,0x0,0x0,0x1F000000,0x1F0000,0x8001F00,0x8001F,0x800,0x8,0x0,0x0,0x0,0x0,0x0,0x0,0x2000000,0x20000,0x200,0x2,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    };
 //   for (n = 0; n < ARRAY_SIZE(pci_conf_data); n++) {
   //     pci_conf_data[n] = tswap32(pci_conf_data[n]);
   // }
 //   rom_add_blob_fixed("pci_conf_data", pci_conf_data, sizeof(pci_conf_data), 0x18012400);
  //  rom_add_blob_fixed("pci_conf_data", pci_conf_data, sizeof(pci_conf_data), 0x18013400);
//}


static void pci_vpb_update_window(PCIVPBState *s, int i)
{
    /* Adjust the offset of the alias region we use for
     * the memory window i to account for a change in the
     * value of the corresponding IMAP register.
     * Note that the semantics of the IMAP register differ
     * for realview and versatile variants of the controller.
     */
    hwaddr offset;
    if (s->realview) {
        /* Top bits of register (masked according to window size) provide
         * top bits of PCI address.
         */
        offset = s->imap[i] & ~(s->mem_win_size[i] - 1);
    } else {
        /* Bottom 4 bits of register provide top 4 bits of PCI address */
        offset = s->imap[i] << 28;
    }
    memory_region_set_alias_offset(&s->pci_mem_window[i], offset);
}

static void pci_vpb_update_all_windows(PCIVPBState *s)
{
    /* Update all alias windows based on the current register state */
    int i;

    for (i = 0; i < 3; i++) {
        pci_vpb_update_window(s, i);
    }
}

static int pci_vpb_post_load(void *opaque, int version_id)
{
    PCIVPBState *s = opaque;
    pci_vpb_update_all_windows(s);
    return 0;
}

static const VMStateDescription pci_vpb_vmstate = {
    .name = "versatile-pci",
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = pci_vpb_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(imap, PCIVPBState, 3),
        VMSTATE_UINT32_ARRAY(smap, PCIVPBState, 3),
        VMSTATE_UINT32(selfid, PCIVPBState),
        VMSTATE_UINT32(flags, PCIVPBState),
        VMSTATE_UINT8(irq_mapping, PCIVPBState),
        VMSTATE_END_OF_LIST()
    }
};

#define TYPE_VERSATILE_PCI "versatile_pci"
#define PCI_VPB(obj) \
    OBJECT_CHECK(PCIVPBState, (obj), TYPE_VERSATILE_PCI)

#define TYPE_VERSATILE_PCI_HOST "versatile_pci_host"
#define PCI_VPB_HOST(obj) \
    OBJECT_CHECK(PCIDevice, (obj), TYPE_VERSATILE_PCIHOST)

typedef enum {
    PCI_IMAP0 = 0x0,
    PCI_IMAP1 = 0x4,
    PCI_IMAP2 = 0x8,
    PCI_SELFID = 0xc,
    PCI_FLAGS = 0x10,
    PCI_SMAP0 = 0x14,
    PCI_SMAP1 = 0x18,
    PCI_SMAP2 = 0x1c,
} PCIVPBControlRegs;

static inline void pci_cfg_write_word(PCIVPBState *s, uint8_t a)
{
    uint8_t x[2] = { a, 0 };
    
    cpu_physical_memory_write(s->config_baseaddr, x, 2);
}

static inline uint32_t pci_cfg_read_word(PCIVPBState *s, uint8_t val)
{
    uint32_t x[2];
    
    cpu_physical_memory_read(s->config_baseaddr + val, x, 1);
    DB_PRINT("addr: %08x val: 0x%" PRIx32 " data: 0x%" PRIx32 "\n", (unsigned)(s->config_baseaddr + val),val, (unsigned)x[1]);
    return x[0];
}

static void pci_vpb_reg_write(void *opaque, hwaddr addr,
                              uint64_t val, unsigned size)
{
    PCIVPBState *s = opaque;
    DB_PRINT("offset: %08x data: 0x%" PRIx64 "\n", (unsigned)addr, val);
    switch (addr) {
//    case PCI_IMAP0:
    case 0x00:
//    case PCI_IMAP1:
//    case PCI_IMAP2:
    {
        int win = (addr - PCI_IMAP0) >> 2;
        s->imap[win] = val;
        pci_vpb_update_window(s, win);
        break;
    }
    case PCI_SELFID:
        s->selfid = val;
        break;
    case PCI_FLAGS:
        s->flags = val;
        break;
    case PCI_SMAP0:
    case PCI_SMAP1:
    case PCI_SMAP2:
    {
        int win = (addr - PCI_SMAP0) >> 2;
        s->smap[win] = val;
        break;
    }
    case 0x120: /* configindaddr */	/* pcie config space access: Address field */
        s->configindaddr = val;
if (val==0x4d4) val=0xb0;
pci_cfg_read_word(s, val);


//if (val==0xb4) s->configinddata=0x102C50;
//if (val==0xc) s->configinddata=0x10010;
//if (val==0xdc) s->configinddata=0x2;
//if (val==0x4) s->configinddata=0x060010;
//if (val==0x10) s->configinddata=0x0;
        break;
    case 0x124: /* configinddata */	/* pcie config space access: Data field:  */
	s->configinddata = val;//0x10010000;//val;
        break; //0x2008; //for 0x18012000, 0x18013000.....0x10010000 for 0x18014000
    case 0x400 ... 0x7ff:
        break;
    case 0xC80: /* func0_imap1 */	
	s->func0_imap1 = val;
	break;
    case 0xD00: /* iarr0_lower */	
	s->iarr0_lower = val;
	break;
    case 0xD04: /* iarr0_upper */	
	s->iarr0_upper = val;
	break;
    case 0xD08: /* iarr1_lower */	
	s->iarr1_lower = val;
	break;
    case 0xD0C: /* iarr1_upper */	
	s->iarr1_upper = val;
	break;
    case 0xD10: /* iarr2_lower */	
	s->iarr2_lower = val;
	break;
    case 0xD14: /* iarr2_upper */	
	s->iarr1_upper = val;
	break;
    case 0xD20: /* oarr0 */		
	s->oarr0 = val;
	break;
    case 0xD28: /* oarr1 */		
	s->oarr1 = val;
	break;
    case 0xD30: /* oarr2 */		
	s->oarr2 = val;
	break;
    case 0xD40: /* omap0_lower */	
	s->omap0_lower = val;
	break;
    case 0xD44: /* omap0_upper */	
	s->omap0_upper = val;
	break;
    case 0xD48: /* omap1_lower */	
	s->omap1_lower = val;
	break;


    default:
        DB_PRINT("pci_vpb_reg_write: Bad offset %x\n", (int)addr);
        break;
    }
}

static uint64_t pci_vpb_reg_read_imp(void *opaque, hwaddr addr,
                                 unsigned size)
{
    PCIVPBState *s = opaque;

    switch (addr) {
    case PCI_IMAP0:
    case PCI_IMAP1:
    case PCI_IMAP2:
    {
        int win = (addr - PCI_IMAP0) >> 2;
        return s->imap[win];
    }
    case PCI_SELFID:
        return s->selfid;
    case PCI_FLAGS:
        return s->flags;
    case PCI_SMAP0:
    case PCI_SMAP1:
    case PCI_SMAP2:
    {
        int win = (addr - PCI_SMAP0) >> 2;
        return s->smap[win];
    }
    case 0x120: /* configindaddr */	/* pcie config space access: Address field */
        return s->configindaddr;  //0xBC;
    case 0x124: /* configinddata */	/* pcie config space access: Data field:  */
        return s->configinddata; //for 0x18012000, 0x18013000.....0x10010000 for 0x18014000
    default:
        DB_PRINT("pci_vpb_reg_read: Bad offset %x\n", (int)addr);
        return 0;
    }
}

static uint64_t pci_vpb_reg_read(void *opaque, hwaddr addr,
                                 unsigned size)
{
    uint32_t ret = pci_vpb_reg_read_imp(opaque, addr, size);

    DB_PRINT("addr: %08x data: 0x%" PRIx32 "\n", (unsigned)addr, ret);
    return ret;
}

static const MemoryRegionOps pci_vpb_reg_ops = {
    .read = pci_vpb_reg_read,
    .write = pci_vpb_reg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static int pci_vpb_broken_irq(int slot, int irq)
{
    /* Determine whether this IRQ value for this slot represents a
     * known broken Linux kernel behaviour for this slot.
     * Return one of the PCI_VPB_IRQMAP_ constants:
     *   BROKEN : if this definitely looks like a broken kernel
     *   FORCE_OK : if this definitely looks good
     *   ASSUME_OK : if we can't tell
     */
    slot %= PCI_NUM_PINS;

    if (irq == 27) {
        if (slot == 2) {
            /* Might be a Phase I kernel, or might be a fixed kernel,
             * since slot 2 is where we expect this IRQ.
             */
            return PCI_VPB_IRQMAP_ASSUME_OK;
        }
        /* Phase I kernel */
        return PCI_VPB_IRQMAP_BROKEN;
    }
    if (irq == slot + 27) {
        /* Phase II kernel */
        return PCI_VPB_IRQMAP_BROKEN;
    }
    if (irq == slot + 27 + 64) {
        /* Phase III kernel */
        return PCI_VPB_IRQMAP_BROKEN;
    }
    /* Anything else must be a fixed kernel, possibly using an
     * arbitrary irq map.
     */
    return PCI_VPB_IRQMAP_FORCE_OK;
}

static void pci_vpb_config_write(void *opaque, hwaddr addr,
                                 uint64_t val, unsigned size)
{
    PCIVPBState *s = opaque;
    DB_PRINT("addr: %08x data: 0x%" PRIx64 "\n", (unsigned)addr, val);
    if (!s->realview && (addr & 0xff) == PCI_INTERRUPT_LINE
        && s->irq_mapping == PCI_VPB_IRQMAP_ASSUME_OK) {
        uint8_t devfn = addr >> 8;
        s->irq_mapping = pci_vpb_broken_irq(PCI_SLOT(devfn), val);
    }
//    pcie_mmcfg_data_write(&s->pci_bus, 0x380000 + addr, val, size);
    pci_data_write(&s->pci_bus, addr, val, size);


}

static uint64_t pci_vpb_config_read(void *opaque, hwaddr addr,
                                    unsigned size)
{
    PCIVPBState *s = opaque;
    uint64_t val;
//    val = pcie_mmcfg_data_read(&s->pci_bus, s->config_baseaddr + addr, size);
    val = pci_data_read(&s->pci_bus, addr, size);
    DB_PRINT("addr: %08x data: 0x%" PRIx64 "\n", (unsigned)addr, val);
    return val;
}

static const MemoryRegionOps pci_vpb_config_ops = {
    .read = pci_vpb_config_read,
    .write = pci_vpb_config_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },

};

static int pci_vpb_map_irq(PCIDevice *d, int irq_num)
{
    PCIVPBState *s = container_of(d->bus, PCIVPBState, pci_bus);

    if (s->irq_mapping == PCI_VPB_IRQMAP_BROKEN) {
        /* Legacy broken IRQ mapping for compatibility with old and
         * buggy Linux guests
         */
        return irq_num;
    }

    /* Slot to IRQ mapping for RealView Platform Baseboard 926 backplane
     *      name    slot    IntA    IntB    IntC    IntD
     *      A       31      IRQ28   IRQ29   IRQ30   IRQ27
     *      B       30      IRQ27   IRQ28   IRQ29   IRQ30
     *      C       29      IRQ30   IRQ27   IRQ28   IRQ29
     * Slot C is for the host bridge; A and B the peripherals.
     * Our output irqs 0..3 correspond to the baseboard's 27..30.
     *
     * This mapping function takes account of an oddity in the PB926
     * board wiring, where the FPGA's P_nINTA input is connected to
     * the INTB connection on the board PCI edge connector, P_nINTB
     * is connected to INTC, and so on, so everything is one number
     * further round from where you might expect.
     */
    return pci_swizzle_map_irq_fn(d, irq_num + 2);
}

static int pci_vpb_rv_map_irq(PCIDevice *d, int irq_num)
{
    /* Slot to IRQ mapping for RealView EB and PB1176 backplane
     *      name    slot    IntA    IntB    IntC    IntD
     *      A       31      IRQ50   IRQ51   IRQ48   IRQ49
     *      B       30      IRQ49   IRQ50   IRQ51   IRQ48
     *      C       29      IRQ48   IRQ49   IRQ50   IRQ51
     * Slot C is for the host bridge; A and B the peripherals.
     * Our output irqs 0..3 correspond to the baseboard's 48..51.
     *
     * The PB1176 and EB boards don't have the PB926 wiring oddity
     * described above; P_nINTA connects to INTA, P_nINTB to INTB
     * and so on, which is why this mapping function is different.
     */
    return pci_swizzle_map_irq_fn(d, irq_num + 3);
}

static void pci_vpb_set_irq(void *opaque, int irq_num, int level)
{
    qemu_irq *pic = opaque;

    qemu_set_irq(pic[irq_num], level);
}

static void pci_vpb_reset(DeviceState *d)
{
    PCIVPBState *s = PCI_VPB(d);

    s->imap[0] = 0;
    s->imap[1] = 0;
    s->imap[2] = 0;
    s->smap[0] = 0;
    s->smap[1] = 0;
    s->smap[2] = 0;
    s->selfid = 0;
    s->flags = 0;
    s->irq_mapping = s->irq_mapping_prop;
//    s->configinddata = 0x8002; 
int n;
uint64_t x[2];
/* Load configuration data in config address space */
    for (n = 0; n < ARRAY_SIZE(pci_conf_data); n++) {
                cpu_physical_memory_write(0x18012400+n,
                                 &pci_conf_data[n],
                                  4);
  cpu_physical_memory_read(0x18012400+n,
                                 x,
                                  4);
    }
  


     //   cpu_physical_memory_write(0x18013400,
       //                           pci_conf_data,
         //                         sizeof(pci_conf_data));

    pci_vpb_update_all_windows(s);
}

static void pci_vpb_init(Object *obj)
{
    PCIHostState *h = PCI_HOST_BRIDGE(obj);
    PCIVPBState *s = PCI_VPB(obj);

    memory_region_init(&s->pci_io_space, OBJECT(s), "pci_io", 1ULL << 32);
    memory_region_init(&s->pci_mem_space, OBJECT(s), "pci_mem", 1ULL << 32);

    pci_bus_new_inplace(&s->pci_bus, sizeof(s->pci_bus), DEVICE(obj), "pci",
                        &s->pci_mem_space, &s->pci_io_space,
                        PCI_DEVFN(11, 0), TYPE_PCI_BUS);
    h->bus = &s->pci_bus;

    object_initialize(&s->pci_dev, sizeof(s->pci_dev), TYPE_VERSATILE_PCI_HOST);
    qdev_set_parent_bus(DEVICE(&s->pci_dev), BUS(&s->pci_bus));

    /* Window sizes for VersatilePB; realview_pci's init will override */
    s->mem_win_size[0] = 0x0c000000;
    s->mem_win_size[1] = 0x10000000;
    s->mem_win_size[2] = 0x10000000;

}

static void pci_vpb_realize(DeviceState *dev, Error **errp)
{
    PCIVPBState *s = PCI_VPB(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    pci_map_irq_fn mapfn;
    int i;

    for (i = 0; i < 4; i++) {
        sysbus_init_irq(sbd, &s->irq[i]);
    }

    if (s->realview) {
        mapfn = pci_vpb_rv_map_irq;
    } else {
        mapfn = pci_vpb_map_irq;
    }

    pci_bus_irqs(&s->pci_bus, pci_vpb_set_irq, mapfn, s->irq, 4);

    /* Our memory regions are:
     * 0 : our control registers
     * 1 : PCI self config window
     * 2 : PCI config window
     * 3 : PCI IO window
     * 4..6 : PCI memory windows
     */

    	memory_region_init_io(&s->controlregs, OBJECT(s), &pci_vpb_reg_ops, s,
                          "pci-vpb-regs", 0x400);
    	sysbus_init_mmio(sbd, &s->controlregs);
    	memory_region_init_io(&s->mem_config, OBJECT(s), &pci_vpb_config_ops, s,
                          "pci-vpb-selfconfig", 0x400);
    	sysbus_init_mmio(sbd, &s->mem_config);
    	memory_region_init_io(&s->mem_config2, OBJECT(s), &pci_vpb_config_ops, s,
                          "pci-vpb-config", 0x100);
    	sysbus_init_mmio(sbd, &s->mem_config2);

   	 /* The window into I/O space is always into a fixed base address;
   	  * its size is the same for both realview and versatile.
   	  */
    	memory_region_init_alias(&s->pci_io_window, OBJECT(s), "pci-vbp-io-window",
                             &s->pci_io_space, 0, 0x100000);

   	 sysbus_init_mmio(sbd, &s->pci_io_space);

   	 /* Create the alias regions corresponding to our three windows onto
   	  * PCI memory space. The sizes vary from board to board; the base
   	  * offsets are guest controllable via the IMAP registers.
   	  */
    for (i = 0; i < 3; i++) {
       	 memory_region_init_alias(&s->pci_mem_window[i], OBJECT(s), "pci-vbp-window",
                                 &s->pci_mem_space, 0, s->mem_win_size[i]);
         sysbus_init_mmio(sbd, &s->pci_mem_window[i]);
    }

    /* TODO Remove once realize propagates to child devices. */
    object_property_set_bool(OBJECT(&s->pci_dev), true, "realized", errp);


}

static int versatile_pci_host_init(PCIDevice *d)
{
    pci_set_word(d->config + PCI_STATUS,
                 PCI_STATUS_66MHZ | PCI_STATUS_DEVSEL_MEDIUM);
    pci_set_byte(d->config + PCI_LATENCY_TIMER, 0x10);
    return 0;
}

static void versatile_pci_host_class_init(ObjectClass *klass, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    k->init = versatile_pci_host_init;
//    k->vendor_id = PCI_VENDOR_ID_XILINX;
k->vendor_id = PCI_VENDOR_ID_BROADCOM;
//    k->device_id = PCI_DEVICE_ID_XILINX_XC2VP30;
k->device_id = BCM501_UNKNOWN;
    k->class_id = PCI_CLASS_PROCESSOR_CO;
    /*
     * PCI-facing part of the host bridge, not usable without the
     * host-facing part, which can't be device_add'ed, yet.
     */
    dc->cannot_instantiate_with_device_add_yet = true;
}

static const TypeInfo versatile_pci_host_info = {
    .name          = TYPE_VERSATILE_PCI_HOST,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCIDevice),
    .class_init    = versatile_pci_host_class_init,
};

static Property pci_vpb_properties[] = {
    DEFINE_PROP_UINT8("broken-irq-mapping", PCIVPBState, irq_mapping_prop,
                      PCI_VPB_IRQMAP_ASSUME_OK),
    DEFINE_PROP_UINT32("config_baseaddr", PCIVPBState, config_baseaddr, 0),
    DEFINE_PROP_END_OF_LIST()
};

static void pci_vpb_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = pci_vpb_realize;
    dc->reset = pci_vpb_reset;
    dc->vmsd = &pci_vpb_vmstate;
    dc->props = pci_vpb_properties;
}

static const TypeInfo pci_vpb_info = {
    .name          = TYPE_VERSATILE_PCI,
    .parent        = TYPE_PCI_HOST_BRIDGE,
    .instance_size = sizeof(PCIVPBState),
    .instance_init = pci_vpb_init,
    .class_init    = pci_vpb_class_init,
};

static void pci_realview_init(Object *obj)
{
    PCIVPBState *s = PCI_VPB(obj);

    s->realview = 1;
    /* The PCI window sizes are different on Realview boards */
//    s->mem_win_size[0] = 0x01000000;
//    s->mem_win_size[1] = 0x04000000;
/*for broadcom pci memory window sized */
    s->mem_win_size[0] = 0x08000000;
    s->mem_win_size[1] = 0x08000000;
    s->mem_win_size[2] = 0x08000000;
}

static const TypeInfo pci_realview_info = {
    .name          = "realview_pci",
    .parent        = TYPE_VERSATILE_PCI,
    .instance_init = pci_realview_init,
};

static void versatile_pci_register_types(void)
{
    type_register_static(&pci_vpb_info);
    type_register_static(&pci_realview_info);
    type_register_static(&versatile_pci_host_info);
}

type_init(versatile_pci_register_types)
