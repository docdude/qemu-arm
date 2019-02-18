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
#include "nand_regs.h"
#include "hw/block/flash.h"
#include "sysemu/blockdev.h"

typedef struct NCregs_State {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    DeviceState *nand;
    uint8_t manf_id, chip_id;
    unsigned int rdy:1;
    unsigned int ale:1;
    unsigned int cle:1;
    unsigned int ce:1;

    ECCState ecc;
    union {
	struct {
	    struct nandregs ncregs;

    };
    uint8_t data[0x1000];
    };
}NCregs_State;

#define TYPE_NCREGS "ncregs"
#define NCREGS(obj) \
    OBJECT_CHECK(NCregs_State, (obj), TYPE_NCREGS)

//#define NCREGS_ERR_DEBUG
#ifdef NCREGS_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif
#define NFLASH_SUPPORT


static void ncregs_reset(DeviceState *d)
{
    NCregs_State *s = NCREGS(d);



	s->ncregs.revision=0x80000601;
	s->ncregs.cmd_start=0x0000000;
	s->ncregs.cmd_ext_address=0x00000000;
	s->ncregs.cmd_address=0x9FE00;
	s->ncregs.cmd_end_address=0x0;
	s->ncregs.intfc_status=0xF00000E0;
	s->ncregs.cs_nand_select=0x60000101;
	s->ncregs.cs_nand_xor=0x1;
	s->ncregs.ll_op=0x0;
	s->ncregs.mplane_base_ext_address=0x0;
	s->ncregs.mplane_base_address=0x0;	

	s->ncregs.acc_control_cs0=0xD7080010;
	s->ncregs.config_cs0=0x25142200;
	s->ncregs.timing_1_cs0=0x7695A56D;
	s->ncregs.timing_2_cs0=0x1EB7;
	s->ncregs.acc_control_cs1=0xD7080010;
	s->ncregs.config_cs1=0x12032200;
	s->ncregs.timing_1_cs1=0x7695A56D;
	s->ncregs.timing_2_cs1=0x1EB7;
	s->ncregs.corr_stat_threshold=0x41;
	s->ncregs.blk_wr_protect=0x0;
	s->ncregs.multiplane_opcodes_1=0xD1708010;
	s->ncregs.multiplane_opcodes_2=0x15780000;
	s->ncregs.multiplane_ctrl=0x0;
	s->ncregs.uncorr_error_count=0x1378;
	s->ncregs.corr_error_count=0;
	s->ncregs.read_error_count=0;
	s->ncregs.block_lock_status=0;
	s->ncregs.ecc_corr_ext_addr=0;
	s->ncregs.ecc_corr_addr=0;
	s->ncregs.ecc_unc_ext_addr=0;
	s->ncregs.ecc_unc_addr=0x7FE0600;
	s->ncregs.flash_read_ext_addr=0;
	s->ncregs.flash_read_addr=0;
	s->ncregs.program_page_ext_addr=0;
	s->ncregs.program_page_addr=0x9FE00;
	s->ncregs.copy_back_ext_addr=0;
	s->ncregs.copy_back_addr=0;
        s->ncregs.block_erase_ext_addr=0;
	s->ncregs.block_erase_addr=0x80000;
    	s->ncregs.inv_read_ext_addr=0;
	s->ncregs.inv_read_addr=0;
	s->ncregs.init_status=0x60800000;
	s->ncregs.onfi_status=0;
	s->ncregs.onfi_debug_data=0x15b3;
	s->ncregs.semaphore=0;

	s->ncregs.flash_device_id=0xC2F1801D;
	s->ncregs.flash_device_id_ext=0xC2F1801D;
	s->ncregs.ll_rddata=0x0;

	s->ncregs.spare_area_read_ofs[0]=0xFFFFBB30;
	s->ncregs.spare_area_read_ofs[1]=0x7460D857;
	s->ncregs.spare_area_read_ofs[2]=0x4217F886;
	s->ncregs.spare_area_read_ofs[3]=0xD5F7F58D;
 	s->ncregs.spare_area_read_ofs[4]=0xffffffff;
 	s->ncregs.spare_area_read_ofs[5]=0xffffffff;
	s->ncregs.spare_area_read_ofs[6]=0xffffffff;
	s->ncregs.spare_area_read_ofs[7]=0xffffffff;
	s->ncregs.spare_area_read_ofs[8]=0xffffffff;
	s->ncregs.spare_area_read_ofs[9]=0xffffffff;
	s->ncregs.spare_area_read_ofs[10]=0xffffffff;
	s->ncregs.spare_area_read_ofs[11]=0xffffffff;
	s->ncregs.spare_area_read_ofs[12]=0xffffffff;
	s->ncregs.spare_area_read_ofs[13]=0xffffffff;
 	s->ncregs.spare_area_read_ofs[14]=0xffffffff;
	s->ncregs.spare_area_read_ofs[15]=0xffffffff;

	s->ncregs.spare_area_write_ofs[0]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[1]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[2]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[3]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[4]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[5]=0xFFFFFFFF;
 	s->ncregs.spare_area_write_ofs[6]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[7]=0xFFFFFFFF;
 	s->ncregs.spare_area_write_ofs[8]=0xFFFFFFFF;
 	s->ncregs.spare_area_write_ofs[9]=0xFFFFFFFF;
 	s->ncregs.spare_area_write_ofs[10]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[11]=0xFFFFFFFF;
 	s->ncregs.spare_area_write_ofs[12]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[13]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[14]=0xFFFFFFFF;
	s->ncregs.spare_area_write_ofs[15]=0xFFFFFFFF;

	s->ncregs.flash_cache[0]=0xFF0400EA;
	s->ncregs.flash_cache[1]=0x14F09FE5;
 	s->ncregs.flash_cache[2]=0x14F09FE5;
 	s->ncregs.flash_cache[3]=0x14F09FE5;
	s->ncregs.flash_cache[4]=0x14F09FE5;
	s->ncregs.flash_cache[5]=0x14F09FE5;
	s->ncregs.flash_cache[6]=0x14F09FE5;
 	s->ncregs.flash_cache[7]=0x14F09FE5;
 	s->ncregs.flash_cache[8]=0x701D00C0;
	s->ncregs.flash_cache[9]=0x941D00C0;
 	s->ncregs.flash_cache[10]=0xB81D00C0;
	s->ncregs.flash_cache[11]=0xE81D00C0;
 	s->ncregs.flash_cache[12]=0x181E00C0;
 	s->ncregs.flash_cache[13]=0x481E00C0;
	s->ncregs.flash_cache[14]=0x781E00C0;
 	s->ncregs.flash_cache[15]=0x78563412;

	s->ncregs.direct_read_rd_miss=0x1;
	s->ncregs.block_erase_complete=0x1;
	s->ncregs.copy_back_complete=0x0;
	s->ncregs.program_page_complete=0x1;
	s->ncregs.no_ctlr_ready=0x1;
 	s->ncregs.nand_rb_b=0x1;
	s->ncregs.ecc_mips_uncorr=0x1;
	s->ncregs.ecc_mips_corr=0x0;

    	DB_PRINT("****RESET****\n");

}


