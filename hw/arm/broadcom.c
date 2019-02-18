/*
 * ARM RealView Baseboard System emulation.
 *
 * Copyright (c) 2006-2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/sysbus.h"
#include "hw/arm/arm.h"
#include "hw/arm/imx.h"
#include "hw/unicore32/puv3.h"
#include "hw/arm/primecell.h"
#include "hw/char/serial.h"
#include "hw/devices.h"
#include "hw/pci/pci.h"
#include "net/net.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/i2c/i2c.h"
#include "sysemu/blockdev.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "hw/block/flash.h"


#include "hw/loader.h"
#include "hw/ssi.h"



#define SMP_BOOT_ADDR 0xffff0000
#define SMP_BOOTREG_ADDR 0xffff0400
#define MPCORE_PERIPHBASE 0x19020000
#define IRQ_OFFSET 32 /* pic interrupts start from index 32 */
#define NUM_IRQS 256
#define FLASH_SIZE (16 * 1024 * 1024)
#define FLASH_SECTOR_SIZE (128 * 1024)
static struct arm_boot_info broadcom_binfo = {
    .smp_loader_start = SMP_BOOT_ADDR,
    .smp_bootreg_addr = SMP_BOOTREG_ADDR,
};

/* The following two lists must be consistent.  */
enum broadcom_board_type {
    BRCM_GEN,
    BRCM_NS,
    BRCM_NS_QT
};

static const int broadcom_board_id[] = {
    0x15b3,
    0x2706,
//0xffffffff,
    0x2708
};

static const int broadcom_board_zreladdr[] = {
    0x80200000,
    0x00000000,
    0x80000000
};

static const uint32_t a9_clocks[] = {
    500000000, /* AMBA AXI ACLK: 500MHz */
    23750000, /* CLCD clock: 23.75MHz */
    66670000, /* Test chip reference clock: 66.67MHz */
};

static void erom(void)
{
    int n;
    uint32_t erom[] = {

	/* CCA_CORE_ID */
        0x4BF80001,0x2A004201,0x18000005,0x181200C5,
	/* CCB_CORE_ID */
	0x4BF50B01,0x1000201,0x18001005,0x18002005,0x18003005,0x18004005,0x18005005,0x18006005,	0x18007005,0x18008005,0x18009005,
	/* NS_DMA_CORE_ID */
	0x4BF50201,0x1004211,0x3,0x1802C005,0x181140C5,
	/* GMAC_CORE_ID */
	0x4BF82D01,0x5004211,0x103,0x18024005,0x181100C5,
	/* GMAC_CORE_ID */
	0x4BF82D01,0x5004211,0x203,0x18025005,0x181110C5,
	/* GMAC_CORE_ID */
	0x4BF82D01,0x5004211,0x303,0x18026005,0x181120C5,
	/* GMAC_CORE_ID */
	0x4BF82D01,0x5004211,0x403,0x18027005,0x181130C5,
	/* NS_PCIEG2_CORE_ID */
	0x4BF50101,0x1084411,0x503,0x18012005,0x8000135,0x8000000,0x181010C5,0x1810A185,
	/* NS_PCIEG2_CORE_ID */
	0x4BF50101,0x1084411,0x603,0x18013005,0x40000135,0x8000000,0x181020C5,0x1810B185,
	/* NS_PCIEG2_CORE_ID */
	0x4BF50101,0x1084411,0x703,0x18014005,0x48000135,0x8000000,0x181030C5,0x1810C185,
	/* ARMCA9_CORE_ID */
	0x4BF51001,0x1104611,0x803,0x1800B005,0x1800C005,0x19000135,0x20000,0x19020235,0x3000,0x181000C5,0x18106185,0x18107285,
	/* NS_USB20_CORE_ID */
	0x4BF50401,0x1004211,0x903,0x18021005,0x18022005,0x181150C5,
	/* NS_USB30_CORE_ID */
	0x4BF50501,0x1004211,0xA03,0x18023005,0x181050C5,
	/* NS_SDIO3_CORE_ID */
	0x4BF50301,0x1004211,0xB03,0x18020005,0x181160C5,
	/* I2S_CORE_ID */
	0x4BF83401,0x3004211,0xC03,0x1802A005,0x181170C5,
	/* NS_A9JTAG_CORE_ID */
	0x4BF50601,0x1084211,0xD03,0x18210035,0x10000,0x181180C5,0x1811C085,
	/* NS_DDR23_CORE_ID--Denali DDR */			
	0x4BF50701,0x1100601,0x18010005,0x135,0x8000000,0x80000135,0x30000000,0xB0000235,0x10000000,
	0x18108185,0x18109285,
	/* NS_ROM_CORE_ID */	
	0x4BF50801,0x1080201,0xFFFD0035,0x30000,0x1810D085,
	/* NS_NAND_CORE_ID */	
	0x4BF50901,0x1080401,0x18028005,0x1C000135,0x2000000,0x1811A185,
	/* NS_QSPI_CORE_ID */	
	0x4BF50A01,0x1080401,0x18029005,0x1E000135,0x2000000,0x1811B185, 
	/* EROM_CORE_ID */
	0x43B36601,0x201,0x18130005,0x43B13501,0x80201,0x18000075,0x10000,0x18121085,
	/* AXI_CORE_ID */
	0x43B30101,0x1000201,0x1A000035,0x100000,
	/* DEF_AI_COMP_ID */
	0x43BFFF01,0x280A01,0x10000035,0x8000000,0x18011005,0x18015035,0xB000,0x1802B105,0x1802D135,0xD3000,0x18104105,
	0x1810E215,0x18119205,0x1811D235,0x3000,0x18122335,0xE000,0x18131305,0x18137335,0xD9000,0x18220335,0xDE000,0x19023335,
	0xFDD000,0x1A100335,0x1F00000,0x20000435,0x20000000,0x50000435,0x30000000,0xC0000435,0x3FFD0000,0x18132085,0x18133185,
	0x18134285,0x18135385,0x18136485,
	/* EROM End  */
	0xF  
    };
    for (n = 0; n < ARRAY_SIZE(erom); n++) {
        erom[n] = tswap32(erom[n]);
    }
    rom_add_blob_fixed("erom", erom, sizeof(erom), 0xffff1000);
}

