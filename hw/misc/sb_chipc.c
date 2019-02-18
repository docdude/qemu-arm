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
#include "sb_chipc.h"
//#include "sbconfig.h"

#define TYPE_CHIPCREGS "sb_chipc"
#define CHIPCREGS(obj) \
    OBJECT_CHECK(Chipcregs_State, (obj), TYPE_CHIPCREGS)

//#define CHIPCREGS_ERR_DEBUG
#ifdef CHIPCREGS_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif
#define NFLASH_SUPPORT
typedef struct Chipcregs_State {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    uint32_t core_id;

    union {
	struct {
	    struct _chipcregs chipcregs;
/***SSB Registers***/
    };
    uint8_t data[0x1000];
    };
} Chipcregs_State;


static void chipcregs_reset(DeviceState *d)
{
    Chipcregs_State *s = CHIPCREGS(d);
    DB_PRINT("****RESET****\n");

      s->chipcregs.capabilities_ext = 0x00000002;
    s->chipcregs.flashcontrol = 0x80000000;
    s->chipcregs.clkdiv = 0xe0830;

    s->chipcregs.pmucapabilities = 0x00000011;
s->chipcregs.corecontrol=0x3d;
}

static uint64_t chipcregs_read_imp(void *opaque, hwaddr offset, unsigned size)
{
    Chipcregs_State *s = (Chipcregs_State *)opaque;

    switch (offset) {
    case 0x00: /* CHIP ID */
        return s->chipcregs.chipid;
    case 0x04: /* CAPABILITIES */
      //  return 0x908003C3; //SSB
	return 0x48000A;  //AIx
    case 0x08: /* CORE CONTROL*/
	return s->chipcregs.corecontrol;
    case 0x0c: /* BIST */
	return s->chipcregs.bist;
    /* OTP */
    case 0x10: /* OTP Status */
	return 0x9000;
    case 0x14: /* OTP Control */
	return 0;
    case 0x18: /* OTP Prog */
	return 0;
    case 0x1c: /* OTPLAYOUT corerev >= 23 */
    	return 0x111200;
    /* Interrupt control */
    case 0x20: /* INTERRUPT STATUS */

    case 0x24: /* INTERRUPT MASK */
	return s->chipcregs.intmask;
    /* Chip specific regs */
    case 0x28: /* CHIP CONTROL */
	return 0;
    case 0x2c: /* CHIP STATUS */
	return s->chipcregs.chipstatus;  //possible values 7000000(usb3),6000000,5000000,4000000,3000000(pcie)....0(stiod)
    /* Jtag Master */
    case 0x30: /* JTAG Command */
    case 0x34: /* JTAG IR */
    case 0x38: /* JTAG DR */
    case 0x3c: /* JTAG Control */

    /* serial flash interface registers */
    case 0x40: /* FLASH CONTROL */
	return s->chipcregs.flashcontrol;
    case 0x44: /* FLASH ADDRESS */
    case 0x48: /* FLASH DATA */

    /* Silicon backplane configuration broadcast control */
    case 0x50: /* Broadcast Address */
	return s->chipcregs.broadcastaddress;
    case 0x54: /* Broadcast Data */
	return s->chipcregs.broadcastdata;
    /* gpio - cleared only by power-on-reset */
    case 0x58: /* GPIOPULLUP corerev >= 20 */
    case 0x5c: /* GPIOPULLDOWN corerev >= 20 */
    case 0x60: /* GPIOIN */
    case 0x64: /* GPIOOUT */
    case 0x68: /* GPIOOUTEN */
    case 0x6c: /* GPIO Control */
    case 0x70: /* GPIO Polarity */
    case 0x74: /* GPIO Interrupt Mask */

    /* GPIO events corerev >= 11 */
    case 0x78: /* GPIOEVENT */
    case 0x7c: /* GPIOEVENTINTMASK */
 
    case 0x80: /* Watchdog Timer */

    /* GPIO events corerev >= 11 */
    case 0x84: /* gpioeventintpolarity */

    /* GPIO based LED powersave registers corerev >= 16 */
    case 0x88: /* gpiotimerval */
	return s->chipcregs.gpiotimerval;
    case 0x8c: /* gpiotimeroutmask */
    case 0x90: /* Clock Control N */
    case 0x94: /* Clock Control SB */
    case 0x98: /* Clock Control PCI */
    case 0x9c: /* Clock Control M2 */
    case 0xa0: /* clockcontrol_m3 cpu */
    case 0xa4: /* clkdiv :: corerev >= 3 */
	return s->chipcregs.clkdiv;
    case 0xa8: /* gpiodebugsel :: corerev >= 28 */
    case 0xac: /* capabilities_ext */
	return s->chipcregs.capabilities_ext;
    /* pll delay registers (corerev >= 4) */
    case 0xb0: /* pll_on_delay */
    case 0xb4: /* fref_sel_delay */
    case 0xb8: /* slow_clk_ctl :: 5 < corerev < 10 */
    case 0xbc: /* PAD */

    /* Instaclock registers (corerev >= 10) */
    case 0xc0: /* system_clk_ctl */
    case 0xc4: /* clkstatestretch */
    case 0xc8:  /* PAD[2] */

    /* Indirect backplane access (corerev >= 22) */
    case 0xd0: /* bp_addrlow 0xd0 */
    case 0xd4: /* bp_addrhigh */
    case 0xd8: /* bp_data */
    case 0xdc: /* PAD */
    case 0xe0: /* bp_indaccess */
    case 0xe4: /* PAD[3] */

    /* More clock dividers (corerev >= 32) */
    case 0xf0: /* clkdiv2 */
    case 0xf4: /* PAD[2] */

    /* In AI chips, pointer to erom */
    case 0xfc: /* eromptr */
	return 0xffff1000;
    /* ExtBus control registers (corerev >= 3) */
    case 0x100: /*pcmcia_config */
    case 0x104: /* pcmcia_memwait */
    case 0x108: /* pcmcia_attrwait  */
    case 0x10c: /* pcmcia_iowait */
    case 0x110: /* ide_config */
    case 0x114: /* ide_memwait */
    case 0x118: /* ide_attrwait */
    case 0x11c: /* ide_iowait */
    case 0x120: /* prog_config */
    case 0x124: /* prog_waitcount */
    case 0x128: /* flash_config */
    case 0x12c: /* flash_waitcount */
    case 0x130: /* SECI_config SECI configuration */
    case 0x134: /* PAD[3] */

    /* Enhanced Coexistence Interface (ECI) registers (corerev >= 21) */
    case 0x140: /* eci_output */
    case 0x144: /* eci_control */
    case 0x148: /* eci_inputlo */
    case 0x14c: /* eci_inputmi */
    case 0x150: /* eci_inputhi */
    case 0x154: /* eci_inputintpolaritylo */
    case 0x158: /* eci_inputintpolaritymi */
    case 0x15c: /* eci_inputintpolarityhi */
    case 0x160: /* eci_intmasklo */
    case 0x164: /* eci_intmaskmi */
    case 0x168: /* eci_intmaskhi */
    case 0x16c: /* eci_eventlo */
    case 0x170: /* eci_eventmi */
    case 0x174: /* eci_eventhi */
    case 0x178: /* eci_eventmasklo */
    case 0x17c: /* eci_eventmaskmi */
    case 0x180: /* eci_eventmaskhi */
    case 0x184: /* PAD[3] */

    /* SROM interface (corerev >= 32) */
    case 0x190: /* sromcontrol */
    case 0x194: /* sromaddress */
    case 0x198: /* sromdata */
    case 0x19c: /* PAD[17] */

    /* Clock control and hardware workarounds (corerev >= 20) */
    case 0x1e0: /* clk_ctl_st */
    case 0x1e4: /* hw_war */
    case 0x1e8: /* PAD[70] */

    /* UARTs */
    case 0x300: /* uart0data */
    case 0x304: /* uart0imr */
    case 0x308: /* uart0fcr */
    case 0x30c: /* uart0lcr */
    case 0x310: /* uart0mcr */
    case 0x314: /* uart0lsr */
    case 0x318: /* uart0msr */
    case 0x31c: /* uart0scratch */
    case 0x320: /* PAD[248] :: corerev >= 1 */
            goto bad_reg;
	break;
    case 0x400: /* uart1data */
    case 0x404: /* uart1imr */
    case 0x408: /* uart1fcr */
    case 0x40c: /* uart1lcr */
    case 0x410: /* uart1mcr */
    case 0x414: /* uart1lsr */
    case 0x418: /* uart1msr */
    case 0x41c: /* uart1scratch */
    case 0x420: /* PAD[126] */
    case 0x424: /* PAD[248] :: corerev >= 1 */
            goto bad_reg;
	break;
    /* PMU registers (corerev >= 20) */
    case 0x600: /* pmucontrol */
	return s->chipcregs.pmucontrol;
    case 0x604: /* PMU Cababilities */
	return s->chipcregs.pmucapabilities;
    case 0x608: /* pmustatus */
    case 0x60c: /* res_state */
    case 0x610: /* res_pending */
    case 0x614: /* pmutimer */
    case 0x618: /* min_res_mask */
    case 0x61c: /* max_res_mask */
    case 0x620: /* res_table_sel */
    case 0x624: /* res_dep_mask */
    case 0x628: /* res_updn_timer */
    case 0x62c: /* res_timer */
    case 0x630: /* clkstretch */
    case 0x634: /* pmuwatchdog */
	return s->chipcregs.pmuwatchdog;
    case 0x638: /* gpiosel :: rev >= 1 */
    case 0x63c: /* gpioenable :: rev >= 1 */
    case 0x640: /* res_req_timer_sel */
    case 0x644: /* res_req_timer */
    case 0x648: /* res_req_mask */
    case 0x64c: /* pmucapabilities_ext :: pmurev >=15 */
    case 0x650: /* chipcontrol_addr */
    case 0x654: /* chipcontrol_data */
    case 0x658: /* regcontrol_addr */
    case 0x65c: /* regcontrol_data */
    case 0x660: /* pllcontrol_addr */
    case 0x664: /* pllcontrol_data */
    case 0x668: /* pmustrapopt :: corerev >= 28 */
    case 0x66c: /* pmu_xtalfreq :: pmurev >= 10 */
    case 0x670: /* retention_ctl :: pmurev >= 15 */
    case 0x674: /* PAD[3] */
    case 0x680: /* retention_grpidx */
    case 0x684: /* retention_grpctl */
    case 0x688: /* PAD[94] */
    case 0x800: /* sromotp[768] */
    /* Nand flash MLC controller registers (corerev >= 38) */
#ifdef NFLASH_SUPPORT
    case 0xc00: /* nand_revision */
    case 0xc04: /* nand_cmd_start */
    case 0xc08: /* nand_cmd_addr_x */
    case 0xc0c: /* nand_cmd_addr */
    case 0xc10: /* nand_cmd_end_addr */
    case 0xc14: /* nand_cs_nand_select */
    case 0xc18: /* nand_cs_nand_xor */
    case 0xc1c: /* PAD */
    case 0xc20: /* nand_spare_rd0 */
    case 0xc24: /* nand_spare_rd4 */
    case 0xc28: /* nand_spare_rd8 */
    case 0xc2c: /* nand_spare_rd12 */
    case 0xc30: /* nand_spare_wr0 */
    case 0xc34: /* nand_spare_wr4 */
    case 0xc38: /* nand_spare_wr8 */
    case 0xc3c: /* nand_spare_wr12 */
    case 0xc40: /* nand_acc_control */
    case 0xc44: /* PAD */
    case 0xc48: /* nand_config */
    case 0xc4c: /* PAD */
    case 0xc50: /* nand_timing_1 */
    case 0xc54: /* nand_timing_2 */
    case 0xc58: /* nand_semaphore */
    case 0xc5c: /* PAD */
    case 0xc60: /* nand_devid */
         return 0xc2;
    case 0xc64: /* nand_devid_x */
    case 0xc68: /* nand_block_lock_status */
    case 0xc6c: /* nand_intfc_status */
    case 0xc70: /* nand_ecc_corr_addr_x */
    case 0xc74: /* nand_ecc_corr_addr */
    case 0xc78: /* nand_ecc_unc_addr_x */
    case 0xc7c: /* nand_ecc_unc_addr */
    case 0xc80: /* nand_read_error_count */
    case 0xc84: /* nand_corr_stat_threshold */
    case 0xc88: /* PAD[2] */
    case 0xc90: /* nand_read_addr_x */
    case 0xc94: /* nand_read_addr */
    case 0xc98: /* nand_page_program_addr_x */
    case 0xc9c: /* nand_page_program_addr */
    case 0xca0: /* nand_copy_back_addr_x */
    case 0xca4: /* nand_copy_back_addr */
    case 0xca8: /* nand_block_erase_addr_x */
    case 0xcac: /* nand_block_erase_addr */
    case 0xcb0: /* nand_inv_read_addr_x */
    case 0xcb4: /* nand_inv_read_addr */
    case 0xcb8: /* PAD[2] */
    case 0xcc0: /* nand_blk_wr_protect */
    case 0xcc4: /* PAD[3] */
    case 0xcd0: /* nand_acc_control_cs1 */
    case 0xcd4: /* nand_config_cs1 */
    case 0xcd8: /* nand_timing_1_cs1 */
    case 0xcdc: /* nand_timing_2_cs1 */
    case 0xce0: /* PAD[20] */
    case 0xd30: /* nand_spare_rd16 */
    case 0xd34: /* nand_spare_rd20 */
    case 0xd38: /* nand_spare_rd24 */
    case 0xd3c: /* nand_spare_rd28 */
    case 0xd40: /* nand_cache_addr */
    case 0xd44: /* nand_cache_data */
    case 0xd48: /* nand_ctrl_config */
    case 0xd4c: /* nand_ctrl_status */
    /* NFLASH_SUPPORT */
#else    /* GCI starting at 0xC00 */
    case 0xc00: /* gci_corecaps0 */ 
    case 0xc04: /* gci_corecaps1 */
    case 0xc08: /* gci_corecaps2 */
    case 0xc0c: /* gci_corectrl */
    case 0xc10: /* gci_corestat */ 
    case 0xc14: /* gci_intstat */
    case 0xc18: /* gci_intmask */ 
    case 0xc1c: /* gci_wakemask */ 
    case 0xc20: /* gci_levelintstat */ 
    case 0xc24: /* gci_eventintstat */
    case 0xc28: /* PAD[6] */
    case 0xc40: /* gci_indirect_addr */
    case 0xc44: /* gci_gpioctl */
    case 0xc48: /* PAD */
    case 0xc4c: /* gci_gpiomask */
    case 0xc50: /* PAD */
    case 0xc54: /* gci_miscctl */ 
    case 0xc58: /* PAD[2] */
    case 0xc60: /* gci_input[32] */ 
    case 0xce0: /* gci_event[32] */
    case 0xd60: /* gci_output[4] */
    case 0xd70: /* gci_control_0 */
    case 0xd74: /* gci_control_1 */ 
    case 0xd78: /* gci_level_polreg */ 
    case 0xd7c: /* gci_levelintmask */ 
    case 0xd80: /* gci_eventintmask */ 
    case 0xd84: /* PAD[3] */
    case 0xd90: /* gci_inbandlevelintmask */ 
    case 0xd94: /* gci_inbandeventintmask */ 
    case 0xd98: /* PAD[2] */
    case 0xda0: /* gci_seciauxtx */ 
    case 0xda4: /* gci_seciauxrx */ 
    case 0xda8: /* gci_secitx_datatag */ 
    case 0xdac: /* gci_secirx_datatag */ 
    case 0xdb0: /* gci_secitx_datamask */ 
    case 0xdb4: /* gci_seciusef0tx_reg */ 
    case 0xdb8: /* gci_secif0tx_offset */ 
    case 0xdbc: /* gci_secif0rx_offset */ 
    case 0xdc0: /* gci_secif1tx_offset */ 
    case 0xdc4: /* PAD[3] */
    case 0xdd0: /* gci_uartescval */ 
    case 0xdd4: /* PAD[3] */
    case 0xde0: /* gci_secibauddiv */ 
    case 0xde4: /* gci_secifcr */ 
    case 0xde8: /* gci_secilcr */ 
    case 0xdec: /* gci_secimcr */ 
    case 0xdf0: /* PAD[2] */
    case 0xdf8: /* gci_baudadj */ 
    case 0xdfc: /* PAD */
    case 0xe00: /* gci_chipctrl */ 
    case 0xe04: /* gci_chipsts */ 
#endif
#if 0 /////moved to sb_hwregs.c/////
    /***********Sonics Silicon Backplane************/
    case 0x0EA8: /* IM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0EB0: /* IM Error Log Backplane Revision >= 2.3 Only */ 
    case 0x0ED8: /* TM Port Connection ID 0 Backplane Revision >= 2.3 Only */ 
    case 0x0EF8: /* TM Port Lock 0 Backplane Revision >= 2.3 Only */
    case 0x0F08: /* IPS Flag */ 
    case 0x0F18: /* TPS Flag */
    case 0x0F48: /* TM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0F50: /* TM Error Log Backplane Revision >= 2.3 Only */
    case 0x0F60: /* Address Match 3 */
    case 0x0F68: /* Address Match 2 */
    case 0x0F70: /* Address Match 1 */
    case 0x0F90: /* IM State */
	return s->sbimstate;
    case 0x0F94: /* Interrupt Vector  */
    case 0x0F98: /* TM State Low */
        return 0x00030001;
    case 0x0F9C: /* TM State High */
	return s->sbtmstatehigh;
    case 0x0FA0: /* BWA0 */
    case 0x0FA8: /* IM Config Low */
    case 0x0FAC: /* IM Config High */
    case 0x0FB0: /* Address Match 0 */ 
	return 	s->sbadmatch0;
    case 0x0FB8: /* TM Config Low */
	return s->sbtmstatelow;
    case 0x0FBC: /* TM Config High */
    case 0x0FC0: /* B Config */ 
    case 0x0FC8: /* B State */
    case 0x0FD8: /* ACTCNFG */
    case 0x0FE8: /* FLAGST */
//        break;
    case 0x0FF8: /* Core ID Low */
	return 0x70000000;  //s->sbidlow
    case 0x0FFC: /* Core ID High */
	return s->sbidhigh;//0x4243a00F;//0x4243900f; 
#endif
    /***********CHIPCOMMON B************/
    case 0x1000: /* SPROM Shadow Area */
    default:
    bad_reg:
	DB_PRINT("Bad register offset 0x%x\n", (int)offset);
        return 0;
    }
}