static uint64_t ncregs_read_imp(void *opaque, hwaddr offset, unsigned size)
{
    NCregs_State *s = (NCregs_State *)opaque;
//NCore_State NCore_State;
	volatile unsigned int * const ai_ioctrl = (unsigned int *)0xfffd0408;
uint64_t r=0;
   int rdy;

    switch (offset) {
	case 0x000: /* revision */
	    return s->ncregs.revision;
	case 0x004: /* cmd_start */
	    return s->ncregs.cmd_start;
	case 0x008: /* cmd_ext_address */
	    return s->ncregs.cmd_ext_address;
	case 0x00c: /* cmd_address */
	    return s->ncregs.cmd_address;
	case 0x010: /* cmd_end_address */
	    return s->ncregs.cmd_end_address;
	case 0x014: /* intfc_status */

    ncore_getpins(s->nand, &rdy);
    s->rdy = rdy;
	    return s->ncregs.intfc_status;
	case 0x018: /* cs_nand_select */
	    return s->ncregs.cs_nand_select;
	case 0x01c: /* cs_nand_xor */
	    return s->ncregs.cs_nand_xor;
	case 0x020: /* ll_op */
	    return s->ncregs.ll_op;
	case 0x024: /* mplane_base_ext_address */
	    return s->ncregs.mplane_base_ext_address;
	case 0x028: /* mplane_base_address */
	    return s->ncregs.mplane_base_address;
	case 0x02c: /* PAD[9]-> 0x02C ~ 0x04C */

	case 0x050: /* acc_control_cs0 */
	    return s->ncregs.acc_control_cs0;
	case 0x054: /* config_cs0 */
	    return s->ncregs.config_cs0;
	case 0x058: /* timing_1_cs0 */
	    return s->ncregs.timing_1_cs0;
	case 0x05c: /* timing_2_cs0 */
	    return s->ncregs.timing_2_cs0;
	case 0x060: /* acc_control_cs1 */
	    return s->ncregs.acc_control_cs1;
	case 0x064: /* config_cs1 */
	    return s->ncregs.config_cs1;
	case 0x068: /* timing_1_cs1 */
	    return s->ncregs.timing_1_cs1;
	case 0x06c: /* timing_2_cs1 */
	    return s->ncregs.timing_2_cs1;
	case 0x070 ... 0x0bc: /* PAD[20] -> 0x070 ~ 0x0bc */

	case 0x0c0: /* corr_stat_threshold */
	    return s->ncregs.corr_stat_threshold;
	case 0x0c4: /* PAD[1] */

	case 0x0c8: /* blk_wr_protect */
	    return s->ncregs.blk_wr_protect;
	case 0x0cc: /* multiplane_opcodes_1 */
	    return s->ncregs.multiplane_opcodes_1;
	case 0x0d0: /* multiplane_opcodes_2 */
	    return s->ncregs.multiplane_opcodes_2;
	case 0x0d4: /* multiplane_ctrl */
	    return s->ncregs.multiplane_ctrl;
	case 0x0d8 ... 0x0dc: /* PAD[9] -> 0x0d8 ~ 0x0f8 */

	case 0x0fc: /* uncorr_error_count */
	    return s->ncregs.uncorr_error_count;
	case 0x100: /* corr_error_count */
	    return s->ncregs.corr_error_count;
	case 0x104: /* read_error_count */
	    return s->ncregs.read_error_count;
	case 0x108: /* block_lock_status */
	    return s->ncregs.block_lock_status;
	case 0x10c: /* ecc_corr_ext_addr */
	    return s->ncregs.ecc_corr_ext_addr;
	case 0x110: /* ecc_corr_addr */
	    return s->ncregs.ecc_corr_addr;
	case 0x114: /* ecc_unc_ext_addr */
	    return s->ncregs.ecc_unc_ext_addr;
	case 0x118: /* ecc_unc_addr */
	    return s->ncregs.ecc_unc_addr;
	case 0x11c: /* flash_read_ext_addr */
	    return s->ncregs.flash_read_ext_addr;
	case 0x120: /* flash_read_addr */
	    return s->ncregs.flash_read_addr;
	case 0x124: /* program_page_ext_addr */
	    return s->ncregs.program_page_ext_addr;
	case 0x128: /* program_page_addr */
	    return s->ncregs.program_page_addr;
	case 0x12c: /* copy_back_ext_addr */
	    return s->ncregs.copy_back_ext_addr;
	case 0x130: /* copy_back_addr */
	    return s->ncregs.copy_back_addr;
	case 0x134: /* block_erase_ext_addr */
	    return s->ncregs.block_erase_ext_addr;
	case 0x138: /* block_erase_addr */
	    return s->ncregs.block_erase_addr;
	case 0x13c: /* inv_read_ext_addr */
	    return s->ncregs.inv_read_ext_addr;
	case 0x140: /* inv_read_addr */
	    return s->ncregs.inv_read_addr;
	case 0x144: /* init_status */
	    return s->ncregs.init_status;
	case 0x148: /* onfi_status */
	    return s->ncregs.onfi_status;
	case 0x14c: /* onfi_debug_data */
	    return s->ncregs.onfi_debug_data;
	case 0x150: /* semaphore */
	    return s->ncregs.semaphore;
	case 0x154 ... 0x190: /* PAD[16]-> 0x154 ~ 0x190 */

	case 0x194: /* flash_device_id */
ncore_setpins(s->nand, s->cle=0, s->ale=0, s->ce=0, 1, 0);
            r = ncore_getio(s->nand) | ncore_getio(s->nand) << 8 | ncore_getio(s->nand) << 16 | ncore_getio(s->nand) << 24;
   DB_PRINT("addr_r: %08x data_r: %08x\n", (unsigned)offset, (unsigned)r);	
	    return  ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_device_id) : s->ncregs.flash_device_id;
	case 0x198: /* flash_device_id_ext */
	    return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_device_id_ext) : s->ncregs.flash_device_id_ext;
	case 0x19c: /* ll_rddata */
	    return s->ncregs.ll_rddata;
	case 0x1a0 ... 0x1fc: /* PAD[24] -> 0x1a0 ~ 0x1fc */

