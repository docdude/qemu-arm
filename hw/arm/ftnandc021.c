/*
 * QEMU model of the FTNANDC021 NAND Flash Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
#include "hw/devices.h"
#include "hw/block/flash.h"
#include "sysemu/blockdev.h"

#include "hw/ftnandc021.h"
#define DEBUG_NAND
#ifdef DEBUG_NAND
#define DB_PRINT(...) do { \
    fprintf(stderr,  "[**%s**] ", __func__); \
    fprintf(stderr, ## __VA_ARGS__); \
    } while (0);
#else
    #define DB_PRINT(...)
#endif
#define TYPE_FTNANDC021 "ftnandc021"

typedef struct Ftnandc021State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion mmio;

    qemu_irq irq;
    DeviceState *flash;

    /* DMA hardware handshake */
    qemu_irq req;

    uint8_t  manf_id, chip_id;

    int      cmd;
    int      len;    /* buffer length for page read/write */
    int      pi;    /* page index */
    int      bw;    /* bus width (8-bits, 16-bits) */

    uint64_t size;    /* flash size (maximum access range) */
    uint32_t pgsz;    /* page size (Bytes) */
    uint32_t bksz;    /* block size (Bytes) */
    uint32_t alen;    /* address length (cycle) */

    uint32_t id[2];
    uint8_t  oob[16];/* 5 bytes for 512/2048 page; 7 bytes for 4096 page */

    /* HW register caches */
    uint32_t sr;
    uint32_t fcr;
    uint32_t mcr;
    uint32_t ier;
    uint32_t bcr;
} Ftnandc021State;

#define FTNANDC021(obj) \
    OBJECT_CHECK(Ftnandc021State, obj, TYPE_FTNANDC021)

static void ftnandc021_update_irq(Ftnandc021State *s)
{
    if (s->ier & IER_ENA) {
        if ((s->ier & 0x0f) & (s->sr >> 2)) {
            qemu_set_irq(s->irq, 1);
        } else {
            qemu_set_irq(s->irq, 0);
        }
    }
}

static void ftnandc021_set_idle(Ftnandc021State *s)
{
    /* CLE=0, ALE=0, CS=1 */
    nand_setpins(s->flash, 0, 0, 1, 1, 0);

    /* Set command compelete */
    s->sr |= SR_CMD;

    /* Update IRQ signal */
    ftnandc021_update_irq(s);
}

static void ftnandc021_set_cmd(Ftnandc021State *s, uint8_t cmd)
{
    /* CLE=1, ALE=0, CS=0 */
    nand_setpins(s->flash, 1, 0, 0, 1, 0);

    /* Write out command code */
    nand_setio(s->flash, cmd);
}

static void ftnandc021_set_addr(Ftnandc021State *s, int col, int row)
{
    /* CLE=0, ALE=1, CS=0 */
    nand_setpins(s->flash, 0, 1, 0, 1, 0);

    if (col < 0 && row < 0) {
        /* special case for READ_ID (0x90) */
        nand_setio(s->flash, 0);
    } else {
        /* column address */
        if (col >= 0) {
            nand_setio(s->flash, extract32(col, 0, 8));
            nand_setio(s->flash, extract32(col, 8, 8));
        }
        /* row address */
        if (row >= 0) {
            nand_setio(s->flash, extract32(row, 0, 8));
            if (s->alen >= 4) {
                nand_setio(s->flash, extract32(row, 8, 8));
            }
            if (s->alen >= 5) {
                nand_setio(s->flash, extract32(row, 16, 8));
            }
        }
    }
}

static void ftnandc021_handle_ack(void *opaque, int line, int level)
{
    Ftnandc021State *s = FTNANDC021(opaque);

    if (!s->bcr) {
        return;
    }

    if (level) {
        qemu_set_irq(s->req, 0);
    } else if (s->len > 0) {
        qemu_set_irq(s->req, 1);
    }
}

