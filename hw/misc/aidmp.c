/*
 * Broadcom SiliconBackplane hardware register definitions.
 * SiliconBackplane Chipcommon core hardware definitions.
 *
 * Copyright (c) 2014 Just Playing Around.
 * Written by Juan F. Loya, MD
 *
 * This code is licensed under the GPL.
 */

#include "hw/hw.h"
#include "qemu/timer.h"
#include "qemu/bitops.h"
#include "hw/sysbus.h"
#include "sysemu/sysemu.h"
#include "aidmp.h"


#define TYPE_AIDMPREGS "aidmpregs"
#define AIDMPREGS(obj) \
    OBJECT_CHECK(AIdmpregs_State, (obj), TYPE_AIDMPREGS)

#define AIDMPREGS_ERR_DEBUG
#ifdef AIDMPREGS_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif
#define NFLASH_SUPPORT

/* core codes */
#define	NODEV_CORE_ID		0x700		/* Invalid coreid */
#define	CC_CORE_ID		0x800		/* chipcommon core */
#define	ILINE20_CORE_ID		0x801		/* iline20 core */
#define	SRAM_CORE_ID		0x802		/* sram core */
#define	SDRAM_CORE_ID		0x803		/* sdram core */
#define	PCI_CORE_ID		0x804		/* pci core */
#define	MIPS_CORE_ID		0x805		/* mips core */
#define	ENET_CORE_ID		0x806		/* enet mac core */
#define	CODEC_CORE_ID		0x807		/* v90 codec core */
#define	USB_CORE_ID		0x808		/* usb 1.1 host/device core */
#define	ADSL_CORE_ID		0x809		/* ADSL core */
#define	ILINE100_CORE_ID	0x80a		/* iline100 core */
#define	IPSEC_CORE_ID		0x80b		/* ipsec core */
#define	UTOPIA_CORE_ID		0x80c		/* utopia core */
#define	PCMCIA_CORE_ID		0x80d		/* pcmcia core */
#define	SOCRAM_CORE_ID		0x80e		/* internal memory core */
#define	MEMC_CORE_ID		0x80f		/* memc sdram core */
#define	OFDM_CORE_ID		0x810		/* OFDM phy core */
#define	EXTIF_CORE_ID		0x811		/* external interface core */
#define	D11_CORE_ID		0x812		/* 802.11 MAC core */
#define	APHY_CORE_ID		0x813		/* 802.11a phy core */
#define	BPHY_CORE_ID		0x814		/* 802.11b phy core */
#define	GPHY_CORE_ID		0x815		/* 802.11g phy core */
#define	MIPS33_CORE_ID		0x816		/* mips3302 core */
#define	USB11H_CORE_ID		0x817		/* usb 1.1 host core */
#define	USB11D_CORE_ID		0x818		/* usb 1.1 device core */
#define	USB20H_CORE_ID		0x819		/* usb 2.0 host core */
#define	USB20D_CORE_ID		0x81a		/* usb 2.0 device core */
#define	SDIOH_CORE_ID		0x81b		/* sdio host core */
#define	ROBO_CORE_ID		0x81c		/* roboswitch core */
#define	ATA100_CORE_ID		0x81d		/* parallel ATA core */
#define	SATAXOR_CORE_ID		0x81e		/* serial ATA & XOR DMA core */
#define	GIGETH_CORE_ID		0x81f		/* gigabit ethernet core */
#define	PCIE_CORE_ID		0x820		/* pci express core */
#define	NPHY_CORE_ID		0x821		/* 802.11n 2x2 phy core */
#define	SRAMC_CORE_ID		0x822		/* SRAM controller core */
#define	MINIMAC_CORE_ID		0x823		/* MINI MAC/phy core */
#define	ARM11_CORE_ID		0x824		/* ARM 1176 core */
#define	ARM7S_CORE_ID		0x825		/* ARM7tdmi-s core */
#define	LPPHY_CORE_ID		0x826		/* 802.11a/b/g phy core */
#define	PMU_CORE_ID		0x827		/* PMU core */
#define	SSNPHY_CORE_ID		0x828		/* 802.11n single-stream phy core */
#define	SDIOD_CORE_ID		0x829		/* SDIO device core */
#define	ARMCM3_CORE_ID		0x82a		/* ARM Cortex M3 core */
#define	HTPHY_CORE_ID		0x82b		/* 802.11n 4x4 phy core */
#define	MIPS74K_CORE_ID		0x82c		/* mips 74k core */
#define	GMAC_CORE_ID		0x82d		/* Gigabit MAC core */
#define	DMEMC_CORE_ID		0x82e		/* DDR1/2 memory controller core */
#define	PCIERC_CORE_ID		0x82f		/* PCIE Root Complex core */
#define	OCP_CORE_ID		0x830		/* OCP2OCP bridge core */
#define	SC_CORE_ID		0x831		/* shared common core */
#define	AHB_CORE_ID		0x832		/* OCP2AHB bridge core */
#define	SPIH_CORE_ID		0x833		/* SPI host core */
#define	I2S_CORE_ID		0x834		/* I2S core */
#define	DMEMS_CORE_ID		0x835		/* SDR/DDR1 memory controller core */
#define	DEF_SHIM_COMP		0x837		/* SHIM component in ubus/6362 */