//	case 0x200 ... 0x23c: /* spare_area_read_ofs[16] -> 0x200 ~ 0x23c */

	case 0x200: 
ncore_setpins(s->nand, s->cle=0, s->ale=0, s->ce=0, 1, 0);
            r = ecc_digest(&s->ecc,ncore_getio(s->nand)) | ecc_digest(&s->ecc,ncore_getio(s->nand)) << 8 | ecc_digest(&s->ecc,ncore_getio(s->nand)) << 16 | ecc_digest(&s->ecc,ncore_getio(s->nand)) << 24;
   DB_PRINT("addr_r: %08x data_r: %08x\n", (unsigned)offset, (unsigned)r);
	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[0]=r) : (s->ncregs.spare_area_read_ofs[0]=r);
	case 0x204:
ncore_setpins(s->nand, s->cle=0, s->ale=0, s->ce=0, 1, 0);
            r = ncore_getio(s->nand) | ncore_getio(s->nand) << 8 | ncore_getio(s->nand) << 16 | ncore_getio(s->nand) << 24;
   DB_PRINT("addr_r: %08x data_r: %08x\n", (unsigned)offset, (unsigned)r);	
	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[1]=r) : (s->ncregs.spare_area_read_ofs[1]=r);
	case 0x208:
ncore_setpins(s->nand, s->cle=0, s->ale=0, s->ce=0, 1, 0);
            r = ncore_getio(s->nand) | ncore_getio(s->nand) << 8 | ncore_getio(s->nand) << 16 | ncore_getio(s->nand) << 24;
   DB_PRINT("addr_r: %08x data_r: %08x\n", (unsigned)offset, (unsigned)r);
	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[2]=r) : (s->ncregs.spare_area_read_ofs[2]=r);
	case 0x20c:
ncore_setpins(s->nand, s->cle=0, s->ale=0, s->ce=0, 1, 0);
            r = ncore_getio(s->nand) | ncore_getio(s->nand) << 8 | ncore_getio(s->nand) << 16 | ncore_getio(s->nand) << 24;
   DB_PRINT("addr_r: %08x data_r: %08x\n", (unsigned)offset, (unsigned)r);
	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[3]=r) : (s->ncregs.spare_area_read_ofs[3]=r);
	case 0x210:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[4]) : s->ncregs.spare_area_read_ofs[4];
	case 0x214:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[5]) : s->ncregs.spare_area_read_ofs[5];
	case 0x218:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[6]) : s->ncregs.spare_area_read_ofs[6];
	case 0x21c:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[7]) : s->ncregs.spare_area_read_ofs[7];
	case 0x220:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[8]) : s->ncregs.spare_area_read_ofs[8];
	case 0x224:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[9]) : s->ncregs.spare_area_read_ofs[9];
	case 0x228:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[10]) : s->ncregs.spare_area_read_ofs[10];
	case 0x22c:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[11]) : s->ncregs.spare_area_read_ofs[11];
	case 0x230:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[12]) : s->ncregs.spare_area_read_ofs[12];
	case 0x234:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[13]) : s->ncregs.spare_area_read_ofs[13];
	case 0x238:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[14]) : s->ncregs.spare_area_read_ofs[14];
	case 0x23c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_read_ofs[15]) : s->ncregs.spare_area_read_ofs[15];
	case 0x240 ... 0x243: /* PAD[16] -> 0x240 ~ 0x27c */

