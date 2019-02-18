/*
 * Broadcom SiliconBackplane hardware register definitions.
 * NAND Controller Interface hardware definitions.
 *
 * Copyright (c) 2014 Just Playing Around.
 * Written by Juan F. Loya, MD
 *
 * This code is licensed under the GPL.
 */

#ifndef NAND_IO

#include "hw/hw.h"
#include "qemu/timer.h"
#include "qemu/bitops.h"
#include "hw/sysbus.h"
#include "sysemu/sysemu.h"
#include "nand_core.h"
#include "hw/block/flash.h"
#include "sysemu/blockdev.h"

# define NAND_CMD_READ0		0x00
# define NAND_CMD_READ1		0x01
# define NAND_CMD_READ2		0x50
# define NAND_CMD_LPREAD2	0x30
# define NAND_CMD_READCACHESTART 0x31
# define NAND_CMD_READCACHEEXIT  0x34
# define NAND_CMD_READCACHELAST  0x3f
# define NAND_CMD_NOSERIALREAD2	0x35
# define NAND_CMD_RANDOMREAD1	0x05
# define NAND_CMD_RANDOMREAD2	0xe0
# define NAND_CMD_READID	0x90
# define NAND_CMD_RESET		0xff
# define NAND_CMD_PAGEPROGRAM1	0x80
# define NAND_CMD_PAGEPROGRAM2	0x10
# define NAND_CMD_CACHEPROGRAM2	0x15
# define NAND_CMD_BLOCKERASE1	0x60
# define NAND_CMD_BLOCKERASE2	0xd0
# define NAND_CMD_READSTATUS	0x70
# define NAND_CMD_COPYBACKPRG1	0x85




# define MAX_PAGE		0x800
# define MAX_OOB		0x40
typedef struct NCore_State NCore_State;
struct NCore_State {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    DeviceState *nand;
    uint8_t manf_id, chip_id;
    uint8_t buswidth; /* in BYTES */
    int size, pages;
    int page_shift, oob_shift, erase_shift, addr_shift;
    uint8_t *storage;
    BlockDriverState *bdrv;
    int mem_oob;

    uint8_t cle, ale, ce, wp, gnd;

    uint8_t io[MAX_PAGE + MAX_OOB + 0x400];
    uint8_t *ioaddr;
    int iolen;

    uint32_t cmd;
    uint64_t addr;
    int addrlen;
    int status;
    int offset;

    void (*blk_write)(NCore_State *s);
    void (*blk_erase)(NCore_State *s);
    void (*blk_load)(NCore_State *s, uint64_t addr, int offset);

    uint32_t ioaddr_vmstate;
    ECCState ecc;
    union {
	struct {
	    struct nandregs ncregs;

    };
    uint8_t data[0x1000];
    };
};

#define TYPE_NCORE "ncore"
#define NCORE(obj) \
    OBJECT_CHECK(NCore_State, (obj), TYPE_NCORE)