static void ftnandc021_command(Ftnandc021State *s, uint32_t cmd)
{
    int i;

    s->sr &= ~SR_CMD;
    s->cmd = cmd;
DB_PRINT("command=0x%02x\n", cmd);
printf("******pi=0x%x\n", s->pi);
    switch (cmd) {
    case FTNANDC021_CMD_RDID:    /* read id */
        ftnandc021_set_cmd(s, 0x90);
        ftnandc021_set_addr(s, -1, -1);
        nand_setpins(s->flash, 0, 0, 0, 1, 0);
        if (s->bw == 8) {
            s->id[0] = (nand_getio(s->flash) << 0)
                     | (nand_getio(s->flash) << 8)
                     | (nand_getio(s->flash) << 16)
                     | (nand_getio(s->flash) << 24);
            s->id[1] = (nand_getio(s->flash) << 0);
        } else {
            s->id[0] = (nand_getio(s->flash) << 0)
                     | (nand_getio(s->flash) << 16);
            s->id[1] = (nand_getio(s->flash) << 0);
        }
        break;
    case FTNANDC021_CMD_RESET:    /* reset */
        ftnandc021_set_cmd(s, 0xff);
        break;
    case FTNANDC021_CMD_RDST:    /* read status */
        ftnandc021_set_cmd(s, 0x70);
        nand_setpins(s->flash, 0, 0, 0, 1, 0);
        s->id[1] = (nand_getio(s->flash) << 0);
        break;
    case FTNANDC021_CMD_RDPG:    /* read page */
        ftnandc021_set_cmd(s, 0x00);
        ftnandc021_set_addr(s, 0, s->pi);
        ftnandc021_set_cmd(s, 0x30);
        nand_setpins(s->flash, 0, 0, 0, 1, 0);
        s->len = s->pgsz;
        break;
    case FTNANDC021_CMD_RDOOB:    /* read oob */
        ftnandc021_set_cmd(s, 0x00);

        ftnandc021_set_addr(s, s->pgsz, s->pi);
        ftnandc021_set_cmd(s, 0x30);
        nand_setpins(s->flash, 0, 0, 0, 1, 0);
        for (i = 0; i < 16 * (s->pgsz / 512); ) {
            if (s->bw == 8) {
                if (i < 16) {
                    s->oob[i] = (uint8_t)nand_getio(s->flash);
                } else {
                    (void)nand_getio(s->flash);
                }
                i += 1;
            } else {
                if (i < 16) {
                    *(uint16_t *)(s->oob + i) = (uint16_t)nand_getio(s->flash);
                } else {
                    (void)nand_getio(s->flash);
                }
                i += 2;
            }
        }
        break;
    case FTNANDC021_CMD_WRPG:    /* write page + read status */
        ftnandc021_set_cmd(s, 0x80);
        ftnandc021_set_addr(s, 0, s->pi);
        /* data phase */
        nand_setpins(s->flash, 0, 0, 0, 1, 0);
        s->len = s->pgsz;
        break;
    case FTNANDC021_CMD_ERBLK:    /* erase block + read status */
        ftnandc021_set_cmd(s, 0x60);
        ftnandc021_set_addr(s, -1, s->pi);
        ftnandc021_set_cmd(s, 0xd0);
        /* read status */
        ftnandc021_command(s, 0x04);
        break;
    case FTNANDC021_CMD_WROOB:    /* write oob + read status */
        ftnandc021_set_cmd(s, 0x80);
        ftnandc021_set_addr(s, s->pgsz, s->pi);
        /* data phase */
        nand_setpins(s->flash, 0, 0, 0, 1, 0);
        for (i = 0; i < 16 * (s->pgsz / 512); ) {
            if (s->bw == 8) {
                if (i <= 16) {
                    nand_setio(s->flash, s->oob[i]);
                } else {
                    nand_setio(s->flash, 0xffffffff);
                }
                i += 1;
            } else {
                if (i <= 16) {
                    nand_setio(s->flash, s->oob[i] | (s->oob[i + 1] << 8));
                } else {
                    nand_setio(s->flash, 0xffffffff);
                }
                i += 2;
            }
        }
        ftnandc021_set_cmd(s, 0x10);
        /* read status */
        ftnandc021_command(s, 0x04);
        break;
    default:
        DB_PRINT("ftnandc021: unknow command=0x%02x\n", cmd);
        break;
    }

    /* if cmd is not page read/write, then return to idle mode */
    switch (s->cmd) {
    case FTNANDC021_CMD_RDPG:
    case FTNANDC021_CMD_WRPG:
        if (s->bcr && (s->len > 0)) {
            qemu_set_irq(s->req, 1);
        }
        break;
    default:
        ftnandc021_set_idle(s);
        break;
    }
}