#define ACPHY_CORE_ID		0x83b		/* Dot11 ACPHY */
#define PCIE2_CORE_ID		0x83c		/* pci express Gen2 core */
#define USB30D_CORE_ID		0x83d		/* usb 3.0 device core */
#define ARMCR4_CORE_ID		0x83e		/* ARM CR4 CPU */
#define APB_BRIDGE_CORE_ID	0x135		/* APB bridge core ID */
#define AXI_CORE_ID		0x301		/* AXI/GPV core ID */
#define EROM_CORE_ID		0x366		/* EROM core ID */
#define OOB_ROUTER_CORE_ID	0x367		/* OOB router core ID */
#define DEF_AI_COMP		0xfff		/* Default component, in ai chips it maps all
						 * unused address ranges
						 */

#define CC_4706_CORE_ID		0x500		/* chipcommon core */
#define NS_PCIEG2_CORE_ID	0x501		/* PCIE Gen 2 core */
#define NS_DMA_CORE_ID		0x502		/* DMA core */
#define NS_SDIO3_CORE_ID	0x503		/* SDIO3 core */
#define NS_USB20_CORE_ID	0x504		/* USB2.0 core */
#define NS_USB30_CORE_ID	0x505		/* USB3.0 core */
#define NS_A9JTAG_CORE_ID	0x506		/* ARM Cortex A9 JTAG core */
#define NS_DDR23_CORE_ID	0x507		/* Denali DDR2/DDR3 memory controller */
#define NS_ROM_CORE_ID		0x508		/* ROM core */
#define NS_NAND_CORE_ID		0x509		/* NAND flash controller core */
#define NS_QSPI_CORE_ID		0x50a		/* SPI flash controller core */
#define NS_CCB_CORE_ID		0x50b		/* ChipcommonB core */
#define SOCRAM_4706_CORE_ID	0x50e		/* internal memory core */
#define NS_SOCRAM_CORE_ID	SOCRAM_4706_CORE_ID
#define	ARMCA9_CORE_ID		0x510		/* ARM Cortex A9 core (ihost) */
#define	NS_IHOST_CORE_ID	ARMCA9_CORE_ID	/* ARM Cortex A9 core (ihost) */
#define GMAC_COMMON_4706_CORE_ID	0x5dc		/* Gigabit MAC core */
#define GMAC_4706_CORE_ID	0x52d		/* Gigabit MAC core */
#define AMEMC_CORE_ID		0x52e		/* DDR1/2 memory controller core */
#define ALTA_CORE_ID		0x534		/* I2S core */
#define DDR23_PHY_CORE_ID	0x5dd

typedef struct AIdmpregs_State {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    uint32_t core_id;
    union {
	struct {
	    struct _aidmp aidmpregs;

    };
    uint8_t data[0x1000];
    };
} AIdmpregs_State;


static void aidmpregs_reset(DeviceState *d)
{
    AIdmpregs_State *s = AIDMPREGS(d);
	s->aidmpregs.resetctrl=0;
	s->aidmpregs.iostatus=0x1;	//s->aidmpregs.ioctrl=0x1000000;
	s->aidmpregs.oobselouta30=0x3bfdf;  //TODO fix for PCIE Gen 2 core
    DB_PRINT("****CORE 0x%x RESET****\n",s->core_id);

}