static uint64_t chipcregs_read(void *opaque, hwaddr offset, unsigned size)
{
    uint32_t ret = chipcregs_read_imp(opaque, offset, size);

    DB_PRINT("addr: %08x data: %08x\n", (unsigned)offset, (unsigned)ret);
    return ret;
}

#if 0
/* SYS_CFGCTRL functions */
#define SYS_CFG_OSC 1
#define SYS_CFG_VOLT 2
#define SYS_CFG_AMP 3
#define SYS_CFG_TEMP 4
#define SYS_CFG_RESET 5
#define SYS_CFG_SCC 6
#define SYS_CFG_MUXFPGA 7
#define SYS_CFG_SHUTDOWN 8
#define SYS_CFG_REBOOT 9
#define SYS_CFG_DVIMODE 11
#define SYS_CFG_POWER 12
#define SYS_CFG_ENERGY 13

/* SYS_CFGCTRL site field values */
#define SYS_CFG_SITE_MB 0
#define SYS_CFG_SITE_DB1 1
#define SYS_CFG_SITE_DB2 2

/**
 * vexpress_cfgctrl_read:
 * @s: arm_sysctl_state pointer
 * @dcc, @function, @site, @position, @device: split out values from
 * SYS_CFGCTRL register
 * @val: pointer to where to put the read data on success
 *
 * Handle a VExpress SYS_CFGCTRL register read. On success, return true and
 * write the read value to *val. On failure, return false (and val may
 * or may not be written to).
 */