static uint64_t
ftnandc021_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    uint32_t i, ret = 0;
    Ftnandc021State *s = FTNANDC021(opaque);
DB_PRINT("memory access@%#" HWADDR_PRIx "\n", addr);
    switch (addr) {
    case REG_DR:
        if ((s->cmd == FTNANDC021_CMD_RDPG) && (s->len > 0)) {
            if (s->bw == 8) {
                for (i = 0; i < 4 && s->len > 0; i++, s->len--) {
                    ret = deposit32(ret, i * 8, 8, nand_getio(s->flash));
                }
            } else {
                for (i = 0; i < 2 && s->len > 1; i++, s->len -= 2) {
                    ret = deposit32(ret, i * 16, 16, nand_getio(s->flash));
                }
            }
            if (s->len <= 0) {
                ftnandc021_set_idle(s);
            }
        }
        break;
    case REG_SR:
        return s->sr;
    case REG_ACR:
        return s->cmd << 8;
    case REG_RDBR:
        return s->oob[0];
    case REG_RDLSN:
        return s->oob[1] | (s->oob[2] << 8);
    case REG_RDCRC:
        if (s->pgsz > 2048) {
            return s->oob[8] | (s->oob[9] << 8)
                   | (s->oob[10] << 16) | (s->oob[11] << 24);
        } else {
            return s->oob[8] | (s->oob[9] << 8);
        }
    case REG_FCR:
        return s->fcr;
    case REG_PIR:
        return s->pi;
    case REG_PCR:
        return 1;           /* page count = 1 */
    case REG_MCR:
        return s->mcr;
    case REG_IDRL:
        return s->id[0];
    case REG_IDRH:
        return s->id[1];
    case REG_IER:
        return s->ier;
    case REG_BCR:
        return s->bcr;
    case REG_ATR1:
        return 0x02240264;  /* AC Timing */
    case REG_ATR2:
        return 0x42054209;  /* AC Timing */
    case REG_PRR:
        return 0x00000001;  /* Always ready for I/O */
    case REG_REVR:
        return 0x00010100;  /* Rev. 1.1.0 */
    case REG_CFGR:
        return 0x00081601;  /* 8-bit BCH, 16 bit flash, 1 chip */
    default:
        DB_PRINT("ftnandc021: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftnandc021_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    uint32_t i;
    Ftnandc021State *s = FTNANDC021(opaque);
DB_PRINT("memory access@%#" HWADDR_PRIx " val=0x%lx\n", addr, val);
    switch (addr) {
    case REG_DR:
        if (s->cmd == FTNANDC021_CMD_WRPG && s->len > 0) {
            if (s->bw == 8) {
                for (i = 0; i < 4 && s->len > 0; i++, s->len--) {
                    nand_setio(s->flash,
                               extract32((uint32_t)val, i * 8, 8));
                }
            } else {
                for (i = 0; i < 2 && s->len > 1; i++, s->len -= 2) {
                    nand_setio(s->flash,
                               extract32((uint32_t)val, i * 16, 16));
                }
            }
            if (s->len <= 0) {
                ftnandc021_set_cmd(s, 0x10);
                /* read status */
                ftnandc021_command(s, 0x04);
            }
        }
        break;
    case REG_ACR:
        if (!(val & ACR_START)) {
            break;
        }
        ftnandc021_command(s, extract32((uint32_t)val, 8, 5));
        break;
    case REG_WRBR:
        s->oob[0] = (uint32_t)val & 0xff;
        break;
    case REG_WRLSN:
        s->oob[1] = ((uint32_t)val >> 0) & 0xff;
        s->oob[2] = ((uint32_t)val >> 8) & 0xff;
        break;
    case REG_WRCRC:
        s->oob[8] = ((uint32_t)val >> 0) & 0xff;
        s->oob[9] = ((uint32_t)val >> 8) & 0xff;
        if (s->pgsz > 2048) {
            s->oob[10] = ((uint32_t)val >> 16) & 0xff;
            s->oob[11] = ((uint32_t)val >> 24) & 0xff;
        }
        break;
    case REG_FCR:
        s->fcr = (uint32_t)val;
        if (s->fcr & FCR_16BIT) {
            s->bw = 16;
        } else {
            s->bw = 8;
        }
        break;
    case REG_PIR:
        s->pi = (uint32_t)val & 0x03ffffff;
        break;
    case REG_MCR:
        s->mcr = (uint32_t)val;
        /* page size */
        switch (extract32(s->mcr, 8, 2)) {
        case 0:
            s->pgsz = 512;
            break;
        case 2:
            s->pgsz = 4096;
            break;
        default:
            s->pgsz = 2048;
            break;
        }
        /* block size */
        s->bksz = s->pgsz * (1 << (4 + extract32(s->mcr, 16, 2)));
        /* address length (cycle) */
        s->alen = 3 + extract32(s->mcr, 10, 2);
        /* flash size */
        s->size = 1ULL << (24 + extract32(s->mcr, 4, 4));
        break;
    case REG_IER:
        s->ier = (uint32_t)val & 0x8f;
        ftnandc021_update_irq(s);
        break;
    case REG_ISCR:
        s->sr &= ~(((uint32_t)val & 0x0f) << 2);
        ftnandc021_update_irq(s);
        break;
    case REG_BCR:
        s->bcr = (uint32_t)val;
        break;
    default:
        DB_PRINT("ftnandc021: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftnandc021_mem_read,
    .write = ftnandc021_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftnandc021_reset(DeviceState *ds)
{
    Ftnandc021State *s = FTNANDC021(SYS_BUS_DEVICE(ds));
    Error *local_errp = NULL;

    s->flash = DEVICE(object_property_get_link(OBJECT(s),
                                               "flash",
                                               &local_errp));
    if (local_errp) {
        fprintf(stderr, "ftnandc021: Unable to get flash link\n");
        abort();
    }

    s->sr    = 0;
    s->fcr   = 0;
    s->mcr   = 0;
    s->ier   = 0;
    s->bcr   = 0;
    s->id[0] = 0;
    s->id[1] = 0;

    /* We can assume our GPIO outputs have been wired up now */
    qemu_set_irq(s->req, 0);
}

static void ftnandc021_realize(DeviceState *dev, Error **errp)
{
    Ftnandc021State *s = FTNANDC021(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->mmio,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTNANDC021,
                          0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);

    qdev_init_gpio_in(dev, ftnandc021_handle_ack, 1);
    qdev_init_gpio_out(dev, &s->req, 1);
}

static const VMStateDescription vmstate_ftnandc021 = {
    .name = TYPE_FTNANDC021,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(sr, Ftnandc021State),
        VMSTATE_UINT32(fcr, Ftnandc021State),
        VMSTATE_UINT32(mcr, Ftnandc021State),
        VMSTATE_UINT32(ier, Ftnandc021State),
        VMSTATE_UINT32(bcr, Ftnandc021State),
        VMSTATE_END_OF_LIST()
    }
};

static void ftnandc021_instance_init(Object *obj)
{
    Ftnandc021State *s = FTNANDC021(obj);
#if 1
    object_property_add_link(obj,
                             "flash",
                             TYPE_DEVICE,
                             (Object **) &s->flash,
                             object_property_allow_set_link,
                             OBJ_PROP_LINK_UNREF_ON_RELEASE,
                             &error_abort);
#endif
   
}

static void ftnandc021_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftnandc021;
    dc->reset   = ftnandc021_reset;
    dc->realize = ftnandc021_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftnandc021_info = {
    .name          = TYPE_FTNANDC021,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftnandc021State),
    .instance_init = ftnandc021_instance_init,
    .class_init    = ftnandc021_class_init,
};

static void ftnandc021_register_types(void)
{
    type_register_static(&ftnandc021_info);
}

type_init(ftnandc021_register_types)