#define D(x)
#define DNAND(x)
//#define NCREGS_ERR_DEBUG
#ifdef NCREGS_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif

#if 0
struct nand_state_t
{
    DeviceState *nand;
    MemoryRegion iomem;
    unsigned int rdy:1;
    unsigned int ale:1;
    unsigned int cle:1;
    unsigned int ce:1;
};

//static struct nand_state_t nand_state;
static uint64_t nand_read(void *opaque, hwaddr addr, unsigned size)
{
    struct nand_state_t *s = opaque;
    uint32_t r;
    int rdy;
    switch (addr) {
	case 0x194: /* revision */
	DB_PRINT("Bad register offset 0x%x\n", (int)addr);
	    return 1;

    default:
	DB_PRINT("Bad register offset 0x%x\n", (int)addr);
        return 0;
    }

    r = nand_getio(s->nand);
    nand_getpins(s->nand, &rdy);
    s->rdy = rdy;

    DNAND(printf("%s addr=%x r=%x\n", __func__, addr, r));
    return r;
}

static void
nand_write(void *opaque, hwaddr addr, uint64_t value,
           unsigned size)
{
    struct nand_state_t *s = opaque;
    int rdy;

    DNAND(printf("%s addr=%x v=%x\n", __func__, addr, (unsigned)value));
    nand_setpins(s->nand, s->cle, s->ale, s->ce, 1, 0);
    nand_setio(s->nand, value);
    nand_getpins(s->nand, &rdy);
    s->rdy = rdy;
}

static const MemoryRegionOps nand_ops = {
    .read = nand_read,
    .write = nand_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};
#endif

static void broadcom_init(QEMUMachineInitArgs *args,
                          enum broadcom_board_type board_type)
{
  //  ARMCPU *cpu = NULL;
 //   CPUARMState *env;
    ObjectClass *cpu_oc;
    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *ram_lo = g_new(MemoryRegion, 1);
    MemoryRegion *ram_hi = g_new(MemoryRegion, 1);
    MemoryRegion *ram_alias = g_new(MemoryRegion, 1);
    MemoryRegion *ram_hack = g_new(MemoryRegion, 1);
    DeviceState *dev; //*sysctl;//, *gpio2;//, *pl041;
    SysBusDevice *busdev;
    qemu_irq pic[NUM_IRQS];
//    qemu_irq mmc_irq[2];
    PCIBus *pci_bus = NULL;
 //   NICInfo *nd;
//    I2CBus *i2c;
    int n;
//    int done_nic = 0;
    qemu_irq cpu_irq[4];

//    uint32_t proc_id = 0;
//    uint32_t sys_id;
    ram_addr_t low_ram_size;
    ram_addr_t ram_size = args->ram_size;
//    hwaddr periphbase = 0;


    cpu_oc = cpu_class_by_name(TYPE_ARM_CPU, args->cpu_model);
    if (!cpu_oc) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }


    for (n = 0; n < smp_cpus; n++) {
        Object *cpuobj = object_new(object_class_get_name(cpu_oc));
        Error *err = NULL;
        object_property_set_int(cpuobj,MPCORE_PERIPHBASE, "reset-cbar", &err);
        if (err) {
            error_report("%s", error_get_pretty(err));
            exit(1);
        }


        object_property_set_bool(cpuobj, true, "realized", &err);
        if (err) {
            error_report("%s", error_get_pretty(err));
            exit(1);
        }

        cpu_irq[n] = qdev_get_gpio_in(DEVICE(cpuobj), ARM_CPU_IRQ);
    }
 //   cpu = ARM_CPU(first_cpu);