//	case 0x280..0x2bc: /* spare_area_write_ofs[16] -> 0x280 ~ 0x2bc */
	case 0x280: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[0]) : s->ncregs.spare_area_write_ofs[0];
	case 0x284: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[1]) : s->ncregs.spare_area_write_ofs[1];
	case 0x288: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[2]) : s->ncregs.spare_area_write_ofs[2];
	case 0x28c: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[3]) : s->ncregs.spare_area_write_ofs[3];
	case 0x290: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[4]) : s->ncregs.spare_area_write_ofs[4];
	case 0x294: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[5]) : s->ncregs.spare_area_write_ofs[5];
	case 0x298: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[6]) : s->ncregs.spare_area_write_ofs[6];
	case 0x29c: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[7]) : s->ncregs.spare_area_write_ofs[7];
	case 0x2a0: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[8]) : s->ncregs.spare_area_write_ofs[8];
	case 0x2a4: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[9]) : s->ncregs.spare_area_write_ofs[9];
	case 0x2a8: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[10]) : s->ncregs.spare_area_write_ofs[10];
	case 0x2ac: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[11]) : s->ncregs.spare_area_write_ofs[11];
	case 0x2b0: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[12]) : s->ncregs.spare_area_write_ofs[12];
	case 0x2b4: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[13]) : s->ncregs.spare_area_write_ofs[13];
	case 0x2b8: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[14]) : s->ncregs.spare_area_write_ofs[14];
	case 0x2bc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.spare_area_write_ofs[15]) : s->ncregs.spare_area_write_ofs[15];

	case 0x2c0 ... 0x3fc: /* PAD[80] -> 0x2c0 ~ 0x3fc */

//	case 0x400..0x5fc: /* flash_cache[128] -> 0x400~0x5fc */
	case 0x400: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[0]) : s->ncregs.flash_cache[0];
	case 0x404: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[1]) : s->ncregs.flash_cache[1];
	case 0x408: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[2]) : s->ncregs.flash_cache[2];
	case 0x40c: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[3]) : s->ncregs.flash_cache[3];
	case 0x410: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[4]) : s->ncregs.flash_cache[4];
	case 0x414: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[5]) : s->ncregs.flash_cache[5];
	case 0x418: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[6]) : s->ncregs.flash_cache[6];
	case 0x41c: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[7]) : s->ncregs.flash_cache[7];
	case 0x420: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[8]) : s->ncregs.flash_cache[8];
	case 0x424: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[9]) : s->ncregs.flash_cache[9];
	case 0x428: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[10]) : s->ncregs.flash_cache[10];
	case 0x42c: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[11]) : s->ncregs.flash_cache[11];
	case 0x430: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[12]) : s->ncregs.flash_cache[12];
	case 0x434: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[13]) : s->ncregs.flash_cache[13];
	case 0x438: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[14]) : s->ncregs.flash_cache[14];
	case 0x43c: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[15]) : s->ncregs.flash_cache[15];
	case 0x440: 	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[16]) : s->ncregs.flash_cache[16];