//#define NCORE_ERR_DEBUG
#ifdef NCORE_ERR_DEBUG
#define DB_PRINT(...) do { \
    fprintf(stderr,  ": %s: ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif
#define NFLASH_SUPPORT

static void mem_and(uint8_t *dest, const uint8_t *src, size_t n)
{
    /* Like memcpy() but we logical-AND the data into the destination */
    int i;
    for (i = 0; i < n; i++) {
        dest[i] &= src[i];
    }
}

# define NAND_NO_AUTOINCR	0x00000001
# define NAND_BUSWIDTH_16	0x00000002
# define NAND_NO_PADDING	0x00000004
# define NAND_CACHEPRG		0x00000008
# define NAND_COPYBACK		0x00000010
# define NAND_IS_AND		0x00000020
# define NAND_4PAGE_ARRAY	0x00000040
# define NAND_NO_READRDY	0x00000100
# define NAND_SAMSUNG_LP	(NAND_NO_PADDING | NAND_COPYBACK)

# define NAND_IO

# define PAGE(addr)		((addr) >> ADDR_SHIFT)
# define PAGE_START(page)	(PAGE(page) * (PAGE_SIZE + OOB_SIZE))
# define PAGE_MASK		((1 << ADDR_SHIFT) - 1)
# define OOB_SHIFT		(PAGE_SHIFT - 5)
# define OOB_SIZE		(1 << OOB_SHIFT)
# define SECTOR(addr)		((addr) >> (9 + ADDR_SHIFT - PAGE_SHIFT))
# define SECTOR_OFFSET(addr)	((addr) & ((511 >> PAGE_SHIFT) << 8))

# define PAGE_SIZE		256
# define PAGE_SHIFT		8
# define PAGE_SECTORS		1
# define ADDR_SHIFT		8
# include "nand_core.c"
# define PAGE_SIZE		512
# define PAGE_SHIFT		9
# define PAGE_SECTORS		1
# define ADDR_SHIFT		8
# include "nand_core.c"
# define PAGE_SIZE		2048
# define PAGE_SHIFT		11
# define PAGE_SECTORS		4
# define ADDR_SHIFT		16
# include "nand_core.c"

/* Information based on Linux drivers/mtd/nand/nand_ids.c */
static const struct {
    int size;
    int width;
    int page_shift;
    int erase_shift;
    uint32_t options;
} nand_flash_ids[0x100] = {
    [0 ... 0xff] = { 0 },

    [0x6e] = { 1,	8,	8, 4, 0 },
    [0x64] = { 2,	8,	8, 4, 0 },
    [0x6b] = { 4,	8,	9, 4, 0 },
    [0xe8] = { 1,	8,	8, 4, 0 },
    [0xec] = { 1,	8,	8, 4, 0 },
    [0xea] = { 2,	8,	8, 4, 0 },
    [0xd5] = { 4,	8,	9, 4, 0 },
    [0xe3] = { 4,	8,	9, 4, 0 },
    [0xe5] = { 4,	8,	9, 4, 0 },
    [0xd6] = { 8,	8,	9, 4, 0 },

    [0x39] = { 8,	8,	9, 4, 0 },
    [0xe6] = { 8,	8,	9, 4, 0 },
    [0x49] = { 8,	16,	9, 4, NAND_BUSWIDTH_16 },
    [0x59] = { 8,	16,	9, 4, NAND_BUSWIDTH_16 },

    [0x33] = { 16,	8,	9, 5, 0 },
    [0x73] = { 16,	8,	9, 5, 0 },
    [0x43] = { 16,	16,	9, 5, NAND_BUSWIDTH_16 },
    [0x53] = { 16,	16,	9, 5, NAND_BUSWIDTH_16 },

    [0x35] = { 32,	8,	9, 5, 0 },
    [0x75] = { 32,	8,	9, 5, 0 },
    [0x45] = { 32,	16,	9, 5, NAND_BUSWIDTH_16 },
    [0x55] = { 32,	16,	9, 5, NAND_BUSWIDTH_16 },

    [0x36] = { 64,	8,	9, 5, 0 },
    [0x76] = { 64,	8,	9, 5, 0 },
    [0x46] = { 64,	16,	9, 5, NAND_BUSWIDTH_16 },
    [0x56] = { 64,	16,	9, 5, NAND_BUSWIDTH_16 },

    [0x78] = { 128,	8,	9, 5, 0 },
    [0x39] = { 128,	8,	9, 5, 0 },
    [0x79] = { 128,	8,	9, 5, 0 },
    [0x72] = { 128,	16,	9, 5, NAND_BUSWIDTH_16 },
    [0x49] = { 128,	16,	9, 5, NAND_BUSWIDTH_16 },
    [0x74] = { 128,	16,	9, 5, NAND_BUSWIDTH_16 },
    [0x59] = { 128,	16,	9, 5, NAND_BUSWIDTH_16 },

    [0x71] = { 256,	8,	9, 5, 0 },

    /*
     * These are the new chips with large page size. The pagesize and the
     * erasesize is determined from the extended id bytes
     */
# define LP_OPTIONS	(NAND_SAMSUNG_LP | NAND_NO_READRDY | NAND_NO_AUTOINCR)
# define LP_OPTIONS16	(LP_OPTIONS | NAND_BUSWIDTH_16)

    /* 512 Megabit */
    [0xa2] = { 64,	8,	0, 0, LP_OPTIONS },
    [0xf2] = { 64,	8,	0, 0, LP_OPTIONS },
    [0xb2] = { 64,	16,	0, 0, LP_OPTIONS16 },
    [0xc2] = { 64,	16,	0, 0, LP_OPTIONS16 },

    /* 1 Gigabit */
    [0xa1] = { 128,	8,	0, 0, LP_OPTIONS },
    [0xf1] = { 128,	8,	0, 0, LP_OPTIONS },
    [0xb1] = { 128,	16,	0, 0, LP_OPTIONS16 },
    [0xc1] = { 128,	16,	0, 0, LP_OPTIONS16 },

    /* 2 Gigabit */
    [0xaa] = { 256,	8,	0, 0, LP_OPTIONS },
    [0xda] = { 256,	8,	0, 0, LP_OPTIONS },
    [0xba] = { 256,	16,	0, 0, LP_OPTIONS16 },
    [0xca] = { 256,	16,	0, 0, LP_OPTIONS16 },

    /* 4 Gigabit */
    [0xac] = { 512,	8,	0, 0, LP_OPTIONS },
    [0xdc] = { 512,	8,	0, 0, LP_OPTIONS },
    [0xbc] = { 512,	16,	0, 0, LP_OPTIONS16 },
    [0xcc] = { 512,	16,	0, 0, LP_OPTIONS16 },

    /* 8 Gigabit */
    [0xa3] = { 1024,	8,	0, 0, LP_OPTIONS },
    [0xd3] = { 1024,	8,	0, 0, LP_OPTIONS },
    [0xb3] = { 1024,	16,	0, 0, LP_OPTIONS16 },
    [0xc3] = { 1024,	16,	0, 0, LP_OPTIONS16 },

    /* 16 Gigabit */
    [0xa5] = { 2048,	8,	0, 0, LP_OPTIONS },
    [0xd5] = { 2048,	8,	0, 0, LP_OPTIONS },
    [0xb5] = { 2048,	16,	0, 0, LP_OPTIONS16 },
    [0xc5] = { 2048,	16,	0, 0, LP_OPTIONS16 },
};

static void ncore_reset(DeviceState *d)
{
    NCore_State *s = NCORE(d);


    s->cmd = NANDCMD_NULL;
    s->addr = 0;
    s->addrlen = 0;
    s->iolen = 0;
    s->offset = 0;
//    s->status &= NAND_IOSTATUS_UNPROTCT;
    s->status |= NANDIST_CTRL_READY | NANDIST_FLASH_READY;

	s->ncregs.revision=0x80000601;
	s->ncregs.cmd_start=0x0000000;
	s->ncregs.cmd_ext_address=0x1c000000;
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

static inline void ncore_pushio_byte(NCore_State *s, uint8_t value)
{
    s->ioaddr[s->iolen++] = value;
    for (value = s->buswidth; --value;) {
        s->ioaddr[s->iolen++] = 0;
    }
}


static void ncore_command(NCore_State *s)
{
    unsigned int offset;
    switch (s->cmd) {
    case NAND_CMD_READ0:
    case NAND_CMD_READCACHEEXIT:
        s->iolen = 0;
        break;

    case NAND_CMD_READID:
        s->ioaddr = s->io;
        s->iolen = 0;
        ncore_pushio_byte(s, s->manf_id);
        ncore_pushio_byte(s, s->chip_id);
        ncore_pushio_byte(s, 0x80); /* Don't-care byte (often 0xa5) */
        if (nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP) {
            /* Page Size, Block Size, Spare Size; bit 6 indicates
             * 8 vs 16 bit width NAND.
             */
            ncore_pushio_byte(s, (s->buswidth == 2) ? 0x55 : 0x1d);
        } else {
            ncore_pushio_byte(s, 0xc0); /* Multi-plane */
        }
        break;

    case NAND_CMD_RANDOMREAD2:
    case NAND_CMD_NOSERIALREAD2:
        if (!(nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP))
            break;
        offset = s->addr & ((1 << s->addr_shift) - 1);
        s->blk_load(s, s->addr, offset);
        if (s->gnd)
            s->iolen = (1 << s->page_shift) - offset;
        else
            s->iolen = (1 << s->page_shift) + (1 << s->oob_shift) - offset;
        break;

    case NAND_CMD_RESET:
        ncore_reset(DEVICE(s));
        break;

    case NAND_CMD_PAGEPROGRAM1:
        s->ioaddr = s->io;
        s->iolen = 0;
        break;

    case NAND_CMD_PAGEPROGRAM2:
        if (s->wp) {
            s->blk_write(s);
        }
        break;

    case NAND_CMD_BLOCKERASE1:
        break;

    case NAND_CMD_BLOCKERASE2:
        s->addr &= (1ull << s->addrlen * 8) - 1;
        s->addr <<= nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP ?
                                                                    16 : 8;

        if (s->wp) {
            s->blk_erase(s);
        }
        break;

    case NAND_CMD_READSTATUS:
        s->ioaddr = s->io;
        s->iolen = 0;
        ncore_pushio_byte(s, s->status);
        break;

    default:
        printf("%s: Unknown NAND command 0x%02x\n", __FUNCTION__, s->cmd);
    }
}

void ncore_cmdfunc(DeviceState *dev, uint32_t value)
{
    NCore_State *s = NCORE(dev);
    ncore_setpins(dev, s->cle=1, s->ale=0, s->ce=0, 1, 0);
    switch (value) {
//    case NANDCMD_NULL:
    case NANDCMD_PAGE_RD:
DB_PRINT("[**%s**] cle=%x ale=%x ce=%x wp=%x gnd=%x ****s->addr=%lx\n", __func__,(int) s->cle, s->ale,s->ce, s->wp, s->gnd, s->addr);
        s->cmd = NAND_CMD_READ0;
        ncore_setio(dev, s->cmd);
//        ncore_setpins(dev, s->cle=0, s->ale=1, s->ce=0, 1, 0);
//        ncore_setio(dev, 0);
        break;

    case NANDCMD_ID_RD:
        s->cmd = NAND_CMD_READID;
        ncore_setio(dev, s->cmd);
        ncore_setpins(dev, s->cle=0, s->ale=1, s->ce=0, 1, 0);
        ncore_setio(dev, 0);
        break;
    case NANDCMD_SPARE_RD:
 //   case NAND_CMD_RANDOMREAD2:
//    case NAND_CMD_NOSERIALREAD2:
       
        break;

    case NANDCMD_FLASH_RESET:
        s->cmd = NAND_CMD_RESET;
        break;

    case NANDCMD_PAGE_PROG:
        s->cmd = NAND_CMD_PAGEPROGRAM1;
        break;

    case NANDCMD_SPARE_PROG:
        s->cmd = 0;
        break;

 //   case NAND_CMD_BLOCKERASE1:
 //       break;

    case NANDCMD_BLOCK_ERASE:
        s->cmd = NAND_CMD_BLOCKERASE1;
        break;

    case NANDCMD_STATUS_RD:
        s->cmd = NAND_CMD_READSTATUS;
        break;
    case NAND_CMD_LPREAD2:
        s->cmd = NAND_CMD_READ0;
        break;
    default:
        printf("%s: Unknown NAND command 0x%02x\n", __FUNCTION__, value);
    }
}
static void ncore_pre_save(void *opaque)
{
    NCore_State *s = NCORE(opaque);

    s->ioaddr_vmstate = s->ioaddr - s->io;
}

static int ncore_post_load(void *opaque, int version_id)
{
    NCore_State *s = NCORE(opaque);

    if (s->ioaddr_vmstate > sizeof(s->io)) {
        return -EINVAL;
    }
    s->ioaddr = s->io + s->ioaddr_vmstate;

    return 0;
}

static const VMStateDescription vmstate_ncore = {
    .name = "ncore",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .pre_save = ncore_pre_save,
    .post_load = ncore_post_load,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT8(cle, NCore_State),
        VMSTATE_UINT8(ale, NCore_State),
        VMSTATE_UINT8(ce, NCore_State),
        VMSTATE_UINT8(wp, NCore_State),
        VMSTATE_UINT8(gnd, NCore_State),
        VMSTATE_BUFFER(io, NCore_State),
        VMSTATE_UINT32(ioaddr_vmstate, NCore_State),
        VMSTATE_INT32(iolen, NCore_State),
        VMSTATE_UINT32(cmd, NCore_State),
        VMSTATE_UINT64(addr, NCore_State),
        VMSTATE_INT32(addrlen, NCore_State),
        VMSTATE_INT32(status, NCore_State),
        VMSTATE_INT32(offset, NCore_State),
        /* XXX: do we want to save s->storage too? */
        VMSTATE_UINT8_ARRAY(data, NCore_State, 0x1000),
        VMSTATE_END_OF_LIST()
    }
};


static void ncore_realize(DeviceState *dev, Error **errp)
{
    int pagesize;
    NCore_State *s = NCORE(dev);

    s->buswidth = nand_flash_ids[s->chip_id].width >> 3;
    s->size = nand_flash_ids[s->chip_id].size << 20;
    if (nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP) {
        s->page_shift = 11;
        s->erase_shift = 6;
    } else {
        s->page_shift = nand_flash_ids[s->chip_id].page_shift;
        s->erase_shift = nand_flash_ids[s->chip_id].erase_shift;
    }

    switch (1 << s->page_shift) {
    case 256:
        ncore_init_256(s);
        break;
    case 512:
        ncore_init_512(s);
        break;
    case 2048:
        ncore_init_2048(s);
        break;
    default:
        error_setg(errp, "Unsupported NAND block size %#x\n",
                   1 << s->page_shift);
        return;
    }

    pagesize = 1 << s->oob_shift;  //0x40(64)
    s->mem_oob = 1;
    if (s->bdrv) {
        if (bdrv_is_read_only(s->bdrv)) {
            error_setg(errp, "Can't use a read-only drive");
            return;
        }
        if (bdrv_getlength(s->bdrv) >=
                (s->pages << s->page_shift) + (s->pages << s->oob_shift)) {
            pagesize = 0;
            s->mem_oob = 0;
        }
    } else {
        pagesize += 1 << s->page_shift;
    }
    if (pagesize) {
        s->storage = (uint8_t *) memset(g_malloc(s->pages * pagesize),
                        0xff, s->pages * pagesize);
    }
    /* Give s->ioaddr a sane value in case we save state before it is used. */
    s->ioaddr = s->io;
DB_PRINT("%s\n size=%d\n pages=%d\n,page_shift=%d\n oob_shift=%d\n buswidth=%d\n chipid=%x\n pagesize=%d\n bdrv=%d\n storage=%lx\n mem_oob=%d\n" , __func__,s->size, s->pages, s->page_shift,s->oob_shift, s->buswidth, s->chip_id, pagesize, (uint)bdrv_getlength(s->bdrv), (int long)s->storage, s->mem_oob);
}



static Property ncore_properties[] = {
    DEFINE_PROP_UINT8("manf_id", NCore_State, manf_id, 0),
    DEFINE_PROP_UINT8("chip_id", NCore_State, chip_id, 0),
    DEFINE_PROP_DRIVE("drive", NCore_State, bdrv),
    DEFINE_PROP_END_OF_LIST(),
};


static void ncore_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
//   SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

//    k->init = ncore_init;
    dc->realize = ncore_realize;
    dc->reset = ncore_reset;
    dc->vmsd = &vmstate_ncore;
    dc->props = ncore_properties;
}