#if 0

    if (ram_size > 0x10000000) {
        /* Core tile RAM.  */
        low_ram_size = ram_size - 0x10000000;
        ram_size = 0x10000000;
        memory_region_init_ram(ram_lo, NULL, "System.lowmem", low_ram_size);
        vmstate_register_ram_global(ram_lo);
        memory_region_add_subregion(sysmem, 0x00000000, ram_lo);
    }

    memory_region_init_ram(ram_hi, NULL, "System.highmem", ram_size);
    vmstate_register_ram_global(ram_hi);
    low_ram_size = ram_size;
    if (low_ram_size > 0x10000000)
      low_ram_size = 0x10000000;
    /* SDRAM at address zero.  */
    memory_region_init_alias(ram_alias, NULL, "System.alias",
                             ram_hi, 0, low_ram_size);
    memory_region_add_subregion(sysmem, 0, ram_alias);
    if (1) {
        /* And again at a high address.  */
        memory_region_add_subregion(sysmem, 0x88000000, ram_hi);
    } else {
        ram_size = low_ram_size;
    }
#endif
#if 1
 //   MemoryRegion *ram_alias = g_new(MemoryRegion, 1);
    if (ram_size > 0x8000000) { //128 * 1024 * 1024--can be 128M,256M,512M,1024M
        /* Core tile RAM.  */
        low_ram_size = ram_size - 0x8000000;
        ram_size = 0x8000000;
        memory_region_init_ram(ram_lo, NULL, "System.lowmem", low_ram_size);
        vmstate_register_ram_global(ram_lo);
        memory_region_add_subregion(sysmem, 0x0000000, ram_lo);
    }

    low_ram_size = ram_size;
    if (low_ram_size > 0x8000000)
      low_ram_size = 0x8000000;
    /* SDRAM at address zero.  */
    memory_region_init_alias(ram_alias, NULL, "System.alias",ram_hi, 0, low_ram_size);
    memory_region_add_subregion(sysmem, 0, ram_alias);

    /* And again at a high address.  */
    memory_region_init_ram(ram_hi, NULL, "System.highmem", ram_size);
    vmstate_register_ram_global(ram_hi);
    memory_region_add_subregion(sysmem, 0x88000000, ram_hi);

/*
    DriveInfo *dinfo = drive_get(IF_PFLASH, 0, 0);
    pflash_cfi02_register(0x1c000000, NULL, "brcm,bcm47xx-nvram", FLASH_SIZE,
                          dinfo ? dinfo->bdrv : NULL, FLASH_SECTOR_SIZE,
                          FLASH_SIZE/FLASH_SECTOR_SIZE, 1,
                          1, 0xc2, 0xf1, 0x80, 0x1d, 0xc2, 0xf1,
                              0);
*/

    MemoryRegion *sysrom;
    sysrom = g_new(MemoryRegion, 1);
    memory_region_init_ram(sysrom, NULL, "nvram.sysrom", 0x200000);
    memory_region_add_subregion(sysmem, 0x1c000000, sysrom);
    char *sysboot_filename;
    if (bios_name != NULL) {
        sysboot_filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, bios_name);
        if (sysboot_filename != NULL) {
            uint32_t filesize = get_image_size(sysboot_filename);
            if (load_image_targphys("mtd.bios", 0x1c000000,0x400000) < 0) {
                hw_error("Unable to load %s size %x\n", bios_name,filesize);
            }
        } else {
           hw_error("Unable to find %s\n", bios_name);
        }
    }
/*
    DriveInfo *dmtd = drive_get(IF_MTD, 0, 0);
    DeviceState *nand;
    MemoryRegion *nandiomem;
    nandiomem = g_new(MemoryRegion, 1);
    memory_region_init_ram(nandiomem, NULL, "nand", 0x200000);
    memory_region_add_subregion(sysmem, 0x1c000000, nandiomem);
    if (!dmtd) {
        hw_error("%s: NAND image required", __FUNCTION__);
    }
*/
//    nand = nand_init(dmtd ? dmtd->bdrv : NULL, NAND_MFR_MXIC, 0xf1);
  //  nand_setpins(nand, 0, 0, 0, 1, 0); /* no write-protect */

 //   nand = sysbus_create_simple("nand", 0x1c000000, pic[30]);
#endif 




//   DriveInfo *nand = drive_get(IF_MTD, 0, 0); 


 //   if (!nand) {
 //       hw_error("%s: NAND image required", __FUNCTION__);
 //   }
  //  nand_state.nand = nand_init(nand ? nand->bdrv : NULL, NAND_MFR_MXIC, 0xf1);

  //  memory_region_init_io(&nand_state.iomem, NULL, &nand_ops, &nand_state,
  //                        "nand", 0x1000);
  //  memory_region_add_subregion(sysmem, 0x18028000,
   //                             &nand_state.iomem);
//DriveInfo *nand = drive_get(IF_MTD, 0, 0);
  //  if (!nand) {
 //       hw_error("%s: NAND image required", __FUNCTION__);
  //  }
//s->nand = ncregs_init(nand ? blk_by_legacy_dinfo(nand) : NULL, NAND_MFR_MXIC, 0xf1);    
//memory_region_init_io(&s->iomem, OBJECT(s), &ncregs_ops, s, "ncregs", 0x18028000); 
//sysbus_init_mmio(dev, &s->iomem);