/*	case 0x444: 
 x=17;
for (n=0x444; n < 0x600; n+=0x04) {
	
printf("\tcase 0x%x:	return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[%d]) : s->ncregs.flash_cache[%d];\n",n,x,x);
x++;
}
return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[15]) : s->ncregs.flash_cache[15];
*/
       case 0x444:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[17]) : s->ncregs.flash_cache[17];
        case 0x448:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[18]) : s->ncregs.flash_cache[18];
        case 0x44c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[19]) : s->ncregs.flash_cache[19];
        case 0x450:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[20]) : s->ncregs.flash_cache[20];
        case 0x454:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[21]) : s->ncregs.flash_cache[21];
        case 0x458:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[22]) : s->ncregs.flash_cache[22];
        case 0x45c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[23]) : s->ncregs.flash_cache[23];
        case 0x460:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[24]) : s->ncregs.flash_cache[24];
        case 0x464:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[25]) : s->ncregs.flash_cache[25];
        case 0x468:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[26]) : s->ncregs.flash_cache[26];
        case 0x46c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[27]) : s->ncregs.flash_cache[27];
        case 0x470:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[28]) : s->ncregs.flash_cache[28];
        case 0x474:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[29]) : s->ncregs.flash_cache[29];
        case 0x478:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[30]) : s->ncregs.flash_cache[30];
        case 0x47c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[31]) : s->ncregs.flash_cache[31];
        case 0x480:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[32]) : s->ncregs.flash_cache[32];
        case 0x484:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[33]) : s->ncregs.flash_cache[33];
        case 0x488:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[34]) : s->ncregs.flash_cache[34];
        case 0x48c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[35]) : s->ncregs.flash_cache[35];
        case 0x490:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[36]) : s->ncregs.flash_cache[36];
        case 0x494:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[37]) : s->ncregs.flash_cache[37];
        case 0x498:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[38]) : s->ncregs.flash_cache[38];
        case 0x49c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[39]) : s->ncregs.flash_cache[39];
        case 0x4a0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[40]) : s->ncregs.flash_cache[40];
        case 0x4a4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[41]) : s->ncregs.flash_cache[41];
        case 0x4a8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[42]) : s->ncregs.flash_cache[42];
        case 0x4ac:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[43]) : s->ncregs.flash_cache[43];
        case 0x4b0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[44]) : s->ncregs.flash_cache[44];
        case 0x4b4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[45]) : s->ncregs.flash_cache[45];
        case 0x4b8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[46]) : s->ncregs.flash_cache[46];
        case 0x4bc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[47]) : s->ncregs.flash_cache[47];
        case 0x4c0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[48]) : s->ncregs.flash_cache[48];
        case 0x4c4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[49]) : s->ncregs.flash_cache[49];
        case 0x4c8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[50]) : s->ncregs.flash_cache[50];
        case 0x4cc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[51]) : s->ncregs.flash_cache[51];
        case 0x4d0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[52]) : s->ncregs.flash_cache[52];
        case 0x4d4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[53]) : s->ncregs.flash_cache[53];
        case 0x4d8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[54]) : s->ncregs.flash_cache[54];
        case 0x4dc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[55]) : s->ncregs.flash_cache[55];
        case 0x4e0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[56]) : s->ncregs.flash_cache[56];
        case 0x4e4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[57]) : s->ncregs.flash_cache[57];
        case 0x4e8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[58]) : s->ncregs.flash_cache[58];
        case 0x4ec:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[59]) : s->ncregs.flash_cache[59];
        case 0x4f0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[60]) : s->ncregs.flash_cache[60];
        case 0x4f4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[61]) : s->ncregs.flash_cache[61];
        case 0x4f8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[62]) : s->ncregs.flash_cache[62];
        case 0x4fc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[63]) : s->ncregs.flash_cache[63];
        case 0x500:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[64]) : s->ncregs.flash_cache[64];
        case 0x504:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[65]) : s->ncregs.flash_cache[65];
        case 0x508:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[66]) : s->ncregs.flash_cache[66];
        case 0x50c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[67]) : s->ncregs.flash_cache[67];
        case 0x510:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[68]) : s->ncregs.flash_cache[68];
        case 0x514:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[69]) : s->ncregs.flash_cache[69];
        case 0x518:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[70]) : s->ncregs.flash_cache[70];
        case 0x51c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[71]) : s->ncregs.flash_cache[71];
        case 0x520:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[72]) : s->ncregs.flash_cache[72];
        case 0x524:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[73]) : s->ncregs.flash_cache[73];
        case 0x528:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[74]) : s->ncregs.flash_cache[74];
        case 0x52c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[75]) : s->ncregs.flash_cache[75];
        case 0x530:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[76]) : s->ncregs.flash_cache[76];
        case 0x534:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[77]) : s->ncregs.flash_cache[77];
        case 0x538:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[78]) : s->ncregs.flash_cache[78];
        case 0x53c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[79]) : s->ncregs.flash_cache[79];
        case 0x540:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[80]) : s->ncregs.flash_cache[80];
        case 0x544:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[81]) : s->ncregs.flash_cache[81];
        case 0x548:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[82]) : s->ncregs.flash_cache[82];
        case 0x54c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[83]) : s->ncregs.flash_cache[83];
        case 0x550:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[84]) : s->ncregs.flash_cache[84];
        case 0x554:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[85]) : s->ncregs.flash_cache[85];
        case 0x558:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[86]) : s->ncregs.flash_cache[86];
        case 0x55c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[87]) : s->ncregs.flash_cache[87];
        case 0x560:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[88]) : s->ncregs.flash_cache[88];
        case 0x564:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[89]) : s->ncregs.flash_cache[89];
        case 0x568:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[90]) : s->ncregs.flash_cache[90];
        case 0x56c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[91]) : s->ncregs.flash_cache[91];
        case 0x570:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[92]) : s->ncregs.flash_cache[92];
        case 0x574:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[93]) : s->ncregs.flash_cache[93];
        case 0x578:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[94]) : s->ncregs.flash_cache[94];
        case 0x57c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[95]) : s->ncregs.flash_cache[95];
        case 0x580:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[96]) : s->ncregs.flash_cache[96];
        case 0x584:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[97]) : s->ncregs.flash_cache[97];
        case 0x588:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[98]) : s->ncregs.flash_cache[98];
        case 0x58c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[99]) : s->ncregs.flash_cache[99];
        case 0x590:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[100]) : s->ncregs.flash_cache[100];
        case 0x594:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[101]) : s->ncregs.flash_cache[101];
        case 0x598:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[102]) : s->ncregs.flash_cache[102];
        case 0x59c:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[103]) : s->ncregs.flash_cache[103];
        case 0x5a0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[104]) : s->ncregs.flash_cache[104];
        case 0x5a4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[105]) : s->ncregs.flash_cache[105];
        case 0x5a8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[106]) : s->ncregs.flash_cache[106];
        case 0x5ac:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[107]) : s->ncregs.flash_cache[107];
        case 0x5b0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[108]) : s->ncregs.flash_cache[108];
        case 0x5b4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[109]) : s->ncregs.flash_cache[109];
        case 0x5b8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[110]) : s->ncregs.flash_cache[110];
        case 0x5bc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[111]) : s->ncregs.flash_cache[111];
        case 0x5c0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[112]) : s->ncregs.flash_cache[112];
        case 0x5c4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[113]) : s->ncregs.flash_cache[113];
        case 0x5c8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[114]) : s->ncregs.flash_cache[114];
        case 0x5cc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[115]) : s->ncregs.flash_cache[115];
        case 0x5d0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[116]) : s->ncregs.flash_cache[116];
        case 0x5d4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[117]) : s->ncregs.flash_cache[117];
        case 0x5d8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[118]) : s->ncregs.flash_cache[118];
        case 0x5dc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[119]) : s->ncregs.flash_cache[119];
        case 0x5e0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[120]) : s->ncregs.flash_cache[120];
        case 0x5e4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[121]) : s->ncregs.flash_cache[121];
        case 0x5e8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[122]) : s->ncregs.flash_cache[122];
        case 0x5ec:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[123]) : s->ncregs.flash_cache[123];
        case 0x5f0:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[124]) : s->ncregs.flash_cache[124];
        case 0x5f4:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[125]) : s->ncregs.flash_cache[125];
        case 0x5f8:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[126]) : s->ncregs.flash_cache[126];
        case 0x5fc:     return ai_ioctrl ? __builtin_bswap32(s->ncregs.flash_cache[127]) : s->ncregs.flash_cache[127];

	case 0x600 ... 0xefc: /* PAD[576] -> 0x600 ~ 0xefc */

	case 0xf00: /* direct_read_rd_miss */
	    return s->ncregs.direct_read_rd_miss;
	case 0xf04: /* block_erase_complete */
	    return s->ncregs.block_erase_complete;
	case 0xf08: /* copy_back_complete */
	    return s->ncregs.copy_back_complete;
	case 0xf0c: /* program_page_complete */
	    return s->ncregs.program_page_complete;
	case 0xf10: /* no_ctlr_ready */
	    return s->ncregs.no_ctlr_ready;
	case 0xf14: /* nand_rb_b */
	    return s->ncregs.nand_rb_b;
	case 0xf18: /* ecc_mips_uncorr */
	    return s->ncregs.ecc_mips_uncorr;
	case 0xf1c: /* ecc_mips_corr */
	    return s->ncregs.ecc_mips_corr;
    default:
 //   bad_reg:
	DB_PRINT("Bad register offset 0x%x\n", (int)offset);
        return 0;
    }
}