static const TypeInfo ncore_info = {
    .name          = TYPE_NCORE,
 //   .parent        = TYPE_SYS_BUS_DEVICE,
    .parent        = TYPE_DEVICE,
    .instance_size = sizeof(NCore_State),
 //   .instance_init = ncore_init,

    .class_init    = ncore_class_init,
};

static void ncore_register_types(void)
{
    type_register_static(&ncore_info);
}

/*
 * Chip inputs are CLE, ALE, CE, WP, GND and eight I/O pins.  Chip
 * outputs are R/B and eight I/O pins.
 *
 * CE, WP and R/B are active low.
 */
void ncore_setpins(DeviceState *dev, uint8_t cle, uint8_t ale,
                  uint8_t ce, uint8_t wp, uint8_t gnd)
{
    NCore_State *s = NCORE(dev);

    s->cle = cle;
    s->ale = ale;
    s->ce = ce;
    s->wp = wp;
    s->gnd = gnd;
    if (wp) {
        s->status |= NANDIST_CTRL_READY;
    } else {
        s->status &= ~NANDIST_CTRL_READY;
    }
}

void ncore_getpins(DeviceState *dev, int *rb)
{
    *rb = 1;
}


void ncore_setio(DeviceState *dev, uint32_t value)
{
    int i;
    NCore_State *s = NCORE(dev);
DB_PRINT("[**%s**] cle=%x ale=%x ce=%x wp=%x gnd=%x ****s->addr=%" PRIx64 "\n", __func__,(int) s->cle, s->ale,s->ce, s->wp, s->gnd, s->addr);
    if (!s->ce && s->cle) {
        if (nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP) {
            if (s->cmd == NAND_CMD_READ0
                && (value == NAND_CMD_LPREAD2
                    || value == NAND_CMD_READCACHESTART
                    || value == NAND_CMD_READCACHELAST))
                return;
            if (value == NAND_CMD_RANDOMREAD1) {
                s->addr &= ~((1 << s->addr_shift) - 1);
                s->addrlen = 0;
                return;
            }
        }
        if (value == NAND_CMD_READ0) {
            s->offset = 0;
        } else if (value == NAND_CMD_READ1) {
            s->offset = 0x100;
            value = NAND_CMD_READ0;
        } else if (value == NAND_CMD_READ2) {
            s->offset = 1 << s->page_shift;
            value = NAND_CMD_READ0;
        }

        s->cmd = value;

        if (s->cmd == NAND_CMD_READSTATUS ||
                s->cmd == NAND_CMD_PAGEPROGRAM2 ||
                s->cmd == NAND_CMD_BLOCKERASE1 ||
                s->cmd == NAND_CMD_BLOCKERASE2 ||
                s->cmd == NAND_CMD_NOSERIALREAD2 ||
                s->cmd == NAND_CMD_RANDOMREAD2 ||
                s->cmd == NAND_CMD_RESET ||
                s->cmd == NAND_CMD_READCACHEEXIT) {
            ncore_command(s);
        }

        if (s->cmd != NAND_CMD_RANDOMREAD2) {
            s->addrlen = 0;
        }
    }
DB_PRINT("[**%s**] s->addr=%" PRIx64 " s->offset=%x s->addrlen=%d value%x cmd=0x%" PRIx32 "\n", __func__, s->addr, s->offset,s->addrlen,value, s->cmd);
    if (s->ale) {
        unsigned int shift = s->addrlen * 8;
        unsigned int mask = ~(0xff << shift);
        unsigned int v = value << shift;

        s->addr = (s->addr & mask) | v;
        s->addrlen ++;
DB_PRINT("[**%s**] s->addr=%" PRIx64 " shift=%x mask=%x v=%x s->addrlen=%d cmd=0x%" PRIx32 "\n", __func__, s->addr, shift, mask, v, s->addrlen,s->cmd);
        switch (s->addrlen) {
        case 1:
            if (s->cmd == NAND_CMD_READID) {
                ncore_command(s);
            }
            break;
        case 2: /* fix cache address as a byte address */
            s->addr <<= (s->buswidth - 1);
            break;
        case 3:
            if (!(nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP) &&
                    (s->cmd == NAND_CMD_READ0 ||
                     s->cmd == NAND_CMD_PAGEPROGRAM1)) {
                ncore_command(s);
            }
            break;
        case 4:
            if ((nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP) &&
                    nand_flash_ids[s->chip_id].size < 256 && /* 1Gb or less */
                    (s->cmd == NAND_CMD_READ0 ||
                     s->cmd == NAND_CMD_PAGEPROGRAM1)) {
                ncore_command(s);
            }
            break;
        case 5:
            if ((nand_flash_ids[s->chip_id].options & NAND_SAMSUNG_LP) &&
                    nand_flash_ids[s->chip_id].size >= 256 && /* 2Gb or more */
                    (s->cmd == NAND_CMD_READ0 ||
                     s->cmd == NAND_CMD_PAGEPROGRAM1)) {
                ncore_command(s);
            }
            break;
        default:
            break;
        }
    }

    if (!s->cle && !s->ale && s->cmd == NAND_CMD_PAGEPROGRAM1) {
        if (s->iolen < (1 << s->page_shift) + (1 << s->oob_shift)) {
            for (i = s->buswidth; i--; value >>= 8) {
                s->io[s->iolen ++] = (uint8_t) (value & 0xff);
            }
        }
    } else if (!s->cle && !s->ale && s->cmd == NAND_CMD_COPYBACKPRG1) {
        if ((s->addr & ((1 << s->addr_shift) - 1)) <
                (1 << s->page_shift) + (1 << s->oob_shift)) {
            for (i = s->buswidth; i--; s->addr++, value >>= 8) {
                s->io[s->iolen + (s->addr & ((1 << s->addr_shift) - 1))] =
                    (uint8_t) (value & 0xff);
            }
        }
    }
}


