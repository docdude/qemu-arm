/*
 * QEMU model of the FTSDC010 MMC/SD Host Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
#include "hw/sd.h"
#include "sysemu/sysemu.h"
#include "sysemu/blockdev.h"

#include "qemu/bitops.h"
#include "hw/ftsdc010.h"

#define TYPE_FTSDC010   "ftsdc010"

typedef struct Ftsdc010State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    SDState *card;
    qemu_irq irq;

    /* DMA hardware handshake */
    qemu_irq req;

    uint32_t datacnt;

    /* HW register cache */
    uint32_t cmd;
    uint32_t arg;
    uint32_t rsp[4];
    uint32_t rspcmd;
    uint32_t dcr;
    uint32_t dtr;
    uint32_t dlr;
    uint32_t status;
    uint32_t ier;
    uint32_t pwr;
    uint32_t clk;
} Ftsdc010State;

#define FTSDC010(obj) \
    OBJECT_CHECK(Ftsdc010State, obj, TYPE_FTSDC010)

static void ftsdc010_update_irq(Ftsdc010State *s)
{
    qemu_set_irq(s->irq, !!(s->ier & s->status));
}

static void ftsdc010_handle_ack(void *opaque, int line, int level)
{
    Ftsdc010State *s = FTSDC010(opaque);

    if (!(s->dcr & DCR_DMA)) {
        return;
    }

    if (level) {
        qemu_set_irq(s->req, 0);
    } else if (s->datacnt) {
        qemu_set_irq(s->req, 1);
    }
}

static void ftsdc010_send_command(Ftsdc010State *s)
{
    SDRequest request;
    uint8_t response[16];
    int rlen;

    request.cmd = s->cmd & CMD_IDX;
    request.arg = s->arg;

    rlen = sd_do_command(s->card, &request, response);
    if (rlen < 0) {
        goto error;
    }
    if (s->cmd & CMD_WAIT_RSP) {
#define RWORD(n) ((response[n] << 24) | (response[n + 1] << 16) \
                  | (response[n + 2] << 8) | response[n + 3])
        if (rlen == 0 || (rlen == 4 && (s->cmd & CMD_LONG_RSP))) {
            goto error;
        }
        if (rlen != 4 && rlen != 16) {
            goto error;
        }
        if (rlen == 4) {
            s->rsp[0] = RWORD(0);
            s->rsp[1] = s->rsp[2] = s->rsp[3] = 0;
        } else {
            s->rsp[3] = RWORD(0);
            s->rsp[2] = RWORD(4);
            s->rsp[1] = RWORD(8);
            s->rsp[0] = RWORD(12) & ~1;
        }
        s->rspcmd  = (s->cmd & CMD_IDX);
        s->rspcmd |= (s->cmd & CMD_APP) ? RSP_CMDAPP : 0;
        s->status |= SR_RSP;
#undef RWORD
    } else {
        s->status |= SR_CMD;
    }

    if ((s->dcr & DCR_DMA) && s->datacnt) {
        qemu_set_irq(s->req, 1);
    }

    return;

error:
    s->status |= SR_RSP_TIMEOUT;
}

static void ftsdc010_chip_reset(Ftsdc010State *s)
{
    s->cmd = 0;
    s->arg = 0;
    s->rsp[0] = 0;
    s->rsp[1] = 0;
    s->rsp[2] = 0;
    s->rsp[3] = 0;
    s->rspcmd = 0;
    s->dcr = 0;
    s->dtr = 0;
    s->dlr = 0;
    s->datacnt = 0;
    s->status &= ~(SR_CARD_REMOVED | SR_WPROT);
    s->status |= SR_TXRDY | SR_RXRDY;
    s->ier = 0;
    s->pwr = 0;
    s->clk = 0;
}

