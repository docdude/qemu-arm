/*
 * Broadcom SiliconBackplane hardware register definitions.
 * PCIE Gen2 core hardware definitions.
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
#include "pcieg2_core.h"


#define TYPE_PCIEG2REGS "pcieg2regs"
#define PCIEG2REGS(obj) \
    OBJECT_CHECK(PCIEG2regs_State, (obj), TYPE_PCIEG2REGS)

#define PCIEG2REGS_ERR_DEBUG
#ifdef PCIEG2REGS_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif

typedef struct PCIEG2regs_State {
    SysBusDevice parent_obj;

    MemoryRegion iomem;

    union {
	struct {
	    struct nspcieregs nspcieregss;

    };
    uint8_t data[0x1000];
    };
} PCIEG2regs_State;


static void pcieg2regs_reset(DeviceState *d)
{
    PCIEG2regs_State *s = PCIEG2REGS(d);
	s->pcieg2regs.resetctrl=0;
	s->pcieg2regs.iostatus=0x1;	
	s->pcieg2regs.oobselouta30=0;
    DB_PRINT("****RESET****\n");

}

static uint64_t pcieg2regs_read_imp(void *opaque, hwaddr offset, unsigned size)
{
    PCIEG2regs_State *s = (PCIEG2regs_State *)opaque;

    switch (offset) {
	case 0x000: /*clk_control*/
	case 0x004: /*rc_pm_control*/	
	case 0x008: /*rc_pm_status */	
	case 0x00C: /*ep_pm_control */	
	case 0x010: /*ep_pm_status */	
	case 0x014: /* ep_ltr_control */	
	case 0x018: /* ep_ltr_status */	
	case 0x01C: /* ep_obff_status */	
	case 0x020: /* pcie_err_status */	
	/* PAD[55] */
	case 0x100: /* rc_axi_config */	
	case 0x104: /* ep_axi_config */	
	case 0x108: /* rxdebug_status0 */	
	case 0x10C: /* rxdebug_control0 */	
	/* PAD[4] */

	/* pcie core supports in direct access to config space */
	case 0x120: /* configindaddr */	/* pcie config space access: Address field */
	case 0x124: /* configinddata */	/* pcie config space access: Data field:  */

	/* PAD[52] */
	case 0x1F8:  /* cfg_addr 		bus, dev, func, reg */
	case 0x1FC: /* cfg_data */		
	 case 0x200:   /* sys_eq_page     memory page for event queues: */
	case 0x204: /* sys_msi_page */	
	case 0x208: /* sys_msi_intren */	
	/* PAD[1] */
	case 0x210: /* sys_msi_ctrl0 */	
	case 0x214: /* sys_msi_ctrl1 */	
	case 0x218: /* sys_msi_ctrl2 */	
	case 0x21C: /* sys_msi_ctrl3 */	
	case 0x220: /* sys_msi_ctrl4 */	
	case 0x224: /* sys_msi_ctrl5 */	
	/* PAD[10] */
	case 0x250: /* sys_eq_head0 */	
	case 0x254: /* sys_eq_tail0 */	
	case 0x258: /* sys_eq_head1 */	
	case 0x25C: /* sys_eq_tail1 */	
	case 0x260: /* sys_eq_head2 */	
	case 0x264: /* sys_eq_tail2 */	 
	case 0x268: /* sys_eq_head3 */	
	case 0x26C: /* sys_eq_tail3 */	
	case 0x270: /* sys_eq_head4 */	
	case 0x274: /* sys_eq_tail4 */	
	case 0x278: /* sys_eq_head5 */	
	case 0x27C: /* sys_eq_tail5 */	
	/* PAD[44] */
	case 0x330: /* sys_rc_intx_en */	
	case 0x334: /* sys_rc_intx_csr */	
	/* PAD[2] */
	case 0x340: /* sys_msi_req */	
	case 0x344: /* sys_host_intr_en */	
	case 0x348: /* sys_host_intr_csr */	
	/* PAD[1] */
	case 0x350: /* sys_host_intr0 */	
	case 0x354: /* sys_host_intr1 */	
	case 0x358: /* sys_host_intr2 */	
	case 0x35C: /* sys_host_intr3 */	
	case 0x360: /* sys_ep_int_en0 */	
	case 0x364: /* sys_ep_int_en1 */	
	/* PAD[2] */
	case 0x370: /* sys_ep_int_csr0 */	
	case 0x374: /* sys_ep_int_csr1 */	
	/* PAD[34] */
	case 0x400 - 0x7FF: /* pciecfg[4][64] 	  PCIE Cfg Space */
	case 0x800: /* sprom[64]  SPROM shadow Area */
	/* PAD[224] */
	case 0xC00: /* func0_imap0_0 */	
	case 0xC04: /* func0_imap0_1 */	
	case 0xC08: /* func0_imap0_2 */	
	case 0xC0C: /* func0_imap0_3 */	
	case 0xC10: /* func0_imap0_4 */	
	case 0xC14: /* func0_imap0_5 */	
	case 0xC18: /* func0_imap0_6 */	
	case 0xC1C: /* func0_imap0_7 */	
	case 0xC20: /* func1_imap0_0 */	
	case 0xC24: /* func1_imap0_1 */	
	case 0xC28: /* func1_imap0_2 */	
	case 0xC2C: /* func1_imap0_3 */	
	case 0xC30: /* func1_imap0_4 */	
	case 0xC34: /* func1_imap0_5 */	
	case 0xC38: /* func1_imap0_6 */	
	case 0xC3C: /* func1_imap0_7 */	
	/* PAD[16] */
	case 0xC80: /* func0_imap1 */	
	/* PAD[1] */
	case 0xC88: /* func1_imap1 */	
	/* PAD[13] */
	case 0xCC0: /* func0_imap2 */	
	/* PAD[1] */
	case 0xCC8: /* func1_imap2 */	
	/* PAD[13] */
	case 0xD00: /* iarr0_lower */	
	case 0xD04: /* iarr0_upper */	
	case 0xD08: /* iarr1_lower */	
	case 0xD0C: /* iarr1_upper */	
	case 0xD10: /* iarr2_lower */	
	case 0xD14: /* iarr2_upper */	
	/* PAD[2] */
	case 0xD20: /* oarr0 */		
	/* PAD[1] */
	case 0xD28: /* oarr1 */		
	/* PAD[1] */
	case 0xD30: /* oarr2 */		
	/* PAD[3] */
	case 0xD40: /* omap0_lower */	
	case 0xD44: /* omap0_upper */	
	case 0xD48: /* omap1_lower */	
	case 0xD4C: /* omap1_upper */	
	case 0xD50: /* omap2_lower */	
	case 0xD54: /* omap2_upper */	
	case 0xD58: /* func1_iarr1_size */	
	case 0xD5C: /* func1_iarr2_size */	
	/* PAD[104] */
	case 0xF00: /* mem_control */	
	case 0xF04: /* mem_ecc_errlog0 */	
	case 0xF08: /* mem_ecc_errlog1 */	
	case 0xF0C: /* link_status */	
	case 0xF10: /* strap_status */	
	case 0xF14: /* reset_status */	
	case 0xF18: /* reseten_in_linkdown */	
	case 0xF1C: /* misc_intr_en */	
	case 0xF20: /* tx_debug_cfg */	
	case 0xF24: /* misc_config */	
	case 0xF28: /* misc_status */	
	/* PAD[1] */
	case 0xF30: /* intr_en */		
	case 0xF34: /* intr_clear */	
	case 0xF38: /* intr_status */	
	/* PAD[49] */
    default:
 //   bad_reg:
	DB_PRINT("Bad register offset 0x%x\n", (int)offset);
        return 0;
    }
}