static bool vexpress_cfgctrl_read(arm_sysctl_state *s, unsigned int dcc,
                                  unsigned int function, unsigned int site,
                                  unsigned int position, unsigned int device,
                                  uint32_t *val)
{
    /* We don't support anything other than DCC 0, board stack position 0
     * or sites other than motherboard/daughterboard:
     */
    if (dcc != 0 || position != 0 ||
        (site != SYS_CFG_SITE_MB && site != SYS_CFG_SITE_DB1)) {
        goto cfgctrl_unimp;
    }

    switch (function) {
    case SYS_CFG_VOLT:
        if (site == SYS_CFG_SITE_DB1 && device < s->db_num_vsensors) {
            *val = s->db_voltage[device];
            return true;
        }
        if (site == SYS_CFG_SITE_MB && device == 0) {
            /* There is only one motherboard voltage sensor:
             * VIO : 3.3V : bus voltage between mother and daughterboard
             */
            *val = 3300000;
            return true;
        }
        break;
    case SYS_CFG_OSC:
        if (site == SYS_CFG_SITE_MB && device < ARRAY_SIZE(s->mb_clock)) {
            /* motherboard clock */
            *val = s->mb_clock[device];
            return true;
        }
        if (site == SYS_CFG_SITE_DB1 && device < s->db_num_clocks) {
            /* daughterboard clock */
            *val = s->db_clock[device];
            return true;
        }
        break;
    default:
        break;
    }

cfgctrl_unimp:
    qemu_log_mask(LOG_UNIMP,
                  "arm_sysctl: Unimplemented SYS_CFGCTRL read of function "
                  "0x%x DCC 0x%x site 0x%x position 0x%x device 0x%x\n",
                  function, dcc, site, position, device);
    return false;
}