#if 0
    if (ram_size > 0x40000000) {
        /* 1GB is the maximum the address space permits */
        fprintf(stderr, "brcm-ns: cannot model more than 1GB RAM\n");
        exit(1);
    }

    low_ram_size = ram_size;
    if (low_ram_size > 0x8000000) {
        low_ram_size = 0x8000000;
    }
    /* RAM is from 0x88000000 upwards. The bottom 128MB of the
     * address space should in theory be remappable to various
     * things including ROM or RAM; we always map the RAM there.
     */
    memory_region_init_alias(ram_lo, NULL, "broadcom.lowmem", ram_hi, 0, low_ram_size);
    memory_region_add_subregion(sysmem, 0x0, ram_lo);
    memory_region_init_ram(ram_hi, NULL, "broadcom.highmem", ram_size);
    vmstate_register_ram_global(ram_hi);
    memory_region_add_subregion(sysmem, 0x88000000, ram_hi);


#endif
#if 1
   erom();
#endif
#if 1
    DeviceState *chipcommon;
    chipcommon = qdev_create(NULL, "sb_chipc");
    qdev_prop_set_uint32(chipcommon, "chipid", 0x3f00cf12);//0x0fCF4710);
    qdev_prop_set_uint32(chipcommon, "core_id", 0x800);
//    qdev_prop_set_uint32(chipcommon, "coreidhi", 0x4243502f);
//    qdev_prop_set_uint32(chipcommon, "coreidhi", 0x424350bf);
    qdev_init_nofail(chipcommon);
    sysbus_mmio_map(SYS_BUS_DEVICE(chipcommon), 0, 0x18000000);
#endif


///chipcommon b core///
    DeviceState *ai_regs, *nc;

    /* CCA_CORE_ID */
    ai_regs = qdev_create(NULL, "aidmpregs");
    qdev_prop_set_uint32(ai_regs, "core_id", 0x800);
    qdev_init_nofail(ai_regs);
//    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18000000);
    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18120000);

    /* CCB_CORE_ID */
    ai_regs = qdev_create(NULL, "aidmpregs");
    qdev_prop_set_uint32(ai_regs, "core_id", 0x50b);
    qdev_init_nofail(ai_regs);
    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18001000);

    /* PCIE Gen 2 core */
 //   ai_regs = qdev_create(NULL, "aidmpregs");
 //   qdev_prop_set_uint32(ai_regs, "core_id", 0x501);
 //   qdev_init_nofail(ai_regs);
 //   sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18012000);

    /* NS_DMA_CORE_ID */
    ai_regs = qdev_create(NULL, "aidmpregs");
    qdev_prop_set_uint32(ai_regs, "core_id", 0x502);
    qdev_init_nofail(ai_regs);
    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x1802c000);
    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18114000);

    /* NS_USB30_CORE_ID */
    ai_regs = qdev_create(NULL, "aidmpregs");
    qdev_prop_set_uint32(ai_regs, "core_id", 0x505);
    qdev_init_nofail(ai_regs);
//    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18023000);
    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18105000);

#if 1
    /* NS_NAND_CORE_ID */
    ai_regs = qdev_create(NULL, "aidmpregs");
    qdev_prop_set_uint32(ai_regs, "core_id", 0x509);
    qdev_init_nofail(ai_regs);
 //   sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x18028000);
        sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x1811A000);

    nc = qdev_create(NULL, "ncregs");

 //  qdev_prop_set_uint8(nc, "manf_id", 0xc2);
//   qdev_prop_set_uint8(nc, "chip_id", 0xf1);
//qdev_prop_set_drive_nofail(nc, "drive", bdrv);
    qdev_init_nofail(nc);
    sysbus_mmio_map(SYS_BUS_DEVICE(nc), 0, 0x18028000);
#endif
    /* NS_ROM_CORE_ID */
    ai_regs = qdev_create(NULL, "aidmpregs");
    qdev_prop_set_uint32(ai_regs, "core_id", 0x508);
    qdev_init_nofail(ai_regs);
//    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0xfffd0000);
    sysbus_mmio_map(SYS_BUS_DEVICE(ai_regs), 0, 0x1810d000);

#if 0
    DeviceState *ssb;
    ///chipcommon a core///
    ssb = qdev_create(NULL, "sb_hwregs");
//    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243a00a); /*SSB core id*/
qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243a00a);
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000001);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18000000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18000f00);

    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x424350ba);
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000008);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18001000);
    qdev_prop_set_uint32(ssb, "sbadmatch1", 0x18002000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18001f00);

    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243a04a);
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000001);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18002000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18002f00);

    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243509a);
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000001);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18028000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18003f00);

    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243501a);
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000001);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18012000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18004f00);

    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243501a);
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000001);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18013000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18005f00); 
   
    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243501a);
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000001);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18014000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18006f00);

    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243510a);    
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000008);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x1800b000);
    qdev_prop_set_uint32(ssb, "sbadmatch1", 0x1800c000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18007f00);
    ssb = qdev_create(NULL, "sb_hwregs");
    qdev_prop_set_uint32(ssb, "sbidhigh", 0x4243507a);    
    qdev_prop_set_uint32(ssb, "sbidlow", 0x70000001);
    qdev_prop_set_uint32(ssb, "sbadmatch0", 0x18010000);
    qdev_init_nofail(ssb);
    sysbus_mmio_map(SYS_BUS_DEVICE(ssb), 0, 0x18008f00);