static uint64_t pcieg2regs_read(void *opaque, hwaddr offset, unsigned size)
{
    uint32_t ret = pcieg2regs_read_imp(opaque, offset, size);

    DB_PRINT("addr: %08x data: %08x\n", (unsigned)offset, (unsigned)ret);
    return ret;
}

static void pcieg2regs_write(void *opaque, hwaddr offset,
                             uint64_t val, unsigned size)
{
    PCIEG2regs_State *s = (PCIEG2regs_State *)opaque;

    DB_PRINT("offset: %08x data: %08x\n", (unsigned)offset, (unsigned)val);

    switch (offset) {
	case 0x000: /*clk_control*/
	case 0x004: /*rc_pm_control*/	
	case 0x008: /*rc_pm_status */	
	case 0x00C: /*ep_pm_control */	
	case 0x010: /*ep_pm_status */	
	case 0x014: /* ep_ltr_control */	
	case 0x018: /* ep_ltr_status */	
	case 0x01C: /* ep_obff_status */	
	case 0x020: /* pcie_err_status */	
	/* PAD[55] */
	case 0x100: /* rc_axi_config */	
	case 0x104: /* ep_axi_config */	
	case 0x108: /* rxdebug_status0 */	
	case 0x10C: /* rxdebug_control0 */	
	/* PAD[4] */

	/* pcie core supports in direct access to config space */
	case 0x120: /* configindaddr */	/* pcie config space access: Address field */
	case 0x124: /* configinddata */	/* pcie config space access: Data field:  */

	/* PAD[52] */
	case 0x1F8:  /* cfg_addr 		bus, dev, func, reg */
	case 0x1FC: /* cfg_data */		
	 case 0x200:   /* sys_eq_page     memory page for event queues: */
	case 0x204: /* sys_msi_page */	
	case 0x208: /* sys_msi_intren */	
	/* PAD[1] */
	case 0x210: /* sys_msi_ctrl0 */	
	case 0x214: /* sys_msi_ctrl1 */	
	case 0x218: /* sys_msi_ctrl2 */	
	case 0x21C: /* sys_msi_ctrl3 */	
	case 0x220: /* sys_msi_ctrl4 */	
	case 0x224: /* sys_msi_ctrl5 */	
	/* PAD[10] */
	case 0x250: /* sys_eq_head0 */	
	case 0x254: /* sys_eq_tail0 */	
	case 0x258: /* sys_eq_head1 */	
	case 0x25C: /* sys_eq_tail1 */	
	case 0x260: /* sys_eq_head2 */	
	case 0x264: /* sys_eq_tail2 */	 
	case 0x268: /* sys_eq_head3 */	
	case 0x26C: /* sys_eq_tail3 */	
	case 0x270: /* sys_eq_head4 */	
	case 0x274: /* sys_eq_tail4 */	
	case 0x278: /* sys_eq_head5 */	
	case 0x27C: /* sys_eq_tail5 */	
	/* PAD[44] */
	case 0x330: /* sys_rc_intx_en */	
	case 0x334: /* sys_rc_intx_csr */	
	/* PAD[2] */
	case 0x340: /* sys_msi_req */	
	case 0x344: /* sys_host_intr_en */	
	case 0x348: /* sys_host_intr_csr */	
	/* PAD[1] */
	case 0x350: /* sys_host_intr0 */	
	case 0x354: /* sys_host_intr1 */	
	case 0x358: /* sys_host_intr2 */	
	case 0x35C: /* sys_host_intr3 */	
	case 0x360: /* sys_ep_int_en0 */	
	case 0x364: /* sys_ep_int_en1 */	
	/* PAD[2] */
	case 0x370: /* sys_ep_int_csr0 */	
	case 0x374: /* sys_ep_int_csr1 */	
	/* PAD[34] */
	case 0x400 - 0x7FF: /* pciecfg[4][64] 	  PCIE Cfg Space */
	case 0x800: /* sprom[64]  SPROM shadow Area */
	/* PAD[224] */
	case 0xC00: /* func0_imap0_0 */	
	case 0xC04: /* func0_imap0_1 */	
	case 0xC08: /* func0_imap0_2 */	
	case 0xC0C: /* func0_imap0_3 */	
	case 0xC10: /* func0_imap0_4 */	
	case 0xC14: /* func0_imap0_5 */	
	case 0xC18: /* func0_imap0_6 */	
	case 0xC1C: /* func0_imap0_7 */	
	case 0xC20: /* func1_imap0_0 */	
	case 0xC24: /* func1_imap0_1 */	
	case 0xC28: /* func1_imap0_2 */	
	case 0xC2C: /* func1_imap0_3 */	
	case 0xC30: /* func1_imap0_4 */	
	case 0xC34: /* func1_imap0_5 */	
	case 0xC38: /* func1_imap0_6 */	
	case 0xC3C: /* func1_imap0_7 */	
	/* PAD[16] */
	case 0xC80: /* func0_imap1 */	
	/* PAD[1] */
	case 0xC88: /* func1_imap1 */	
	/* PAD[13] */
	case 0xCC0: /* func0_imap2 */	
	/* PAD[1] */
	case 0xCC8: /* func1_imap2 */	
	/* PAD[13] */
	case 0xD00: /* iarr0_lower */	
	case 0xD04: /* iarr0_upper */	
	case 0xD08: /* iarr1_lower */	
	case 0xD0C: /* iarr1_upper */	
	case 0xD10: /* iarr2_lower */	
	case 0xD14: /* iarr2_upper */	
	/* PAD[2] */
	case 0xD20: /* oarr0 */		
	/* PAD[1] */
	case 0xD28: /* oarr1 */		
	/* PAD[1] */
	case 0xD30: /* oarr2 */		
	/* PAD[3] */
	case 0xD40: /* omap0_lower */	
	case 0xD44: /* omap0_upper */	
	case 0xD48: /* omap1_lower */	
	case 0xD4C: /* omap1_upper */	
	case 0xD50: /* omap2_lower */	
	case 0xD54: /* omap2_upper */	
	case 0xD58: /* func1_iarr1_size */	
	case 0xD5C: /* func1_iarr2_size */	
	/* PAD[104] */
	case 0xF00: /* mem_control */	
	case 0xF04: /* mem_ecc_errlog0 */	
	case 0xF08: /* mem_ecc_errlog1 */	
	case 0xF0C: /* link_status */	
	case 0xF10: /* strap_status */	
	case 0xF14: /* reset_status */	
	case 0xF18: /* reseten_in_linkdown */	
	case 0xF1C: /* misc_intr_en */	
	case 0xF20: /* tx_debug_cfg */	
	case 0xF24: /* misc_config */	
	case 0xF28: /* misc_status */	
	/* PAD[1] */
	case 0xF30: /* intr_en */		
	case 0xF34: /* intr_clear */	
	case 0xF38: /* intr_status */	
	/* PAD[49] */
    default:
 //   bad_reg:
            DB_PRINT("Bad register write %x <= %08x\n", (int)offset,
                     (unsigned)val);
        return;
    }
}