/**
 * vexpress_cfgctrl_write:
 * @s: arm_sysctl_state pointer
 * @dcc, @function, @site, @position, @device: split out values from
 * SYS_CFGCTRL register
 * @val: data to write
 *
 * Handle a VExpress SYS_CFGCTRL register write. On success, return true.
 * On failure, return false.
 */
static bool vexpress_cfgctrl_write(arm_sysctl_state *s, unsigned int dcc,
                                   unsigned int function, unsigned int site,
                                   unsigned int position, unsigned int device,
                                   uint32_t val)
{
    /* We don't support anything other than DCC 0, board stack position 0
     * or sites other than motherboard/daughterboard:
     */
    if (dcc != 0 || position != 0 ||
        (site != SYS_CFG_SITE_MB && site != SYS_CFG_SITE_DB1)) {
        goto cfgctrl_unimp;
    }

    switch (function) {
    case SYS_CFG_OSC:
        if (site == SYS_CFG_SITE_MB && device < ARRAY_SIZE(s->mb_clock)) {
            /* motherboard clock */
            s->mb_clock[device] = val;
            return true;
        }
        if (site == SYS_CFG_SITE_DB1 && device < s->db_num_clocks) {
            /* daughterboard clock */
            s->db_clock[device] = val;
            return true;
        }
        break;
    case SYS_CFG_MUXFPGA:
        if (site == SYS_CFG_SITE_MB && device == 0) {
            /* Select whether video output comes from motherboard
             * or daughterboard: log and ignore as QEMU doesn't
             * support this.
             */
            qemu_log_mask(LOG_UNIMP, "arm_sysctl: selection of video output "
                          "not supported, ignoring\n");
            return true;
        }
        break;
    case SYS_CFG_SHUTDOWN:
        if (site == SYS_CFG_SITE_MB && device == 0) {
            qemu_system_shutdown_request();
            return true;
        }
        break;
    case SYS_CFG_REBOOT:
        if (site == SYS_CFG_SITE_MB && device == 0) {
            qemu_system_reset_request();
            return true;
        }
        break;
    case SYS_CFG_DVIMODE:
        if (site == SYS_CFG_SITE_MB && device == 0) {
            /* Selecting DVI mode is meaningless for QEMU: we will
             * always display the output correctly according to the
             * pixel height/width programmed into the CLCD controller.
             */
            return true;
        }
    default:
        break;
    }

cfgctrl_unimp:
    qemu_log_mask(LOG_UNIMP,
                  "arm_sysctl: Unimplemented SYS_CFGCTRL write of function "
                  "0x%x DCC 0x%x site 0x%x position 0x%x device 0x%x\n",
                  function, dcc, site, position, device);
    return false;
}
#endif
static void chipcregs_write(void *opaque, hwaddr offset,
                             uint64_t val, unsigned size)
{
    Chipcregs_State *s = (Chipcregs_State *)opaque;

    DB_PRINT("offset: %08x data: %08x\n", (unsigned)offset, (unsigned)val);

    switch (offset) {
    case 0x00: /* CHIP ID */
    case 0x04: /* CAPABILITIES */
    case 0x08: /* CORE CONTROL*/
	s->chipcregs.corecontrol=val;
    case 0x0c: /* BIST */
 	s->chipcregs.bist=val;
    /* OTP */
    case 0x10: /* OTP Status */
    case 0x14: /* OTP Control */
    case 0x18: /* OTP Prog */
    case 0x1c: /* OTPLAYOUT corerev >= 23 */
    
    /* Interrupt control */
    case 0x20: /* INTERRUPT STATUS */
    case 0x24: /* INTERRUPT MASK */
	s->chipcregs.intmask=val;
	break;
    /* Chip specific regs */
    case 0x28: /* CHIP CONTROL */
    case 0x2c: /* CHIP STATUS */
	s->chipcregs.chipstatus=val;
    /* Jtag Master */
    case 0x30: /* JTAG Command */
    case 0x34: /* JTAG IR */
    case 0x38: /* JTAG DR */
    case 0x3c: /* JTAG Control */

    /* serial flash interface registers */
    case 0x40: /* FLASH CONTROL */
    case 0x44: /* FLASH ADDRESS */
    case 0x48: /* FLASH DATA */

    /* Silicon backplane configuration broadcast control */
    case 0x50: /* Broadcast Address */
        s->chipcregs.broadcastaddress = val;
    case 0x54: /* Broadcast Data */

    /* gpio - cleared only by power-on-reset */
    case 0x58:  /* GPIOPULLUP corerev >= 20 */
    case 0x5c: /* GPIOPULLDOWN corerev >= 20 */
    case 0x60: /* GPIOIN */
    case 0x64: /* GPIOOUT */
    case 0x68: /* GPIOOUTEN */
    case 0x6c: /* GPIO Control */
    case 0x70: /* GPIO Polarity */
    case 0x74: /* GPIO Interrupt Mask */

    /* GPIO events corerev >= 11 */
    case 0x78: /* GPIOEVENT */
    case 0x7c: /* GPIOEVENTINTMASK */
 
    case 0x80: /* Watchdog Timer */

    /* GPIO events corerev >= 11 */
    case 0x84: /* gpioeventintpolarity */

    /* GPIO based LED powersave registers corerev >= 16 */
    case 0x88: /* gpiotimerval */
	s->chipcregs.gpiotimerval=val;
	break;
    case 0x8c: /* gpiotimeroutmask */
    case 0x90: /* Clock Control N */
    case 0x94: /* Clock Control SB */
    case 0x98: /* Clock Control PCI */
    case 0x9c: /* Clock Control M2 */
    case 0xa0: /* clockcontrol_m3 cpu */
    case 0xa4: /* clkdiv :: corerev >= 3 */
    case 0xa8: /* gpiodebugsel :: corerev >= 28 */
    case 0xac: /* capabilities_ext */

    /* pll delay registers (corerev >= 4) */
    case 0xb0: /* pll_on_delay */
    case 0xb4: /* fref_sel_delay */
    case 0xb8: /* slow_clk_ctl :: 5 < corerev < 10 */
    case 0xbc: /* PAD */

    /* Instaclock registers (corerev >= 10) */
    case 0xc0: /* system_clk_ctl */
    case 0xc4: /* clkstatestretch */
    case 0xc8:  /* PAD[2] */

    /* Indirect backplane access (corerev >= 22) */
    case 0xd0: /* bp_addrlow 0xd0 */
    case 0xd4: /* bp_addrhigh */
    case 0xd8: /* bp_data */
    case 0xdc: /* PAD */
    case 0xe0: /* bp_indaccess */
    case 0xe4: /* PAD[3] */

    /* More clock dividers (corerev >= 32) */
    case 0xf0: /* clkdiv2 */
    case 0xf4: /* PAD[2] */

    /* In AI chips, pointer to erom */
    case 0xfc: /* eromptr */

    /* ExtBus control registers (corerev >= 3) */
    case 0x100: /*pcmcia_config */
    case 0x104: /* pcmcia_memwait */
    case 0x108: /* pcmcia_attrwait  */
    case 0x10c: /* pcmcia_iowait */
    case 0x110: /* ide_config */
    case 0x114: /* ide_memwait */
    case 0x118: /* ide_attrwait */
    case 0x11c: /* ide_iowait */
    case 0x120: /* prog_config */
    case 0x124: /* prog_waitcount */
    case 0x128: /* flash_config */
    case 0x12c: /* flash_waitcount */
    case 0x130: /* SECI_config SECI configuration */
    case 0x134: /* PAD[3] */

    /* Enhanced Coexistence Interface (ECI) registers (corerev >= 21) */
    case 0x140: /* eci_output */
    case 0x144: /* eci_control */
    case 0x148: /* eci_inputlo */
    case 0x14c: /* eci_inputmi */
    case 0x150: /* eci_inputhi */
    case 0x154: /* eci_inputintpolaritylo */
    case 0x158: /* eci_inputintpolaritymi */
    case 0x15c: /* eci_inputintpolarityhi */
    case 0x160: /* eci_intmasklo */
    case 0x164: /* eci_intmaskmi */
    case 0x168: /* eci_intmaskhi */
    case 0x16c: /* eci_eventlo */
    case 0x170: /* eci_eventmi */
    case 0x174: /* eci_eventhi */
    case 0x178: /* eci_eventmasklo */
    case 0x17c: /* eci_eventmaskmi */
    case 0x180: /* eci_eventmaskhi */
    case 0x184: /* PAD[3] */

    /* SROM interface (corerev >= 32) */
    case 0x190: /* sromcontrol */
    case 0x194: /* sromaddress */
    case 0x198: /* sromdata */
    case 0x19c: /* PAD[17] */

    /* Clock control and hardware workarounds (corerev >= 20) */
    case 0x1e0: /* clk_ctl_st */
    case 0x1e4: /* hw_war */
    case 0x1e8: /* PAD[70] */

	/* UARTs */
    case 0x300: /* uart0data */
    case 0x304: /* uart0imr */
    case 0x308: /* uart0fcr */
    case 0x30c: /* uart0lcr */
    case 0x310: /* uart0mcr */
    case 0x314: /* uart0lsr */
    case 0x318: /* uart0msr */
    case 0x31c: /* uart0scratch */
    case 0x320: /* PAD[248] :: corerev >= 1 */

    case 0x400: /* uart1data */
    case 0x404: /* uart1imr */
    case 0x408: /* uart1fcr */
    case 0x40c: /* uart1lcr */
    case 0x410: /* uart1mcr */
    case 0x414: /* uart1lsr */
    case 0x418: /* uart1msr */
    case 0x420: /* uart1scratch */
    case 0x424: /* PAD[126] */

    /* PMU registers (corerev >= 20) */
    case 0x600: /* pmucontrol */
    case 0x604: /* pmucapabilities */
        s->chipcregs.pmucapabilities = val;
	break;
    case 0x608: /* pmustatus */
    case 0x60c: /* res_state */
    case 0x610: /* res_pending */
    case 0x614: /* pmutimer */
    case 0x618: /* min_res_mask */
    case 0x61c: /* max_res_mask */
    case 0x620: /* res_table_sel */
    case 0x624: /* res_dep_mask */
    case 0x628: /* res_updn_timer */
    case 0x62c: /* res_timer */
    case 0x630: /* clkstretch */
    case 0x634: /* pmuwatchdog */
	s->chipcregs.pmuwatchdog=val;
	break;
    case 0x638: /* gpiosel :: rev >= 1 */
    case 0x63c: /* gpioenable :: rev >= 1 */
    case 0x640: /* res_req_timer_sel */
    case 0x644: /* res_req_timer */
    case 0x648: /* res_req_mask */
    case 0x64c: /* pmucapabilities_ext :: pmurev >=15 */
    case 0x650: /* chipcontrol_addr */
    case 0x654: /* chipcontrol_data */
    case 0x658: /* regcontrol_addr */
    case 0x65c: /* regcontrol_data */
    case 0x660: /* pllcontrol_addr */
    case 0x664: /* pllcontrol_data */
    case 0x668: /* pmustrapopt :: corerev >= 28 */
    case 0x66c: /* pmu_xtalfreq :: pmurev >= 10 */
    case 0x670: /* retention_ctl :: pmurev >= 15 */
    case 0x674: /* PAD[3] */
    case 0x680: /* retention_grpidx */
    case 0x684: /* retention_grpctl */
    case 0x688: /* PAD[94] */
    case 0x800: /* sromotp[768] */
    /* Nand flash MLC controller registers (corerev >= 38) */
#ifdef NFLASH_SUPPORT
    case 0xc00: /* nand_revision */
    case 0xc04: /* nand_cmd_start */
    case 0xc08: /* nand_cmd_addr_x */
    case 0xc0c: /* nand_cmd_addr */
    case 0xc10: /* nand_cmd_end_addr */
    case 0xc14: /* nand_cs_nand_select */
    case 0xc18: /* nand_cs_nand_xor */
    case 0xc1c: /* PAD */
    case 0xc20: /* nand_spare_rd0 */
    case 0xc24: /* nand_spare_rd4 */
    case 0xc28: /* nand_spare_rd8 */
    case 0xc2c: /* nand_spare_rd12 */
    case 0xc30: /* nand_spare_wr0 */
    case 0xc34: /* nand_spare_wr4 */
    case 0xc38: /* nand_spare_wr8 */
    case 0xc3c: /* nand_spare_wr12 */
    case 0xc40: /* nand_acc_control */
    case 0xc44: /* PAD */
    case 0xc48: /* nand_config */
    case 0xc4c: /* PAD */
    case 0xc50: /* nand_timing_1 */
    case 0xc54: /* nand_timing_2 */
    case 0xc58: /* nand_semaphore */
    case 0xc5c: /* PAD */
    case 0xc60: /* nand_devid */
    case 0xc64: /* nand_devid_x */
    case 0xc68: /* nand_block_lock_status */
    case 0xc6c: /* nand_intfc_status */
    case 0xc70: /* nand_ecc_corr_addr_x */
    case 0xc74: /* nand_ecc_corr_addr */
    case 0xc78: /* nand_ecc_unc_addr_x */
    case 0xc7c: /* nand_ecc_unc_addr */
    case 0xc80: /* nand_read_error_count */
    case 0xc84: /* nand_corr_stat_threshold */
    case 0xc88: /* PAD[2] */
    case 0xc90: /* nand_read_addr_x */
    case 0xc94: /* nand_read_addr */
    case 0xc98: /* nand_page_program_addr_x */
    case 0xc9c: /* nand_page_program_addr */
    case 0xca0: /* nand_copy_back_addr_x */
    case 0xca4: /* nand_copy_back_addr */
    case 0xca8: /* nand_block_erase_addr_x */
    case 0xcac: /* nand_block_erase_addr */
    case 0xcb0: /* nand_inv_read_addr_x */
    case 0xcb4: /* nand_inv_read_addr */
    case 0xcb8: /* PAD[2] */
    case 0xcc0: /* nand_blk_wr_protect */
    case 0xcc4: /* PAD[3] */
    case 0xcd0: /* nand_acc_control_cs1 */
    case 0xcd4: /* nand_config_cs1 */
    case 0xcd8: /* nand_timing_1_cs1 */
    case 0xcdc: /* nand_timing_2_cs1 */
    case 0xce0: /* PAD[20] */
    case 0xd30: /* nand_spare_rd16 */
    case 0xd34: /* nand_spare_rd20 */
    case 0xd38: /* nand_spare_rd24 */
    case 0xd3c: /* nand_spare_rd28 */
    case 0xd40: /* nand_cache_addr */
    case 0xd44: /* nand_cache_data */
    case 0xd48: /* nand_ctrl_config */
    case 0xd4c: /* nand_ctrl_status */
#else /* NFLASH_SUPPORT */
    /* GCI starting at 0xC00 */
    case 0xc00: /* gci_corecaps0 */ 
    case 0xc04: /* gci_corecaps1 */
    case 0xc08: /* gci_corecaps2 */
    case 0xc0c: /* gci_corectrl */
    case 0xc10: /* gci_corestat */ 
    case 0xc14: /* gci_intstat */
    case 0xc18: /* gci_intmask */ 
    case 0xc1c: /* gci_wakemask */ 
    case 0xc20: /* gci_levelintstat */ 
    case 0xc24: /* gci_eventintstat */
    case 0xc28: /* PAD[6] */
    case 0xc40: /* gci_indirect_addr */
    case 0xc44: /* gci_gpioctl */
    case 0xc48: /* PAD */
    case 0xc4c: /* gci_gpiomask */
    case 0xc50: /* PAD */
    case 0xc54: /* gci_miscctl */ 
    case 0xc58: /* PAD[2] */
    case 0xc60: /* gci_input[32] */ 
    case 0xce0: /* gci_event[32] */
    case 0xd60: /* gci_output[4] */
    case 0xd70: /* gci_control_0 */
    case 0xd74: /* gci_control_1 */ 
    case 0xd78: /* gci_level_polreg */ 
    case 0xd7c: /* gci_levelintmask */ 
    case 0xd80: /* gci_eventintmask */ 
    case 0xd84: /* PAD[3] */
    case 0xd90: /* gci_inbandlevelintmask */ 
    case 0xd94: /* gci_inbandeventintmask */ 
    case 0xd98: /* PAD[2] */
    case 0xda0: /* gci_seciauxtx */ 
    case 0xda4: /* gci_seciauxrx */ 
    case 0xda8: /* gci_secitx_datatag */ 
    case 0xdac: /* gci_secirx_datatag */ 
    case 0xdb0: /* gci_secitx_datamask */ 
    case 0xdb4: /* gci_seciusef0tx_reg */ 
    case 0xdb8: /* gci_secif0tx_offset */ 
    case 0xdbc: /* gci_secif0rx_offset */ 
    case 0xdc0: /* gci_secif1tx_offset */ 
    case 0xdc4: /* PAD[3] */
    case 0xdd0: /* gci_uartescval */ 
    case 0xdd4: /* PAD[3] */
    case 0xde0: /* gci_secibauddiv */ 
    case 0xde4: /* gci_secifcr */ 
    case 0xde8: /* gci_secilcr */ 
    case 0xdec: /* gci_secimcr */ 
    case 0xdf0: /* PAD[2] */
    case 0xdf8: /* gci_baudadj */ 
    case 0xdfc: /* PAD */
    case 0xe00: /* gci_chipctrl */ 
    case 0xe04: /* gci_chipsts */ 
#endif
#if 0 /////moved to sb_hwregs.c //////
   /***********Sonics Silicon Backplane************/
    case 0x0EA8: /* IM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0EB0: /* IM Error Log Backplane Revision >= 2.3 Only */ 
    case 0x0ED8: /* TM Port Connection ID 0 Backplane Revision >= 2.3 Only */ 
    case 0x0EF8: /* TM Port Lock 0 Backplane Revision >= 2.3 Only */
    case 0x0F08: /* IPS Flag */ 
    case 0x0F18: /* TPS Flag */
    case 0x0F48: /* TM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0F50: /* TM Error Log Backplane Revision >= 2.3 Only */
    case 0x0F60: /* Address Match 3 */
    case 0x0F68: /* Address Match 2 */
    case 0x0F70: /* Address Match 1 */
    case 0x0F90: /* IM State */
	s->sbimstate=val;
	break;
    case 0x0F94: /* Interrupt Vector  */
    case 0x0F98: /* TM State Low */
	s->sbtmstatelow=val;
	break;
    case 0x0F9C: /* TM State High */
	s->sbtmstatehigh=val;
    case 0x0FA0: /* BWA0 */
    case 0x0FA8: /* IM Config Low */
    case 0x0FAC: /* IM Config High */
    case 0x0FB0: /* Address Match 0 */ 
	s->sbadmatch0=val;
    case 0x0FB8: /* TM Config Low */
    case 0x0FBC: /* TM Config High */
    case 0x0FC0: /* B Config */ 
    case 0x0FC8: /* B State */
    case 0x0FD8: /* ACTCNFG */
    case 0x0FE8: /* FLAGST */
    case 0x0FF8: /* Core ID Low */
    case 0x0FFC: /* Core ID High */
#endif
    /***********CHIPCOMMON B************/
    case 0x1000: /* SPROM Shadow Area */
	goto bad_reg;
    break;
#if 0
    case 0x08: /* LED */
        s->leds = val;
        break;
    case 0x0c: /* OSC0 */
    case 0x10: /* OSC1 */
    case 0x14: /* OSC2 */
    case 0x18: /* OSC3 */
    case 0x1c: /* OSC4 */
        /* ??? */
        break;
    case 0x20: /* LOCK */
        if (val == LOCK_VALUE)
            s->lockval = val;
        else
            s->lockval = val & 0x7fff;
        break;
    case 0x28: /* CFGDATA1 */
        /* ??? Need to implement this.  */
        s->cfgdata1 = val;
        break;
    case 0x2c: /* CFGDATA2 */
        /* ??? Need to implement this.  */
        s->cfgdata2 = val;
        break;
    case 0x30: /* FLAGSSET */
        s->flags |= val;
        break;
    case 0x34: /* FLAGSCLR */
        s->flags &= ~val;
        break;
    case 0x38: /* NVFLAGSSET */
        s->nvflags |= val;
        break;
    case 0x3c: /* NVFLAGSCLR */
        s->nvflags &= ~val;
        break;
    case 0x40: /* RESETCTL */
        switch (board_id(s)) {
        case BOARD_ID_PB926:
            if (s->lockval == LOCK_VALUE) {
                s->resetlevel = val;
                if (val & 0x100) {
                    qemu_system_reset_request();
                }
            }
            break;
        case BOARD_ID_BRCM_NS:

            if (s->lockval == LOCK_VALUE) {
                s->resetlevel = val;
                if (val & 0x04) {
                    qemu_system_reset_request();
                }
            }
            break;
        }
        break;
    case 0x44: /* PCICTL */
        /* nothing to do.  */
        break;
    case 0x4c: /* FLASH */
        break;
    case 0x50: /* CLCD */
        switch (board_id(s)) {
        case BOARD_ID_PB926:
            /* On 926 bits 13:8 are R/O, bits 1:0 control
             * the mux that defines how to interpret the PL110
             * graphics format, and other bits are r/w but we
             * don't implement them to do anything.
             */
            s->sys_clcd &= 0x3f00;
            s->sys_clcd |= val & ~0x3f00;
            qemu_set_irq(s->pl110_mux_ctrl, val & 3);
            break;
        case BOARD_ID_EB:
            /* The EB is the same except that there is no mux since
             * the EB has a PL111.
             */
            s->sys_clcd &= 0x3f00;
            s->sys_clcd |= val & ~0x3f00;
            break;
//        case BOARD_ID_PBA8:
        case BOARD_ID_BRCM_NS:
            /* On PBA8 and PBX bit 7 is r/w and all other bits
             * are either r/o or RAZ/WI.
             */
            s->sys_clcd &= (1 << 7);
            s->sys_clcd |= val & ~(1 << 7);
            break;
        case BOARD_ID_VEXPRESS:
        default:
            /* On VExpress this register is unimplemented and will RAZ/WI */
            break;
        }
        break;
    case 0x54: /* CLCDSER */
    case 0x64: /* DMAPSR0 */
    case 0x68: /* DMAPSR1 */
    case 0x6c: /* DMAPSR2 */
    case 0x70: /* IOSEL */
    case 0x74: /* PLDCTL */
    case 0x80: /* BUSID */
    case 0x84: /* PROCID0 */
    case 0x88: /* PROCID1 */
    case 0x8c: /* OSCRESET0 */
    case 0x90: /* OSCRESET1 */
    case 0x94: /* OSCRESET2 */
    case 0x98: /* OSCRESET3 */
    case 0x9c: /* OSCRESET4 */
        break;
    case 0xa0: /* SYS_CFGDATA */
        if (board_id(s) != BOARD_ID_VEXPRESS) {
            goto bad_reg;
        }
        s->sys_cfgdata = val;
        return;
    case 0xa4: /* UART Clock Divider */

        /* Undefined bits [19:18] are RAZ/WI, and writing to
         * the start bit just triggers the action; it always reads
         * as zero.
         */
        s->sys_cfgctrl = val & ~((3 << 18) | (1 << 31));
        if (val & (1 << 31)) {
            /* Start bit set -- actually do something */
            unsigned int dcc = extract32(s->sys_cfgctrl, 26, 4);
            unsigned int function = extract32(s->sys_cfgctrl, 20, 6);
            unsigned int site = extract32(s->sys_cfgctrl, 16, 2);
            unsigned int position = extract32(s->sys_cfgctrl, 12, 4);
            unsigned int device = extract32(s->sys_cfgctrl, 0, 12);
            s->sys_cfgstat = 1;            /* complete */
            if (s->sys_cfgctrl & (1 << 30)) {
                if (!vexpress_cfgctrl_write(s, dcc, function, site, position,
                                            device, s->sys_cfgdata)) {
                    s->sys_cfgstat |= 2;        /* error */
                }
            } else {
                uint32_t val;
                if (!vexpress_cfgctrl_read(s, dcc, function, site, position,
                                           device, &val)) {
                    s->sys_cfgstat |= 2;        /* error */
                } else {
                    s->sys_cfgdata = val;
                }
            }
        }
        s->sys_cfgctrl &= ~(1 << 31);
        return;
    case 0xa8: /* SYS_CFGSTAT */
        if (board_id(s) != BOARD_ID_VEXPRESS) {
            goto bad_reg;
        }
        s->sys_cfgstat = val & 3;
        return;
#endif

    default:
    bad_reg:
            DB_PRINT("Bad register write %x <= %08x\n", (int)offset,
                     (unsigned)val);
        return;
    }
}