uint64_t ncore_getio(DeviceState *dev)
{
    int offset;
    uint32_t x = 0;
    NCore_State *s = NCORE(dev);

    /* Allow sequential reading */
    if (!s->iolen && s->cmd == NAND_CMD_READ0) {
       offset = (int) (s->addr & ((1 << s->addr_shift) - 1)) + s->offset;
        s->offset = 0;
DB_PRINT("%s s->addraggasdf=0x%" PRIx64 " offset=%x cmd=0x%" PRIx32 " iolen=%x\n", __func__,s->addr, offset,s->cmd,s->iolen);
        s->blk_load(s, s->addr, offset);

        if (s->gnd)
            s->iolen = (1 << s->page_shift) - offset;
        else
            s->iolen = (1 << s->page_shift) + (1 << s->oob_shift) - offset;
    }
DB_PRINT("%s s->addr=0x%" PRIx64 " offset=0x%x cmd=0x%" PRIx32 " iolen=0x%x\n", __func__,s->addr, offset,s->cmd,s->iolen);
    if (s->ce || s->iolen <= 0) {
        return 0;
    }
//DB_PRINT("%s addr=%x offset=%x cmd=%x\n", __func__,(int) s->addr, (uint)offset,s->cmd);
    for (offset = s->buswidth; offset--;) {
        x |= s->ioaddr[offset] << (offset << 3);
DB_PRINT("**loop**%s s->ioaddr=0x%" PRIx8 " offset=0x%x s->ioaddr[%d]=0x%" PRIx8 " x=0x%" PRIx32 "\n", __func__,*s->ioaddr, offset,offset,s->ioaddr[offset],x);
    }
    /* after receiving READ STATUS command all subsequent reads will
     * return the status register value until another command is issued
     */
    if (s->cmd != NAND_CMD_READSTATUS) {
        s->addr   += s->buswidth;
        s->ioaddr += s->buswidth;
        s->iolen  -= s->buswidth;
    }
DB_PRINT("[**%s**] x=0x%" PRIx32 " \n", __func__,x);
    return x;
}

