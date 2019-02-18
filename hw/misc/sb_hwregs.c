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
#include "sb_hwregs.h"

#define TYPE_SBHWREGS "sb_hwregs"
#define SBHWREGS(obj) \
    OBJECT_CHECK(SB_hwregs_State, (obj), TYPE_SBHWREGS)

#define SBHWREGS_ERR_DEBUG
#ifdef SBHWREGS_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif

typedef struct SB_hwregs_State {
    SysBusDevice parent_obj;

    MemoryRegion iomem;

/***SSB Registers***/
    union {
	struct {
	    struct _sbhwregs sbhwregs;
    };
    uint8_t data[0x100];
    };
} SB_hwregs_State;


static void sbhwregs_reset(DeviceState *d)
{
    SB_hwregs_State *s = SBHWREGS(d);
    DB_PRINT("****RESET****\n");
    s->sbhwregs.sbtmstatehigh = 0x1ff30000;  //mask 1fff0000 ---> core specific flags
    s->sbhwregs.sbimstate = 0;
//    s->sbhwregs.sbadmatch0 = 0x18000000;
    s->sbhwregs.sbtmstatelow = 0x3ffc0001;

}

static uint64_t sbhwregs_read_imp(void *opaque, hwaddr offset, unsigned size)
{
    SB_hwregs_State *s = (SB_hwregs_State *)opaque;

    switch (offset) {
    /***********Sonics Silicon Backplane************/
    case 0x0EA8: /* IM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0EB0: /* IM Error Log Backplane Revision >= 2.3 Only */ 
    case 0x0ED8: /* TM Port Connection ID 0 Backplane Revision >= 2.3 Only */ 
    case 0x0EF8: /* TM Port Lock 0 Backplane Revision >= 2.3 Only */
    case 0x0F08: /* IPS Flag */ 
    case 0x0018: /* TPS Flag */
	return 0x40;
    case 0x0F48: /* TM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0F50: /* TM Error Log Backplane Revision >= 2.3 Only */
    case 0x0060: /* Address Match 3 */
	return 	s->sbhwregs.sbadmatch3;
    case 0x0068: /* Address Match 2 */
	return 	s->sbhwregs.sbadmatch2;
    case 0x0070: /* Address Match 1 */
	return 	s->sbhwregs.sbadmatch1;
    case 0x0090: /* IM State */
	return s->sbhwregs.sbimstate;
    case 0x0f94: /* Interrupt Vector  */
    case 0x0098: /* TM State Low */
        return 0x00030001;
    case 0x0009C: /* TM State High */
	return s->sbhwregs.sbtmstatehigh;
    case 0x0FA0: /* BWA0 */
    case 0x0FA8: /* IM Config Low */
    case 0x0FAC: /* IM Config High */
    case 0x00B0: /* Address Match 0 */ 
	return 	s->sbhwregs.sbadmatch0;
    case 0x00B8: /* TM Config Low */
	return s->sbhwregs.sbtmstatelow;
    case 0x00BC: /* TM Config High */

    case 0x00C0: /* B Config */ 

    case 0x00C8: /* B State */

    case 0x0FD8: /* ACTCNFG */
    case 0x0FE8: /* FLAGST */
//        break;
    case 0x00F8: /* Core ID Low */
	return s->sbhwregs.sbidlow;///0x70000001;  //s->sbidlow
    case 0x00FC: /* Core ID High */
	return s->sbhwregs.sbidhigh;//0x4243a00F;//0x4243900f; 
    /***********CHIPCOMMON B************/
    case 0x1000: /* SPROM Shadow Area */
    default:
//    bad_reg:
	DB_PRINT("Bad register offset 0x%x\n", (int)offset);
        return 0;
    }
}