static const MemoryRegionOps pcieg2regs_ops = {
    .read = pcieg2regs_read,
    .write = pcieg2regs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


static void pcieg2regs_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sd = SYS_BUS_DEVICE(obj);
    PCIEG2regs_State *s = PCIEG2REGS(obj);

    memory_region_init_io(&s->iomem, OBJECT(dev), &pcieg2regs_ops, s,
                          "pcieg2regs", 0x1000);
    sysbus_init_mmio(sd, &s->iomem);

}


static Property pcieg2regs_properties[] = {
//    DEFINE_PROP_UINT32("chipid", Chipcregs_State, chipcregs.chipid, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_pcieg2regs = {
    .name = "pcieg2regs",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(data, PCIEG2regs_State, 0x1000),
        VMSTATE_END_OF_LIST()
    }
};
static void pcieg2regs_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

 //   dc->realize = pcieg2regs_realize;
    dc->reset = pcieg2regs_reset;
    dc->vmsd = &vmstate_pcieg2regs;
    dc->props = pcieg2regs_properties;
}

static const TypeInfo pcieg2regs_info = {
    .name          = TYPE_PCIEG2REGS,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(PCIEG2regs_State),
    .instance_init = pcieg2regs_init,
//    .instance_finalize = pcieg2regs_finalize,
    .class_init    = pcieg2regs_class_init,
};

static void pcieg2regs_register_types(void)
{
    type_register_static(&pcieg2regs_info);
}

type_init(pcieg2regs_register_types)