static const MemoryRegionOps chipcregs_ops = {
    .read = chipcregs_read,
    .write = chipcregs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


static void chipcregs_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sd = SYS_BUS_DEVICE(obj);
    Chipcregs_State *s = CHIPCREGS(obj);

    memory_region_init_io(&s->iomem, OBJECT(dev), &chipcregs_ops, s,
                          "chipcregs", 0x1000);
    sysbus_init_mmio(sd, &s->iomem);

}


static Property chipcregs_properties[] = {
    DEFINE_PROP_UINT32("chipid", Chipcregs_State, chipcregs.chipid, 0),
    DEFINE_PROP_UINT32("core_id", Chipcregs_State, core_id, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_chipcregs = {
    .name = "chipcregs",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(data, Chipcregs_State, 0x1000),
        VMSTATE_END_OF_LIST()
    }
};
static void chipcregs_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

 //   dc->realize = chipcregs_realize;
    dc->reset = chipcregs_reset;
    dc->vmsd = &vmstate_chipcregs;
    dc->props = chipcregs_properties;
}

static const TypeInfo chipcregs_info = {
    .name          = TYPE_CHIPCREGS,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Chipcregs_State),
    .instance_init = chipcregs_init,
//    .instance_finalize = chipcregs_finalize,
    .class_init    = chipcregs_class_init,
};

static void chipcregs_register_types(void)
{
    type_register_static(&chipcregs_info);
}

type_init(chipcregs_register_types)