#endif
        dev = qdev_create(NULL,"a9mpcore_priv");
        qdev_prop_set_uint32(dev, "num-cpu", smp_cpus);
    	qdev_prop_set_uint32(dev, "num-irq", NUM_IRQS);
    	qdev_init_nofail(dev);
    	busdev = SYS_BUS_DEVICE(dev);
    	sysbus_mmio_map(busdev, 0, MPCORE_PERIPHBASE);

        for (n = 0; n < smp_cpus; n++) {
            sysbus_connect_irq(busdev, n, cpu_irq[n]);
        }
       sysbus_create_varargs("l2x0", MPCORE_PERIPHBASE + 0x2000, NULL);

    for (n = 0; n < NUM_IRQS-IRQ_OFFSET; n++) {
        pic[n] = qdev_get_gpio_in(dev, n);
    }
    /*Setup CHIPCOMMON_A and CHIPCOMMON_B UARTS*/
#if 1
   //  if (serial_hds[0] != NULL) {
	    serial_mm_init(sysmem, 0x18000300, 0,   //UART0 CHIPCOMMON_A "ns16550"
                   pic[117-32], 115200, serial_hds[0],
                   DEVICE_LITTLE_ENDIAN);
    // }
   //  if (serial_hds[1] != NULL) {
	    serial_mm_init(sysmem, 0x18000400, 0,   //UART1 CHIPCOMMON_A "ns16550"
                   pic[117-32], 115200, serial_hds[0],
                  DEVICE_LITTLE_ENDIAN);
    //}
   //  if (serial_hds[2] != NULL) {
	    serial_mm_init(sysmem, 0x18008000, 0,   //UART0 CHIPCOMMON_B "ns16550"
                   pic[118-32], 115200, serial_hds[0],
                  DEVICE_LITTLE_ENDIAN);
  //  }
#endif

#if 0
    sysbus_create_simple("pl011", 0x18000300, pic[117-32]);//UART0 CHIPCOMMON_A
    sysbus_create_simple("pl011", 0x18000400, pic[117-32]);//UART1 CHIPCOMMON_A
    sysbus_create_simple("pl011", 0x18008000, pic[118-32]);//UART0 CHIPCOMMON_B
#endif

    /* DMA controller is optional, apparently.  */
   // sysbus_create_simple("pl081", 0x18030000, pic[24]);



  //  sysbus_create_simple("pl061", 0x18013000, pic[6]);
 //   sysbus_create_simple("pl061", 0x18014000, pic[7]);
 //   gpio2 = sysbus_create_simple("pl061", 0x18015000, pic[8]);

//    sysbus_create_simple("pl111", 0x18020000, pic[23]); //LCD

  //  dev = sysbus_create_varargs("pl181", 0x18005000, pic[17], pic[18], NULL);
    /* Wire up MMC card detect and read-only signals. These have
     * to go to both the PL061 GPIO and the sysctl register.
     * Note that the PL181 orders these lines (readonly,inserted)
     * and the PL061 has them the other way about. Also the card
     * detect line is inverted.
     */
//    mmc_irq[0] = qemu_irq_split(
 //       qdev_get_gpio_in(sysctl, ARM_SYSCTL_GPIO_MMC_WPROT),
//        qdev_get_gpio_in(gpio2, 1));
//    mmc_irq[1] = qemu_irq_split(
//        qdev_get_gpio_in(sysctl, ARM_SYSCTL_GPIO_MMC_CARDIN),
 //       qemu_irq_invert(qdev_get_gpio_in(gpio2, 0)));
//    qdev_connect_gpio_out(dev, 0, mmc_irq[0]);
 //   qdev_connect_gpio_out(dev, 1, mmc_irq[1]);

 //   sysbus_create_simple("pl031", 0x18017000, pic[10]);
#if 0
    dev = qdev_create(NULL, "pl330");  //DMA
    qdev_prop_set_uint8(dev, "num_chnls",  8);
    qdev_prop_set_uint8(dev, "num_periph_req",  4);
    qdev_prop_set_uint8(dev, "num_events",  16);

    qdev_prop_set_uint8(dev, "data_width",  64);
    qdev_prop_set_uint8(dev, "wr_cap",  8);
    qdev_prop_set_uint8(dev, "wr_q_dep",  16);
    qdev_prop_set_uint8(dev, "rd_cap",  8);
    qdev_prop_set_uint8(dev, "rd_q_dep",  16);
    qdev_prop_set_uint16(dev, "data_buffer_dep",  256);

    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x19022000);
    sysbus_connect_irq(busdev, 0, pic[45-IRQ_OFFSET]); /* abort irq line */
    for (n = 0; n < 8; ++n) { /* event irqs */
        sysbus_connect_irq(busdev, n + 1, pic[dma_irqs[n] - IRQ_OFFSET]);
    }