uint32_t ncore_getbuswidth(DeviceState *dev)
{
    NCore_State *s = (NCore_State *) dev;
    return s->buswidth << 3;
}


/*
static int ncore_init(SysBusDevice *dev)
{

    NCore_State *s = NCore(dev);
   DriveInfo *nand = drive_get(IF_MTD, 0, 0); 

    if (!nand) {
        hw_error("%s: NAND image required", __FUNCTION__);
    }
    s->nand = nand_init(nand ? nand->bdrv : NULL, NAND_MFR_MXIC, 0xf1);

    memory_region_init_io(&s->iomem, OBJECT(s), &ncore_ops, s,
                          "ncore", 0x1000);
    sysbus_init_mmio(dev, &s->iomem);
return 0;
}
*/
DeviceState *ncore_init(BlockDriverState *bdrv, int manf_id, int chip_id)
{
    DeviceState *dev;


    if (nand_flash_ids[chip_id].size == 0) {
        hw_error("%s: Unsupported NAND chip ID.\n", __FUNCTION__);
    }
    dev = DEVICE(object_new(TYPE_NCORE));
    qdev_prop_set_uint8(dev, "manf_id", manf_id);
    qdev_prop_set_uint8(dev, "chip_id", chip_id);
    if (bdrv) {
        qdev_prop_set_drive_nofail(dev, "drive", bdrv);
    }

    qdev_init_nofail(dev);
    return dev;
}