static uint64_t aidmpregs_read_imp(void *opaque, hwaddr offset, unsigned size)
{
    AIdmpregs_State *s = (AIdmpregs_State *)opaque;

    switch (offset) {
	/* DMP wrapper registers */
	case AI_OOBSELINA30: /* 0x000 */
	case AI_OOBSELINA74: /* 0x004 */
	case AI_OOBSELINB30: /* 0x020 */
	case AI_OOBSELINB74: /* 0x024 */
	case AI_OOBSELINC30: /* 0x040 */
	case AI_OOBSELINC74: /* 0x044 */
	case AI_OOBSELIND30: /* 0x060 */
	case AI_OOBSELIND74: /* 0x064 */
	case AI_OOBSELOUTA30: /* 0x100 */
	    return s->aidmpregs.oobselouta30;
	case AI_OOBSELOUTA74: /* 0x104 */
	case AI_OOBSELOUTB30: /* 0x120 */
	case AI_OOBSELOUTB74: /* 0x124 */
	case AI_OOBSELOUTC30: /* 0x140 */
	case AI_OOBSELOUTC74: /* 0x144 */
	case AI_OOBSELOUTD30: /* 0x160 */
	case AI_OOBSELOUTD74: /* 0x164 */
	case AI_OOBSYNCA: /* 0x200 */
	case AI_OOBSELOUTAEN: /* 0x204 */
	case AI_OOBSYNCB: /* 0x220 */ 
	case AI_OOBSELOUTBEN: /* 0x224 */
	case AI_OOBSYNCC: /* 0x240 */
	case AI_OOBSELOUTCEN: /* 0x244 */
	case AI_OOBSYNCD: /* 0x260 */
	case AI_OOBSELOUTDEN: /* 0x264 */
	case AI_OOBAEXTWIDTH: /* 0x300 */
	case AI_OOBAINWIDTH: /* 0x304 */
	case AI_OOBAOUTWIDTH: /* 0x308 */
	case AI_OOBBEXTWIDTH: /* 0x320 */
	case AI_OOBBINWIDTH: /* 0x324 */
	case AI_OOBBOUTWIDTH: /* 0x328 */
	case AI_OOBCEXTWIDTH: /* 0x340 */
	case AI_OOBCINWIDTH: /* 0x344 */
	case AI_OOBCOUTWIDTH: /* 0x348 */
	case AI_OOBDEXTWIDTH: /* 0x360 */
	case AI_OOBDINWIDTH: /* 0x364 */
	case AI_OOBDOUTWIDTH: /* 0x368 */
	case AI_IOCTRLSET: /* 400 */
	case AI_IOCTRLCLEAR: /* 404 */
	case AI_IOCTRL: /* 408 */
	    return s->aidmpregs.ioctrl;
	case AI_IOSTATUS: /* 500 */
	    return s->aidmpregs.iostatus;

	case AI_IOCTRLWIDTH: /* 700 */
	case AI_IOSTATUSWIDTH: /* 0x704 */
	case AI_RESETCTRL: /* 800 */
	    return s->aidmpregs.resetctrl;
	case AI_RESETSTATUS: /* 804 */
	    return s->aidmpregs.resetstatus;
	case AI_RESETREADID: /* 808 */
	case AI_RESETWRITEID: /* 80c */
//	case AI_ERRLOGCTRL: /* a00 */
	case AI_ERRLOGDONE: /* a04 */
	case AI_ERRLOGSTATUS: /* a08 */
	case AI_ERRLOGADDRLO: /* a0c */
	case AI_ERRLOGADDRHI: /* a10 */
	case AI_ERRLOGID: /* a14 */
	case AI_ERRLOGUSER: /* a18 */
	case AI_ERRLOGFLAGS: /* a1c */
	case AI_INTSTATUS: /* a00 */
	case AI_CONFIG: /* e00 */
	case AI_ITCR	: /* f00 */
	case AI_ITIPOOBA: /* f10 */
	case AI_ITIPOOBB: /* f14 */
	case AI_ITIPOOBC: /* f18 */
	case AI_ITIPOOBD: /* f1c */
	case AI_ITIPOOBAOUT: /* f30 */
	case AI_ITIPOOBBOUT: /* f34 */
	case AI_ITIPOOBCOUT: /* f38 */
	case AI_ITIPOOBDOUT: /* f3c */
	case AI_ITOPOOBA: /* f50 */
	case AI_ITOPOOBB: /* f54 */
	case AI_ITOPOOBC: /* f58 */
	case AI_ITOPOOBD: /* f5c */
	case AI_ITOPOOBAIN: /* f70 */
	case AI_ITOPOOBBIN: /* f74 */
	case AI_ITOPOOBCIN: /* f78 */
	case AI_ITOPOOBDIN: /* f7c */
	case AI_ITOPRESET: /* f90 */
	case AI_PERIPHERIALID4: /* 0xfd0 */
	case AI_PERIPHERIALID5: /* 0xfd4 */
	case AI_PERIPHERIALID6:	/* 0xfd8 */
	case AI_PERIPHERIALID7: /* 0xfdc */
	case AI_PERIPHERIALID0: /* 0xfe0 */
	case AI_PERIPHERIALID1: /* 0xfe4 */
	case AI_PERIPHERIALID2: /* 0xfe8 */
	case AI_PERIPHERIALID3: /* 0xfec */
	case AI_COMPONENTID0: /* ff0 */
	case AI_COMPONENTID1: /* ff4 */
	case AI_COMPONENTID2: /* ff8 */
	case AI_COMPONENTID3: /* ffc */
    default:
 //   bad_reg:
	DB_PRINT("Bad register offset 0x%x\n", (int)offset);
        return 0;
    }
}