#endif

        dev = qdev_create(NULL, "imx_ccm");
        busdev = SYS_BUS_DEVICE(dev);
        qdev_init_nofail(dev);
        sysbus_mmio_map(busdev, 0, 0x1800c000); /* DMU registers */
#if 1
        /* Broadcom Northstar PCIE */
	/* size of windows 128M---> 0x8000000 */
        dev = qdev_create(NULL, "realview_pci");
        qdev_prop_set_uint32(dev,"config_baseaddr",0x18012400);
        busdev = SYS_BUS_DEVICE(dev);
        qdev_init_nofail(dev);
        sysbus_mmio_map(busdev, 0, 0x18012000); /* PCIE0 controller registers */
	sysbus_mmio_map(busdev, 1, 0x18012c00); /* PCIE self-config */
	sysbus_mmio_map(busdev, 2, 0x18012400); /* PCIE config */
        sysbus_mmio_map(busdev, 4, 0x08000000); /* PCIE memory window 1, Port 0 */
        sysbus_connect_irq(busdev, 0, pic[131]);
        pci_bus = (PCIBus *)qdev_get_child_bus(dev, "pci");
        if (!pci_bus) {
           fprintf(stderr, "couldn't create PCI controller!\n");
           exit(1);
       }
       if (pci_bus) {
           /* Register network interfaces. */

	//   int i;
        //   for (i = 0; i < nb_nics; i++) {
        //       nd = &nd_table[i];
//printf("nic  %d\n",i);
               /* There are no PCI NICs on the Bamboo board, but there are
                * PCI slots, so we can pick whatever default model we want. */
               pci_nic_init_nofail(&nd_table[4], pci_bus, "e1000", NULL);
          // }
       }	

#endif
        dev = qdev_create(NULL, "realview_pci");
        busdev = SYS_BUS_DEVICE(dev);
        qdev_init_nofail(dev);
        sysbus_mmio_map(busdev, 0, 0x18013000); /* PCIE1 controller registers */
	sysbus_mmio_map(busdev, 2, 0x18013400); /* PCIE config */
        sysbus_connect_irq(busdev, 0, pic[137]);
        sysbus_mmio_map(busdev, 5, 0x40000000); /* PCIE memory window 2, Port 1 */
        pci_bus = (PCIBus *)qdev_get_child_bus(dev, "pci");
        if (usb_enabled(true)) {
            pci_create_simple(pci_bus, -1, "BCM471A-usb-uhci");  //PCIDevice *pci_create(PCIBus *bus, int devfn, const char *name) devfn may be address TODO
            pci_create_simple(pci_bus, -1, "BCM471A-usb-uhci");
            pci_create_simple(pci_bus, -1, "BCM472A-usb-xhci");
        }
        pci_bus = (PCIBus *)qdev_get_child_bus(dev, "pci");
        if (!pci_bus) {
           fprintf(stderr, "couldn't create PCI controller!\n");
           exit(1);
       }
       if (pci_bus) {
           /* Register network interfaces. */
/* PCIDevice *pci_nic_init_nofail(NICInfo *nd, PCIBus *rootbus,
                               const char *default_model,
                               const char *default_devaddr) */
               pci_nic_init_nofail(&nd_table[5], pci_bus, "ne2k_pci", NULL);  //NULL ----default devaddr TODO
               pci_nic_init_nofail(&nd_table[6], pci_bus, "ne2k_pci", NULL);
       }	

        dev = qdev_create(NULL, "realview_pci");
        busdev = SYS_BUS_DEVICE(dev);
        qdev_init_nofail(dev);
        sysbus_mmio_map(busdev, 0, 0x18014000); /* PCIE2 controller registers */\
	sysbus_mmio_map(busdev, 2, 0x18014400); /* PCIE config */
        sysbus_mmio_map(busdev, 6, 0x48000000); /* PCIE memory window 3, Port 2 */




#if 1
   //     sysbus_mmio_map(busdev, 1, 0x60000000); /* PCIE self-config */
        
//sysbus_mmio_map(busdev, 2, 0x18013400); /* PCIE config */
//sysbus_mmio_map(busdev, 2, 0x18014400); /* PCIE config */
    //    sysbus_mmio_map(busdev, 3, 0x62000000); /* PCIE I/O */


 //       sysbus_connect_irq(busdev, 0, pic[47]);
 //       sysbus_connect_irq(busdev, 1, pic[49]);
 //       sysbus_connect_irq(busdev, 2, pic[50]);
  //      sysbus_connect_irq(busdev, 3, pic[51]);

 //       n = drive_get_max_bus(IF_SCSI);
  //      while (n >= 0) {
   //         pci_create_simple(pci_bus, -1, "lsi53c895a");
     //       n--;
   //     }