static uint64_t sbhwregs_read(void *opaque, hwaddr offset, unsigned size)
{
    uint32_t ret = sbhwregs_read_imp(opaque, offset, size);

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
static void sbhwregs_write(void *opaque, hwaddr offset,
                             uint64_t val, unsigned size)
{
    SB_hwregs_State *s = (SB_hwregs_State *)opaque;

    DB_PRINT("offset: %08x data: %08x\n", (unsigned)offset, (unsigned)val);

    switch (offset) {

   /***********Sonics Silicon Backplane************/
    case 0x0EA8: /* IM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0EB0: /* IM Error Log Backplane Revision >= 2.3 Only */ 
    case 0x0ED8: /* TM Port Connection ID 0 Backplane Revision >= 2.3 Only */ 
    case 0x0EF8: /* TM Port Lock 0 Backplane Revision >= 2.3 Only */
    case 0x0F08: /* IPS Flag */ 
    case 0x0018: /* TPS Flag */
    case 0x0F48: /* TM Error Log A Backplane Revision >= 2.3 Only */
    case 0x0F50: /* TM Error Log Backplane Revision >= 2.3 Only */
    case 0x060: /* Address Match 3 */
    case 0x068: /* Address Match 2 */
    case 0x070: /* Address Match 1 */
    case 0x090: /* IM State */
	s->sbhwregs.sbimstate=val;
	break;
    case 0x0F94: /* Interrupt Vector  */
    case 0x098: /* TM State Low */
	s->sbhwregs.sbtmstatelow=val;
	break;
    case 0x09C: /* TM State High */
	s->sbhwregs.sbtmstatehigh=val;
    case 0x0FA0: /* BWA0 */
    case 0x0FA8: /* IM Config Low */
    case 0x0FAC: /* IM Config High */
    case 0x0B0: /* Address Match 0 */ 
	s->sbhwregs.sbadmatch0=val;
	break;
    case 0x0FB8: /* TM Config Low */
    case 0x0FBC: /* TM Config High */
    case 0x0FC0: /* B Config */ 
    case 0x0FC8: /* B State */
    case 0x0FD8: /* ACTCNFG */
    case 0x0FE8: /* FLAGST */
    case 0x0FF8: /* Core ID Low */
    case 0x0FFC: /* Core ID High */
    /***********CHIPCOMMON B************/
    case 0x1000: /* SPROM Shadow Area */
	goto bad_reg;
    break;

    default:
    bad_reg:
            DB_PRINT("Bad register write %x <= %08x\n", (int)offset,
                     (unsigned)val);
        return;
    }
}

static const MemoryRegionOps sbhwregs_ops = {
    .read = sbhwregs_read,
    .write = sbhwregs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


static void sbhwregs_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sd = SYS_BUS_DEVICE(obj);
    SB_hwregs_State *s = SBHWREGS(obj);

    memory_region_init_io(&s->iomem, OBJECT(dev), &sbhwregs_ops, s,
                          "sb_hwregs", 0x100);
    sysbus_init_mmio(sd, &s->iomem);

}


static Property sbhwregs_properties[] = {
    DEFINE_PROP_UINT32("sbidhigh", SB_hwregs_State, sbhwregs.sbidhigh, 0),
    DEFINE_PROP_UINT32("sbidlow", SB_hwregs_State, sbhwregs.sbidlow, 0),
    DEFINE_PROP_UINT32("sbadmatch0", SB_hwregs_State, sbhwregs.sbadmatch0, 0),
    DEFINE_PROP_UINT32("sbadmatch1", SB_hwregs_State, sbhwregs.sbadmatch1, 0),
    DEFINE_PROP_UINT32("sbadmatch2", SB_hwregs_State, sbhwregs.sbadmatch2, 0),
    DEFINE_PROP_UINT32("sbadmatch3", SB_hwregs_State, sbhwregs.sbadmatch3, 0),

    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_sbhwregs = {
    .name = "sb_hwregs",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(data, SB_hwregs_State, 0x100),
        VMSTATE_END_OF_LIST()
    }
};
static void sbhwregs_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

 //   dc->realize = sbhwregs_realize;
    dc->reset = sbhwregs_reset;
    dc->vmsd = &vmstate_sbhwregs;
    dc->props = sbhwregs_properties;
}

static const TypeInfo sbhwregs_info = {
    .name          = TYPE_SBHWREGS,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SB_hwregs_State),
    .instance_init = sbhwregs_init,
//    .instance_finalize = sbhwregs_finalize,
    .class_init    = sbhwregs_class_init,
};

static void sbhwregs_register_types(void)
{
    type_register_static(&sbhwregs_info);
}

type_init(sbhwregs_register_types)