type_init(ncore_register_types)


#else

/* Program a single page */
static void glue(ncore_blk_write_, PAGE_SIZE)(NCore_State *s)
{
    uint64_t off, page, sector, soff;
    uint8_t iobuf[(PAGE_SECTORS + 2) * 0x200];
    if (PAGE(s->addr) >= s->pages)
        return;

    if (!s->bdrv) {
        mem_and(s->storage + PAGE_START(s->addr) + (s->addr & PAGE_MASK) +
                        s->offset, s->io, s->iolen);
    } else if (s->mem_oob) {
        sector = SECTOR(s->addr);
        off = (s->addr & PAGE_MASK) + s->offset;
        soff = SECTOR_OFFSET(s->addr);
        if (bdrv_read(s->bdrv, sector, iobuf, PAGE_SECTORS) < 0) {
            DB_PRINT("%s: read error in sector %" PRIu64 "\n", __func__, sector);
            return;
        }

        mem_and(iobuf + (soff | off), s->io, MIN(s->iolen, PAGE_SIZE - off));
        if (off + s->iolen > PAGE_SIZE) {
            page = PAGE(s->addr);
            mem_and(s->storage + (page << OOB_SHIFT), s->io + PAGE_SIZE - off,
                            MIN(OOB_SIZE, off + s->iolen - PAGE_SIZE));
        }

        if (bdrv_write(s->bdrv, sector, iobuf, PAGE_SECTORS) < 0) {
            DB_PRINT("%s: write error in sector %" PRIu64 "\n", __func__, sector);
        }
    } else {
        off = PAGE_START(s->addr) + (s->addr & PAGE_MASK) + s->offset;
        sector = off >> 9;
        soff = off & 0x1ff;
        if (bdrv_read(s->bdrv, sector, iobuf, PAGE_SECTORS + 2) < 0) {
            DB_PRINT("%s: read error in sector %" PRIu64 "\n", __func__, sector);
            return;
        }

        mem_and(iobuf + soff, s->io, s->iolen);

        if (bdrv_write(s->bdrv, sector, iobuf, PAGE_SECTORS + 2) < 0) {
            DB_PRINT("%s: write error in sector %" PRIu64 "\n", __func__, sector);
        }
    }
    s->offset = 0;
}