static uint64_t aidmpregs_read_(void *opaque, hwaddr offset, unsigned size)
{
    AIdmpregs_State *s = (AIdmpregs_State *)opaque;
    uint32_t ret = aidmpregs_read_imp(opaque, offset, size);

    DB_PRINT("core_id: 0x%x offset: %08x data: %08x\n", (unsigned)s->core_id, (unsigned)offset, (unsigned)ret);
    return ret;
}

static void aidmpregs_write(void *opaque, hwaddr offset,
                             uint64_t val, unsigned size)
{
    AIdmpregs_State *s = (AIdmpregs_State *)opaque;

    DB_PRINT("core_id: 0x%x offset: %08x data: %08x\n",(unsigned)s->core_id, (unsigned)offset, (unsigned)val);

    switch (offset) {
	/* DMP wrapper registers */
	case AI_OOBSELINA30: /* 0x000 */
	case AI_OOBSELINA74: /* 0x004 */
	case AI_OOBSELINB30: /* 0x020 */
	case AI_OOBSELINB74: /* 0x024 */
	case AI_OOBSELINC30: /* 0x040 */
	case AI_OOBSELINC74: /* 0x044 */
	case AI_OOBSELIND30: /* 0x060 */
	case AI_OOBSELIND74: /* 0x064 */
	case AI_OOBSELOUTA30: /* 0x100 */
	    s->aidmpregs.oobselouta30=val;
	    break;
	case AI_OOBSELOUTA74: /* 0x104 */
	case AI_OOBSELOUTB30: /* 0x120 */
	case AI_OOBSELOUTB74: /* 0x124 */
	case AI_OOBSELOUTC30: /* 0x140 */
	case AI_OOBSELOUTC74: /* 0x144 */
	case AI_OOBSELOUTD30: /* 0x160 */
	case AI_OOBSELOUTD74: /* 0x164 */
	case AI_OOBSYNCA: /* 0x200 */
	case AI_OOBSELOUTAEN: /* 0x204 */
	case AI_OOBSYNCB: /* 0x220 */ 
	case AI_OOBSELOUTBEN: /* 0x224 */
	case AI_OOBSYNCC: /* 0x240 */
	case AI_OOBSELOUTCEN: /* 0x244 */
	case AI_OOBSYNCD: /* 0x260 */
	case AI_OOBSELOUTDEN: /* 0x264 */
	case AI_OOBAEXTWIDTH: /* 0x300 */
	case AI_OOBAINWIDTH: /* 0x304 */
	case AI_OOBAOUTWIDTH: /* 0x308 */
	case AI_OOBBEXTWIDTH: /* 0x320 */
	case AI_OOBBINWIDTH: /* 0x324 */
	case AI_OOBBOUTWIDTH: /* 0x328 */
	case AI_OOBCEXTWIDTH: /* 0x340 */
	case AI_OOBCINWIDTH: /* 0x344 */
	case AI_OOBCOUTWIDTH: /* 0x348 */
	case AI_OOBDEXTWIDTH: /* 0x360 */
	case AI_OOBDINWIDTH: /* 0x364 */
	case AI_OOBDOUTWIDTH: /* 0x368 */
	case AI_IOCTRLSET: /* 400 */
	case AI_IOCTRLCLEAR: /* 404 */
	case AI_IOCTRL: /* 408 */
	    s->aidmpregs.ioctrl=val;
	    break;
	case AI_IOSTATUS: /* 500 */
	    s->aidmpregs.iostatus=val;
	    break;
	case AI_RESETCTRL: /* 800 */
	    s->aidmpregs.resetctrl=val;
	    break;
	case AI_RESETSTATUS: /* 804 */
	    s->aidmpregs.resetstatus=val;
	    break;
	case AI_IOCTRLWIDTH: /* 700 */
	case AI_IOSTATUSWIDTH: /* 0x704 */

	case AI_RESETREADID: /* 808 */
	case AI_RESETWRITEID: /* 80c */
//	case AI_ERRLOGCTRL: /* a00 */
	case AI_ERRLOGDONE: /* a04 */
	case AI_ERRLOGSTATUS: /* a08 */
	case AI_ERRLOGADDRLO: /* a0c */
	case AI_ERRLOGADDRHI: /* a10 */
	case AI_ERRLOGID: /* a14 */
	case AI_ERRLOGUSER: /* a18 */
	case AI_ERRLOGFLAGS: /* a1c */
	case AI_INTSTATUS: /* a00 */
	case AI_CONFIG: /* e00 */
	case AI_ITCR	: /* f00 */
	case AI_ITIPOOBA: /* f10 */
	case AI_ITIPOOBB: /* f14 */
	case AI_ITIPOOBC: /* f18 */
	case AI_ITIPOOBD: /* f1c */
	case AI_ITIPOOBAOUT: /* f30 */
	case AI_ITIPOOBBOUT: /* f34 */
	case AI_ITIPOOBCOUT: /* f38 */
	case AI_ITIPOOBDOUT: /* f3c */
	case AI_ITOPOOBA: /* f50 */
	case AI_ITOPOOBB: /* f54 */
	case AI_ITOPOOBC: /* f58 */
	case AI_ITOPOOBD: /* f5c */
	case AI_ITOPOOBAIN: /* f70 */
	case AI_ITOPOOBBIN: /* f74 */
	case AI_ITOPOOBCIN: /* f78 */
	case AI_ITOPOOBDIN: /* f7c */
	case AI_ITOPRESET: /* f90 */
	case AI_PERIPHERIALID4: /* 0xfd0 */
	case AI_PERIPHERIALID5: /* 0xfd4 */
	case AI_PERIPHERIALID6:	/* 0xfd8 */
	case AI_PERIPHERIALID7: /* 0xfdc */
	case AI_PERIPHERIALID0: /* 0xfe0 */
	case AI_PERIPHERIALID1: /* 0xfe4 */
	case AI_PERIPHERIALID2: /* 0xfe8 */
	case AI_PERIPHERIALID3: /* 0xfec */
	case AI_COMPONENTID0: /* ff0 */
	case AI_COMPONENTID1: /* ff4 */
	case AI_COMPONENTID2: /* ff8 */
	case AI_COMPONENTID3: /* ffc */
    default:
 //   bad_reg:
            DB_PRINT("Bad register write %x <= %08x\n", (int)offset,
                     (unsigned)val);
        return;
    }
}