#endif
#if 0
sysbus_create_simple("nec-usb-xhci", 0x18021000, pic[78]);
sysbus_create_simple("-usb-xhci", 0x18022000, pic[79]);
sysbus_create_simple("BCM472A-usb-xhci", 0x18023000, pic[80]);
#endif

#if 1
    if (nd_table[0].used) {
printf("nd tablejasdfasdf  \n");
        qemu_check_nic_model(&nd_table[0], "xgmac");
        dev = qdev_create(NULL, "xgmac");
        qdev_set_nic_properties(dev, &nd_table[0]);
        qdev_init_nofail(dev);
        sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x18024000);
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, pic[147]);
//        sysbus_connect_irq(SYS_BUS_DEVICE(dev), 1, pic[78]);
//        sysbus_connect_irq(SYS_BUS_DEVICE(dev), 2, pic[79]);

        qemu_check_nic_model(&nd_table[1], "xgmac");
        dev = qdev_create(NULL, "xgmac");
        qdev_set_nic_properties(dev, &nd_table[1]);
        qdev_init_nofail(dev);
        sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x18025000);
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, pic[148]);
//        sysbus_connect_irq(SYS_BUS_DEVICE(dev), 1, pic[81]);
 //       sysbus_connect_irq(SYS_BUS_DEVICE(dev), 2, pic[82]);

        qemu_check_nic_model(&nd_table[2], "xgmac");
        dev = qdev_create(NULL, "xgmac");
        qdev_set_nic_properties(dev, &nd_table[2]);
        qdev_init_nofail(dev);
        sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x18026000);
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, pic[149]);

        qemu_check_nic_model(&nd_table[3], "xgmac");
        dev = qdev_create(NULL, "xgmac");
        qdev_set_nic_properties(dev, &nd_table[3]);
        qdev_init_nofail(dev);
        sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x18027000);
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, pic[150]);

    }

#endif
    /* Memory map for Broadcom Northstar Cortex A9:  */
    /* 0x18000000 		SOC_CHIPCOMON_A_BASE_PA  */
    /* 0x18001000   		SOC_CHIPCOMON_B_BASE_PA  */
    /* 0x1800c000-0x1800cfff	SOC_DMU_BASE_PA  Power, Clock controls */
    /* 0x10002000 		Two-Wire Serial Bus.  */
    /* 0x18008000-0x18008007	UART0. ChipcomonB UART0 */
    /* 0x18000300-0x18000307 	UART1. ChipcomonA UART1 */
    /* 0x18000400-0x18000407	UART2. ChipcomonA UART2 */
    /* 0x19000000-0x19000fff 	cru_regs */

    /* 0x19022000 L2CC_BASE_PA */