/* Erase a single block */
static void glue(ncore_blk_erase_, PAGE_SIZE)(NCore_State *s)
{
    uint64_t i, page, addr;
    uint8_t iobuf[0x200] = { [0 ... 0x1ff] = 0xff, };
    addr = s->addr & ~((1 << (ADDR_SHIFT + s->erase_shift)) - 1);

    if (PAGE(addr) >= s->pages) {
        return;
    }

    if (!s->bdrv) {
        memset(s->storage + PAGE_START(addr),
                        0xff, (PAGE_SIZE + OOB_SIZE) << s->erase_shift);
    } else if (s->mem_oob) {
        memset(s->storage + (PAGE(addr) << OOB_SHIFT),
                        0xff, OOB_SIZE << s->erase_shift);
        i = SECTOR(addr);
        page = SECTOR(addr + (ADDR_SHIFT + s->erase_shift));
        for (; i < page; i ++)
            if (bdrv_write(s->bdrv, i, iobuf, 1) < 0) {
                DB_PRINT("%s: write error in sector %" PRIu64 "\n", __func__, i);
            }
    } else {
        addr = PAGE_START(addr);
        page = addr >> 9;
        if (bdrv_read(s->bdrv, page, iobuf, 1) < 0) {
            DB_PRINT("%s: read error in sector %" PRIu64 "\n", __func__, page);
        }
        memset(iobuf + (addr & 0x1ff), 0xff, (~addr & 0x1ff) + 1);
        if (bdrv_write(s->bdrv, page, iobuf, 1) < 0) {
            DB_PRINT("%s: write error in sector %" PRIu64 "\n", __func__, page);
        }

        memset(iobuf, 0xff, 0x200);
        i = (addr & ~0x1ff) + 0x200;
        for (addr += ((PAGE_SIZE + OOB_SIZE) << s->erase_shift) - 0x200;
                        i < addr; i += 0x200) {
            if (bdrv_write(s->bdrv, i >> 9, iobuf, 1) < 0) {
                DB_PRINT("%s: write error in sector %" PRIu64 "\n",
                       __func__, i >> 9);
            }
        }

        page = i >> 9;
        if (bdrv_read(s->bdrv, page, iobuf, 1) < 0) {
            DB_PRINT("%s: read error in sector %" PRIu64 "\n", __func__, page);
        }
        memset(iobuf, 0xff, ((addr - 1) & 0x1ff) + 1);
        if (bdrv_write(s->bdrv, page, iobuf, 1) < 0) {
            DB_PRINT("%s: write error in sector %" PRIu64 "\n", __func__, page);
        }
    }
}