static uint64_t ftsdc010_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftsdc010State *s = FTSDC010(opaque);
    uint32_t i, ret = 0;

    switch (addr) {
    case REG_SR:
        return s->status;
    case REG_DR:
        if (!(s->dcr & DCR_WR) && s->datacnt && sd_data_ready(s->card)) {
            for (i = 0; i < 4 && s->datacnt; i++, s->datacnt--) {
                ret = deposit32(ret, i * 8, 8, sd_read_data(s->card));
            }
            if (!s->datacnt) {
                s->status |= SR_DAT_END;
            }
            s->status |= SR_DAT;
        }
        break;
    case REG_DCR:
        return s->dcr;
    case REG_DTR:
        return s->dtr;
    case REG_DLR:
        return s->dlr;
    case REG_CMD:
        return s->cmd;
    case REG_ARG:
        return s->arg;
    case REG_RSP0:
        return s->rsp[0];
    case REG_RSP1:
        return s->rsp[1];
    case REG_RSP2:
        return s->rsp[2];
    case REG_RSP3:
        return s->rsp[3];
    case REG_RSPCMD:
        return s->rspcmd;
    case REG_IER:
        return s->ier;
    case REG_PWR:
        return s->pwr;
    case REG_CLK:
        return s->clk;
    case REG_BUS:
        return 0x00000009;  /* support 1-bit/4-bit bus */
    case REG_FEAR:
        return 0x00000010;  /* fifo = 16 */
    case REG_REVR:
        return 0x00030300;  /* rev. 3.3.0 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftsdc010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftsdc010_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    int i;
    Ftsdc010State *s = FTSDC010(opaque);

    switch (addr) {
    case REG_DR:
        if ((s->dcr & DCR_WR) && s->datacnt) {
            for (i = 0; i < 4 && s->datacnt; i++, s->datacnt--) {
                sd_write_data(s->card, extract32((uint32_t)val, i * 8, 8));
            }
            if (!s->datacnt) {
                s->status |= SR_DAT_END;
            }
            s->status |= SR_DAT;
        }
        break;
    case REG_CMD:
        s->cmd = (uint32_t)val;
        if (s->cmd & CMD_RST) {
            ftsdc010_chip_reset(s);
        } else if (s->cmd & CMD_EN) {
            s->cmd &= ~CMD_EN;
            s->status &= ~(SR_CMD | SR_RSP | SR_RSP_ERR);
            ftsdc010_send_command(s);
        }
        break;
    case REG_ARG:
        s->arg = (uint32_t)val;
        break;
    case REG_DCR:
        s->dcr = (uint32_t)val;
        if (s->dcr & DCR_EN) {
            s->dcr &= ~(DCR_EN);
            s->status &= ~(SR_DAT | SR_DAT_END | SR_DAT_ERR);
            s->datacnt = s->dlr;
        }
        break;
    case REG_DTR:
        s->dtr = (uint32_t)val;
        break;
    case REG_DLR:
        s->dlr = (uint32_t)val;
        break;
    case REG_SCR:
        s->status &= ~((uint32_t)val & 0x14ff);
        ftsdc010_update_irq(s);
        break;
    case REG_IER:
        s->ier = (uint32_t)val;
        ftsdc010_update_irq(s);
        break;
    case REG_PWR:
        s->pwr = (uint32_t)val;
        break;
    case REG_CLK:
        s->clk = (uint32_t)val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftsdc010: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftsdc010_mem_read,
    .write = ftsdc010_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftsdc010_reset(DeviceState *ds)
{
    Ftsdc010State *s = FTSDC010(SYS_BUS_DEVICE(ds));

    ftsdc010_chip_reset(s);

    sd_set_cb(s->card, NULL, NULL);

    /* We can assume our GPIO outputs have been wired up now */
    qemu_set_irq(s->req, 0);
}

static void ftsdc010_realize(DeviceState *dev, Error **errp)
{
    Ftsdc010State *s = FTSDC010(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    DriveInfo *dinfo;

    memory_region_init_io(&s->iomem, OBJECT(s), &mmio_ops, s, TYPE_FTSDC010, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    qdev_init_gpio_in(dev, ftsdc010_handle_ack, 1);
    qdev_init_gpio_out(dev, &s->req, 1);

    dinfo = drive_get_next(IF_SD);
    s->card = sd_init(dinfo ? dinfo->bdrv : NULL, false,false);

    s->status = SR_CARD_REMOVED;
    if (dinfo) {
        if (bdrv_is_read_only(dinfo->bdrv)) {
            s->status |= SR_WPROT;
        }
        if (bdrv_is_inserted(dinfo->bdrv)) {
            s->status &= ~SR_CARD_REMOVED;
        }
    }
}

static const VMStateDescription vmstate_ftsdc010 = {
    .name = TYPE_FTSDC010,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cmd, Ftsdc010State),
        VMSTATE_UINT32(arg, Ftsdc010State),
        VMSTATE_UINT32_ARRAY(rsp, Ftsdc010State, 4),
        VMSTATE_UINT32(rspcmd, Ftsdc010State),
        VMSTATE_UINT32(dcr, Ftsdc010State),
        VMSTATE_UINT32(dtr, Ftsdc010State),
        VMSTATE_UINT32(dlr, Ftsdc010State),
        VMSTATE_UINT32(datacnt, Ftsdc010State),
        VMSTATE_UINT32(status, Ftsdc010State),
        VMSTATE_UINT32(ier, Ftsdc010State),
        VMSTATE_UINT32(pwr, Ftsdc010State),
        VMSTATE_UINT32(clk, Ftsdc010State),
        VMSTATE_END_OF_LIST()
    }
};

static void ftsdc010_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftsdc010;
    dc->reset   = ftsdc010_reset;
    dc->realize = ftsdc010_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftsdc010_info = {
    .name           = TYPE_FTSDC010,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(Ftsdc010State),
    .class_init     = ftsdc010_class_init,
};

static void ftsdc010_register_types(void)
{
    type_register_static(&ftsdc010_info);
}

type_init(ftsdc010_register_types)