static const MemoryRegionOps aidmpregs_ops = {
    .read = aidmpregs_read_,
    .write = aidmpregs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


static void aidmpregs_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sd = SYS_BUS_DEVICE(obj);
    AIdmpregs_State *s = AIDMPREGS(obj);

    memory_region_init_io(&s->iomem, OBJECT(dev), &aidmpregs_ops, s,
                          "aidmpregs", 0x1000);
    sysbus_init_mmio(sd, &s->iomem);

}


static Property aidmpregs_properties[] = {
    DEFINE_PROP_UINT32("core_id", AIdmpregs_State, core_id, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_aidmpregs = {
    .name = "aidmpregs",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(data, AIdmpregs_State, 0x1000),
        VMSTATE_END_OF_LIST()
    }
};
static void aidmpregs_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

 //   dc->realize = chipcregs_realize;
    dc->reset = aidmpregs_reset;
    dc->vmsd = &vmstate_aidmpregs;
    dc->props = aidmpregs_properties;
}

static const TypeInfo aidmpregs_info = {
    .name          = TYPE_AIDMPREGS,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AIdmpregs_State),
    .instance_init = aidmpregs_init,
//    .instance_finalize = chipcregs_finalize,
    .class_init    = aidmpregs_class_init,
};

static void aidmpregs_register_types(void)
{
    type_register_static(&aidmpregs_info);
}

type_init(aidmpregs_register_types)