static uint64_t ncregs_read(void *opaque, hwaddr offset, unsigned size)
{
    uint32_t ret = ncregs_read_imp(opaque, offset, size);

    DB_PRINT("addr: %08x data: %08x size:%08x\n", (unsigned)offset, (unsigned)ret, (unsigned)size);
    return ret;
}

static void ncregs_write(void *opaque, hwaddr offset,
                             uint64_t val, unsigned size)
{
    NCregs_State *s = (NCregs_State *)opaque;

    volatile unsigned int * const ai_ioctrl = (unsigned int *)0xfffd0408;
    DB_PRINT("offset: %08x data: %08x size:%08x\n", (unsigned)offset, (unsigned)val, (unsigned)size);
int rdy;
    ncore_setpins(s->nand, 0, 0, 0, 1, 0);
    switch (offset) {
	case 0x000: /* revision */
	    s->ncregs.revision=val;
	    break;
	case 0x004: /* cmd_start */

      //      ncore_setio(s->nand, val & 0xff);
      //      ncore_setio(s->nand, (val >> 8) & 0xff);
      //      ncore_setio(s->nand, (val >> 16) & 0xff);
      //      ncore_setio(s->nand, (val >> 24) & 0xff);
    ncore_cmdfunc(s->nand, val);

    ncore_getpins(s->nand, &rdy);
    s->rdy = rdy;

	    s->ncregs.cmd_start=val;
	    break;
	case 0x008: /* cmd_ext_address */
	    s->ncregs.cmd_ext_address=val;
	    break;
	case 0x00c: /* cmd_address */
    ncore_setpins(s->nand, s->cle=0, s->ale=1, s->ce=0, 1, 0);
  //  ncore_setio(s->nand, 0);
            ncore_setio(s->nand, val & 0xff);
            ncore_setio(s->nand, (val >> 8) & 0xff);
            ncore_setio(s->nand, (val >> 16) & 0xff);
       //     ncore_setio(s->nand, (val >> 24) & 0xff);
    ncore_setpins(s->nand, s->cle=1, s->ale=0, s->ce=0, 1, 0);
    uint8_t x[2] = {0x30, 0};
    cpu_physical_memory_write(0x18028004, x, 1);
    ncore_setio(s->nand, 0x30);
    ncore_getpins(s->nand, &rdy);
    s->rdy = rdy;
	    s->ncregs.cmd_address=val;
	    break;
	case 0x010: /* cmd_end_address */
	    s->ncregs.cmd_end_address=val;
	    break;
	case 0x014: /* intfc_status */
	    s->ncregs.intfc_status=val;
	    break;
	case 0x018: /* cs_nand_select */
	    s->ncregs.cs_nand_select=val;
	    break;
	case 0x01c: /* cs_nand_xor */
	    s->ncregs.cs_nand_xor=val;
	    break;
	case 0x020: /* ll_op */
	    s->ncregs.ll_op=val;
	    break;
	case 0x024: /* mplane_base_ext_address */
	    s->ncregs.mplane_base_ext_address=val;
	    break;
	case 0x028: /* mplane_base_address */
	    s->ncregs.mplane_base_address=val;
	    break;
	case 0x02c: /* PAD[9]-> 0x02C ~ 0x04C */

	case 0x050: /* acc_control_cs0 */
	    s->ncregs.acc_control_cs0=val;
	    break;
	case 0x054: /* config_cs0 */
	    s->ncregs.config_cs0=val;
	    break;
	case 0x058: /* timing_1_cs0 */
	    s->ncregs.timing_1_cs0=val;
	    break;
	case 0x05c: /* timing_2_cs0 */
	    s->ncregs.timing_2_cs0=val;
	    break;
	case 0x060: /* acc_control_cs1 */
	    s->ncregs.acc_control_cs1=val;
	    break;
	case 0x064: /* config_cs1 */
	    s->ncregs.config_cs1=val;
	    break;
	case 0x068: /* timing_1_cs1 */
	    s->ncregs.timing_1_cs1=val;
	    break;
	case 0x06c: /* timing_2_cs1 */
	    s->ncregs.timing_2_cs1=val;
	case 0x070 ... 0x0bc: /* PAD[20] -> 0x070 ~ 0x0bc */

	case 0x0c0: /* corr_stat_threshold */
	    s->ncregs.corr_stat_threshold=val;
	    break;
	case 0x0c4: /* PAD[1] */

	case 0x0c8: /* blk_wr_protect */
	    s->ncregs.blk_wr_protect=val;
	    break;
	case 0x0cc: /* multiplane_opcodes_1 */
	    s->ncregs.multiplane_opcodes_1=val;
	    break;
	case 0x0d0: /* multiplane_opcodes_2 */
	    s->ncregs.multiplane_opcodes_2=val;
	    break;
	case 0x0d4: /* multiplane_ctrl */
	    s->ncregs.multiplane_ctrl=val;
	    break;
	case 0x0d8 ... 0x0dc: /* PAD[9] -> 0x0d8 ~ 0x0f8 */
	case 0x0fc: /* uncorr_error_count */
	    s->ncregs.uncorr_error_count=val;
	    break;
	case 0x100: /* corr_error_count */
	    s->ncregs.corr_error_count=val;
	    break;
	case 0x104: /* read_error_count */
	    s->ncregs.read_error_count=val;
	    break;
	case 0x108: /* block_lock_status */
	    s->ncregs.block_lock_status=val;
	    break;
	case 0x10c: /* ecc_corr_ext_addr */
	    s->ncregs.ecc_corr_ext_addr=val;
	    break;	
	case 0x110: /* ecc_corr_addr */
	    s->ncregs.ecc_corr_addr=val;
	    break;
	case 0x114: /* ecc_unc_ext_addr */
	    s->ncregs.ecc_unc_ext_addr=val;
	    break;
	case 0x118: /* ecc_unc_addr */
	    s->ncregs.ecc_unc_addr=val;
	    break;
	case 0x11c: /* flash_read_ext_addr */
	    s->ncregs.flash_read_ext_addr=val;
	    break;
	case 0x120: /* flash_read_addr */
	    s->ncregs.flash_read_addr=val;
	    break;
	case 0x124: /* program_page_ext_addr */
	    s->ncregs.program_page_ext_addr=val;
	    break;
	case 0x128: /* program_page_addr */
	    s->ncregs.program_page_addr=val;
	    break;
	case 0x12c: /* copy_back_ext_addr */
	    s->ncregs.copy_back_ext_addr=val;
	    break;
	case 0x130: /* copy_back_addr */
	    s->ncregs.copy_back_addr=val;
	    break;
	case 0x134: /* block_erase_ext_addr */
	    s->ncregs.block_erase_ext_addr=val;
	    break;
	case 0x138: /* block_erase_addr */
	    s->ncregs.block_erase_addr=val;
	    break;
	case 0x13c: /* inv_read_ext_addr */
	    s->ncregs.inv_read_ext_addr=val;
	    break;
	case 0x140: /* inv_read_addr */
	    s->ncregs.inv_read_addr=val;
	    break;
	case 0x144: /* init_status */
	    s->ncregs.init_status=val;
	    break;
	case 0x148: /* onfi_status */
	    s->ncregs.onfi_status=val;
	    break;
	case 0x14c: /* onfi_debug_data */
	    s->ncregs.onfi_debug_data=val;
	    break;
	case 0x150: /* semaphore */
	    s->ncregs.semaphore=val;
	    break;
	case 0x154 ... 0x190: /* PAD[16]-> 0x154 ~ 0x190 */

	case 0x194: /* flash_device_id */
	case 0x198: /* flash_device_id_ext */
	case 0x19c: /* ll_rddata */
	case 0x1a0 ... 0x1fc: /* PAD[24] -> 0x1a0 ~ 0x1fc */

//	case 0x200..0x23c: /* spare_area_read_ofs[16] -> 0x200 ~ 0x23c */
	case 0x200: 	s->ncregs.spare_area_read_ofs[0]=val; 	    break;
	case 0x204:	s->ncregs.spare_area_read_ofs[1]=val;	    break;
	case 0x208:	s->ncregs.spare_area_read_ofs[2]=val;	    break;
	case 0x20c:	s->ncregs.spare_area_read_ofs[3]=val;	    break;
	case 0x210:     s->ncregs.spare_area_read_ofs[4]=val;	    break;
	case 0x214:	s->ncregs.spare_area_read_ofs[5]=val;	    break;
	case 0x218:     s->ncregs.spare_area_read_ofs[6]=val;	    break;
	case 0x21c:	s->ncregs.spare_area_read_ofs[7]=val;	    break;
	case 0x220:	s->ncregs.spare_area_read_ofs[8]=val;	    break;
	case 0x224:	s->ncregs.spare_area_read_ofs[9]=val;	    break;
	case 0x228:	s->ncregs.spare_area_read_ofs[10]=val;	    break;
	case 0x22c:	s->ncregs.spare_area_read_ofs[11]=val;	    break;
	case 0x230:	s->ncregs.spare_area_read_ofs[12]=val;	    break;
	case 0x234:	s->ncregs.spare_area_read_ofs[13]=val;	    break;
	case 0x238:	s->ncregs.spare_area_read_ofs[14]=val;	    break;
	case 0x23c:     s->ncregs.spare_area_read_ofs[15]=val;	    break;
	case 0x240 ... 0x27c: /* PAD[16] -> 0x240 ~ 0x27c */

	//case 0x280..0x2bc: /* spare_area_write_ofs[16] -> 0x280 ~ 0x2bc */
	case 0x280: 	ai_ioctrl ? val= __builtin_bswap32(val) : val; s->ncregs.spare_area_write_ofs[0]=val;	    break;
	case 0x284: 	s->ncregs.spare_area_write_ofs[1]=val;	    break;
	case 0x288: 	s->ncregs.spare_area_write_ofs[2]=val;	    break;
	case 0x28c: 	s->ncregs.spare_area_write_ofs[3]=val;	    break;
	case 0x290: 	s->ncregs.spare_area_write_ofs[4]=val;	    break;
	case 0x294: 	s->ncregs.spare_area_write_ofs[5]=val;	    break;
	case 0x298: 	s->ncregs.spare_area_write_ofs[6]=val;	    break;
	case 0x29c: 	s->ncregs.spare_area_write_ofs[7]=val;	    break;
	case 0x2a0: 	s->ncregs.spare_area_write_ofs[8]=val;	    break;
	case 0x2a4: 	s->ncregs.spare_area_write_ofs[9]=val;	    break;
	case 0x2a8: 	s->ncregs.spare_area_write_ofs[10]=val;	    break;
	case 0x2ac: 	s->ncregs.spare_area_write_ofs[11]=val;	    break;
	case 0x2b0: 	s->ncregs.spare_area_write_ofs[12]=val;	    break;
	case 0x2b4: 	s->ncregs.spare_area_write_ofs[13]=val;	    break;
	case 0x2b8: 	s->ncregs.spare_area_write_ofs[14]=val;	    break;
	case 0x2bc:     s->ncregs.spare_area_write_ofs[15]=val;	    break;
	case 0x2c0 ... 0x3fc: /* PAD[80] -> 0x2c0 ~ 0x3fc */

//	case 0x400..0x5fc: /* flash_cache[128] -> 0x400~0x5fc */
	case 0x400: 	s->ncregs.flash_cache[0]=val;	    break;
	case 0x404: 	s->ncregs.flash_cache[1]=val;	    break;
	case 0x408: 	s->ncregs.flash_cache[2]=val;	    break;
	case 0x40c: 	s->ncregs.flash_cache[3]=val;	    break;
	case 0x410: 	s->ncregs.flash_cache[4]=val;	    break;	 
	case 0x414: 	s->ncregs.flash_cache[5]=val;	    break;
	case 0x418: 	s->ncregs.flash_cache[6]=val;	    break;
	case 0x41c: 	s->ncregs.flash_cache[7]=val;	    break;
	case 0x420: 	s->ncregs.flash_cache[8]=val;	    break;
	case 0x424: 	s->ncregs.flash_cache[9]=val;	    break;
	case 0x428: 	s->ncregs.flash_cache[10]=val;	    break;
	case 0x42c: 	s->ncregs.flash_cache[11]=val;	    break;
	case 0x430: 	s->ncregs.flash_cache[12]=val;	    break;
	case 0x434: 	s->ncregs.flash_cache[13]=val;	    break;
	case 0x438: 	s->ncregs.flash_cache[14]=val;	    break;
	case 0x43c: 	s->ncregs.flash_cache[15]=val;	    break;
	case 0x600 ... 0xefc: /* PAD[576] -> 0x600 ~ 0xefc */

	case 0xf00: /* direct_read_rd_miss */
	    s->ncregs.direct_read_rd_miss=val;
	    break;
	case 0xf04: /* block_erase_complete */
	    s->ncregs.block_erase_complete=val;
	    break;
	case 0xf08: /* copy_back_complete */
	    s->ncregs.copy_back_complete=val;
	    break;
	case 0xf0c: /* program_page_complete */
	    s->ncregs.program_page_complete=val;
	    break;
	case 0xf10: /* no_ctlr_ready */
	    s->ncregs.no_ctlr_ready=val;
	    break;
	case 0xf14: /* nand_rb_b */
	    s->ncregs.nand_rb_b=val;
	    break;
	case 0xf18: /* ecc_mips_uncorr */
	    s->ncregs.ecc_mips_uncorr=val;
	    break;
	case 0xf1c: /* ecc_mips_corr */
	    s->ncregs.ecc_mips_corr=val;
	    break;
    default:
 //   bad_reg:
            DB_PRINT("Bad register write %x <= %08x\n", (int)offset,
                     (unsigned)val);
        return;
    }
}