static void glue(ncore_blk_load_, PAGE_SIZE)(NCore_State *s,
                uint64_t addr, int offset)
{
    if (PAGE(addr) >= s->pages) {
        return;
    }

    if (s->bdrv) {
        if (s->mem_oob) {
            if (bdrv_read(s->bdrv, SECTOR(addr), s->io, PAGE_SECTORS) < 0) {
                DB_PRINT("%s: read error in sector %" PRIu64 "\n",
                                __func__, SECTOR(addr));
            }
            memcpy(s->io + SECTOR_OFFSET(s->addr) + PAGE_SIZE,
                            s->storage + (PAGE(s->addr) << OOB_SHIFT),
                            OOB_SIZE);
            s->ioaddr = s->io + SECTOR_OFFSET(s->addr) + offset;

        } else {
            if (bdrv_read(s->bdrv, PAGE_START(addr) >> 9,
                                    s->io, (PAGE_SECTORS + 2)) < 0) {
                DB_PRINT("%s: read error in sector %" PRIu64 "\n",
                                __func__, PAGE_START(addr) >> 9);
            }
            s->ioaddr = s->io + (PAGE_START(addr) & 0x1ff) + offset;
        }
    } else {
        memcpy(s->io, s->storage + PAGE_START(s->addr) +
                        offset, PAGE_SIZE + OOB_SIZE - offset);
        s->ioaddr = s->io;
    }
DB_PRINT("%s: ******** addr 0x%" PRIx64 " sector %" PRIu64 "\n",__func__, addr, SECTOR(addr));
DB_PRINT("%s: ******** s->addr=0x%" PRIx64 " offset=0%x s->io=0x%x SECTOR_OFFSET= %" PRIx64 " s->ioaddr=%x PAGE(s->addr)=%" PRIx64 " PAGE_START(addr)=%" PRIx64 "\n", __func__, s->addr, offset, *s->io, SECTOR_OFFSET(s->addr), *s->ioaddr, PAGE(s->addr), PAGE_START(addr) );
}

static void glue(ncore_init_, PAGE_SIZE)(NCore_State *s)  //2048
{
    s->oob_shift = PAGE_SHIFT - 5;  //11-5=6
    s->pages = s->size >> PAGE_SHIFT; //0x10000
    s->addr_shift = ADDR_SHIFT;  //16

    s->blk_erase = glue(ncore_blk_erase_, PAGE_SIZE);
    s->blk_write = glue(ncore_blk_write_, PAGE_SIZE);
    s->blk_load = glue(ncore_blk_load_, PAGE_SIZE);
}

# undef PAGE_SIZE
# undef PAGE_SHIFT
# undef PAGE_SECTORS
# undef ADDR_SHIFT
#endif	/* NAND_IO */