//#define	MPCORE_SCU_OFF		0x0000	/* Coherency controller */
//#define	MPCORE_GIC_CPUIF_OFF	0x0100	/* Interrupt controller CPU interface */
//#define	MPCORE_GTIMER_OFF	0x0200	/* Global timer */
//#define	MPCORE_LTIMER_OFF	0x0600	/* Local (private) timers */
//#define	MPCORE_GIC_DIST_OFF	0x1000	/* Interrupt distributor registers */
/*
	aix@18000000 {
		compatible = "brcm,bus-aix";
		reg = <0x18000000 0x1000>;
		ranges = <0x00000000 0x18000000 0x00100000>;
		#address-cells = <1>;
		#size-cells = <1>;
		sprom = <&sprom0>;

		usb2@0 {
			reg = <0x18021000 0x1000>;
			interrupts = <GIC_SPI 79 IRQ_TYPE_LEVEL_HIGH>;
		};

		usb3@0 {
			reg = <0x18023000 0x1000>;
			interrupts = <GIC_SPI 80 IRQ_TYPE_LEVEL_HIGH>;
		};

		gmac@0 {
			reg = <0x18024000 0x1000>;
			interrupts = <GIC_SPI 147 IRQ_TYPE_LEVEL_HIGH>;
		};

		gmac@1 {
			reg = <0x18025000 0x1000>;
			interrupts = <GIC_SPI 148 IRQ_TYPE_LEVEL_HIGH>;
		};

		gmac@2 {
			reg = <0x18026000 0x1000>;
			interrupts = <GIC_SPI 149 IRQ_TYPE_LEVEL_HIGH>;
		};

		gmac@3 {
			reg = <0x18027000 0x1000>;
			interrupts = <GIC_SPI 150 IRQ_TYPE_LEVEL_HIGH>;
		};

		pcie@0 {
			reg = <0x18012000 0x1000>;
			interrupts = <GIC_SPI 131 IRQ_TYPE_LEVEL_HIGH>;
		};

		pcie@1 {
			reg = <0x18013000 0x1000>;
			interrupts = <GIC_SPI 137 IRQ_TYPE_LEVEL_HIGH>;
		};
	};
*/
    /*  0x1000d000 SSPI.  */
    /*  0x1000e000 SCI.  */
    /* 0x1000f000 Reserved.  */
    /*  0x10010000 Watchdog.  */
    /* 0x10011000 Timer 0+1.  */
    /* 0x10012000 Timer 2+3.  */
    /*  0x10013000 GPIO 0.  */
    /*  0x10014000 GPIO 1.  */
    /*  0x10015000 GPIO 2.  */
    /*  0x10002000 Two-Wire Serial Bus - DVI. (PB) */
    /* 0x10017000 RTC.  */
    /*  0x10018000 DMC.  */
    /*  0x10019000 PCI controller config.  */
    /*  0x10020000 CLCD.  */
    /* 0x10030000 DMA Controller.  */
    /*  0x10080000 SMC.  */
    /* 0x18000000 GIC1. (PB) */
    /*  0x1e001000 GIC2. (PB) */
    /*  0x1e002000 GIC3. (PB) */
    /*  0x1e003000 GIC4. (PB) */

    /*  0x40000000 NOR flash.  */
    /*  0x44000000 DoC flash.  */
    /*  0x48000000 SRAM.  */
    /*  0x4c000000 Configuration flash.  */
    /* 0x4e000000 Ethernet.  */
    /*  0x4f000000 USB.  */
    /* 0x18012000-0x18012fff PCIE 0  */
    /* 0x18013000-0x18013fff PCIE 1  */
    /* 0x18014000-0x18014fff PCIE2   */

    /* 0x60000000 PCI Self Config.  */
    /* 0x61000000 PCI Config.  */
    /* 0x62000000 PCI IO.  */
    /* 0x08000000-0x0fffffff PCI mem 0.  */
    /* 0x40000000-0x47ffffff PCI mem 1.  */
    /* 0x48000000-0x4fffffff PCI mem 2.  */

    /* ??? Hack to map an additional page of ram for the secondary CPU
       startup code.  I guess this works on real hardware because the
       BootROM happens to be in ROM/flash or in memory that isn't clobbered
       until after Linux boots the secondary CPUs.  */
    memory_region_init_ram(ram_hack, NULL, "SMPBOOT.hack", 0x30000);
    vmstate_register_ram_global(ram_hack);
    memory_region_add_subregion(sysmem, 0xfffd0000/*SMP_BOOT_ADDR*/, ram_hack);

    broadcom_binfo.ram_size = ram_size;
    broadcom_binfo.kernel_filename = args->kernel_filename;
    broadcom_binfo.kernel_cmdline = args->kernel_cmdline;
    broadcom_binfo.initrd_filename = args->initrd_filename;
    broadcom_binfo.nb_cpus = smp_cpus;
    broadcom_binfo.board_id = broadcom_board_id[board_type];
    broadcom_binfo.loader_start = broadcom_board_zreladdr[board_type];
    broadcom_binfo.smp_loader_start = SMP_BOOT_ADDR;
    broadcom_binfo.smp_bootreg_addr = SMP_BOOTREG_ADDR;
    /* Both A9 and 11MPCore put the GIC CPU i/f at base + 0x100 */
    broadcom_binfo.gic_cpu_if_addr = MPCORE_PERIPHBASE + 0x100;
    arm_load_kernel(ARM_CPU(first_cpu), &broadcom_binfo);
}

static void broadcom_brcm_gen_init(QEMUMachineInitArgs *args)
{
    if (!args->cpu_model) {
        args->cpu_model = "cortex-a9";
    }
    broadcom_init(args, BRCM_GEN);
}

static void broadcom_brcm_ns_qt_init(QEMUMachineInitArgs *args)
{
    if (!args->cpu_model) {
        args->cpu_model = "cortex-a9";
    }
    broadcom_init(args, BRCM_NS_QT);
}

static void broadcom_brcm_ns_init(QEMUMachineInitArgs *args)
{
    if (!args->cpu_model) {
        args->cpu_model = "cortex-a9";
    }
    broadcom_init(args, BRCM_NS);
}

static QEMUMachine broadcom_brcm_gen_machine = {
    .name = "brcm-gen",
    .desc = "ARM Broadcom Northstar Generic Platform for Cortex-A9",
    .init = broadcom_brcm_gen_init,
    .block_default_type = IF_SCSI,
    .max_cpus = 4,
};
static QEMUMachine broadcom_brcm_ns_qt_machine = {
    .name = "brcm-ns-qt",
    .desc = "ARM Broadcom Northstar Emulation - Cortex-A9",
    .init = broadcom_brcm_ns_qt_init,
    .block_default_type = IF_SCSI,
    .max_cpus = 4,
};

static QEMUMachine broadcom_brcm_ns_machine = {
    .name = "brcm-ns",
    .desc = "ARM Broadcom Northstart Prototype - Cortex-A9",
    .init = broadcom_brcm_ns_init,
    .block_default_type = IF_SCSI,
    .max_cpus = 4,
};

static void broadcom_machine_init(void)
{
    qemu_register_machine(&broadcom_brcm_gen_machine);
    qemu_register_machine(&broadcom_brcm_ns_qt_machine);
    qemu_register_machine(&broadcom_brcm_ns_machine);
}

machine_init(broadcom_machine_init);