static const MemoryRegionOps ncregs_ops = {
    .read = ncregs_read,
    .write = ncregs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


static void ncregs_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sd = SYS_BUS_DEVICE(obj);
    NCregs_State *s = NCREGS(obj);
   DriveInfo *nand = drive_get(IF_MTD, 0, 0); 

    if (!nand) {
        hw_error("%s: NAND image required", __FUNCTION__);
    }
 s->nand = ncore_init(nand ? nand->bdrv : NULL, NAND_MFR_MXIC, 0xf1);
    memory_region_init_io(&s->iomem, OBJECT(dev), &ncregs_ops, s,
                          "ncregs", 0x1000);
    sysbus_init_mmio(sd, &s->iomem);

}
/*
static int ncregs_init(SysBusDevice *dev)
{

    NCregs_State *s = NCREGS(dev);
   DriveInfo *nand = drive_get(IF_MTD, 0, 0); 

    if (!nand) {
        hw_error("%s: NAND image required", __FUNCTION__);
    }
    s->nand = nand_init(nand ? nand->bdrv : NULL, NAND_MFR_MXIC, 0xf1);

    memory_region_init_io(&s->iomem, OBJECT(s), &ncregs_ops, s,
                          "ncregs", 0x1000);
    sysbus_init_mmio(dev, &s->iomem);
return 0;
}
*/
static Property ncregs_properties[] = {
    DEFINE_PROP_UINT8("manf_id", NCregs_State, manf_id, 0),
    DEFINE_PROP_UINT8("chip_id", NCregs_State, chip_id, 0),
 //   DEFINE_PROP_DRIVE("drive", NCregs_State, bdrv),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_ncregs = {
    .name = "ncregs",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(data, NCregs_State, 0x1000),
        VMSTATE_END_OF_LIST()
    }
};

static void ncregs_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
//   SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

//    k->init = ncregs_init;
//    dc->realize = ncregs_realize;
    dc->reset = ncregs_reset;
    dc->vmsd = &vmstate_ncregs;
    dc->props = ncregs_properties;
}

static const TypeInfo ncregs_info = {
    .name          = TYPE_NCREGS,
    .parent        = TYPE_SYS_BUS_DEVICE,
 //   .parent        = TYPE_DEVICE,
    .instance_size = sizeof(NCregs_State),
    .instance_init = ncregs_init,

    .class_init    = ncregs_class_init,
};

static void ncregs_register_types(void)
{
    type_register_static(&ncregs_info);
}




type_init(ncregs_register_types)

