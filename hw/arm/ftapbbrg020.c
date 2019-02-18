/*
 * QEMU model of the FTAPBBRG020 DMA Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 *
 * Note: The FTAPBBRG020 DMA descending address mode is not implemented.
 */

#include "hw/sysbus.h"
#include "sysemu/dma.h"
#include "sysemu/sysemu.h"
#include "sysemu/blockdev.h"

#include "hw/arm/faraday.h"
#include "hw/ftapbbrg020.h"

#define TYPE_FTAPBBRG020    "ftapbbrg020"

typedef struct Ftapbbrg020State Ftapbbrg020State;

typedef struct Ftapbbrg020Chan {
    Ftapbbrg020State *chip;

    int id;
    int burst;
    int src_bw;
    int src_stride;
    int dst_bw;
    int dst_stride;

    /* HW register caches */
    uint32_t src;
    uint32_t dst;
    uint32_t len;
    uint32_t cmd;
} Ftapbbrg020Chan;

typedef struct Ftapbbrg020State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    qemu_irq irq;

    FaradaySoCState *soc;
    Ftapbbrg020Chan chan[4];
    qemu_irq ack[16];
    uint32_t req;

    int busy;    /* Busy Channel ID */
    int bh_owner;
    QEMUBH *bh;
//    DMAContext *dma;
    AddressSpace *dma;
} Ftapbbrg020State;

#define FTAPBBRG020(obj) \
    OBJECT_CHECK(Ftapbbrg020State, obj, TYPE_FTAPBBRG020)

static uint32_t ftapbbrg020_get_isr(Ftapbbrg020State *s)
{
    int i;
    uint32_t isr = 0;
    Ftapbbrg020Chan *chan;

    for (i = 0; i < 4; ++i) {
        chan = s->chan + i;
        isr |= (chan->cmd & CMD_INTR_STATUS);
    }

    return isr;
}

static void ftapbbrg020_update_irq(Ftapbbrg020State *s)
{
    uint32_t isr = ftapbbrg020_get_isr(s);

    qemu_set_irq(s->irq, isr ? 1 : 0);
}

static void ftapbbrg020_chan_cmd_decode(Ftapbbrg020Chan *c)
{
    uint32_t tmp;

    /* 1. decode burst size */
    c->burst = (c->cmd & CMD_BURST4) ? 4 : 1;

    /* 2. decode source/destination width */
    tmp = extract32(c->cmd, 20, 2);
    if (tmp > 2) {
        tmp = 2;
    }
    c->src_bw = c->dst_bw = 8 << (2 - tmp);

    /* 3. decode source address stride */
    switch (extract32(c->cmd, 8, 2)) {
    case 0:
        c->src_stride = 0;
        break;
    case 1:
        c->src_stride = c->src_bw >> 3;
        break;
    case 2:
        c->src_stride = 2 * (c->src_bw >> 3);
        break;
    case 3:
        c->src_stride = 4 * (c->src_bw >> 3);
        break;
    }

    /* 4. decode destination address stride */
    switch (extract32(c->cmd, 12, 2)) {
    case 0:
        c->dst_stride = 0;
        break;
    case 1:
        c->dst_stride = c->dst_bw >> 3;
        break;
    case 2:
        c->dst_stride = 2 * (c->dst_bw >> 3);
        break;
    case 3:
        c->dst_stride = 4 * (c->dst_bw >> 3);
        break;
    }
}

static void ftapbbrg020_chan_start(Ftapbbrg020Chan *c)
{
    Ftapbbrg020State *s = c->chip;
    hwaddr src, dst;
    uint8_t buf[4096] __attribute__ ((aligned (8)));
    int i, len, stride, src_hs, dst_hs;

    if (!(c->cmd & CMD_START)) {
        return;
    }

    s->busy = c->id;

    /* DMA src/dst address */
    src = c->src;
    dst = c->dst;

    /* DMA hardware handshake id */
    src_hs = extract32(c->cmd, 24, 4);
    dst_hs = extract32(c->cmd, 16, 4);

    /* DMA src/dst sanity check */
    if (cpu_physical_memory_is_io(src) && c->src_stride) {
        fprintf(stderr,
            "ftapbbrg020: src is an I/O device with non-fixed address?\n");
        abort();
    }
    if (cpu_physical_memory_is_io(dst) && c->dst_stride) {
        fprintf(stderr,
            "ftapbbrg020: dst is an I/O device with non-fixed address?\n");
        abort();
    }

    while (c->len > 0) {
        /*
         * Postpone this DMA action
         * if the corresponding dma request is not asserted
         */
        if (src_hs && !(s->req & BIT(src_hs))) {
            break;
        }
        if (dst_hs && !(s->req & BIT(dst_hs))) {
            break;
        }

        len = MIN(sizeof(buf), c->burst * (c->src_bw >> 3));

        /* load data from source into local buffer */
        if (c->src_stride) {
            dma_memory_read(s->dma, src, buf, len);
            src += len;
        } else {
            stride = c->src_bw >> 3;
            for (i = 0; i < len; i += stride) {
                dma_memory_read(s->dma, src, buf + i, stride);
            }
        }

        /* DMA Hardware Handshake */
        if (src_hs) {
            qemu_set_irq(s->ack[src_hs], 1);
        }

        /* store data into destination from local buffer */
        if (c->dst_stride) {
            dma_memory_write(s->dma, dst, buf, len);
            dst += len;
        } else {
            stride = c->dst_bw >> 3;
            for (i = 0; i < len; i += stride) {
                dma_memory_write(s->dma, dst, buf + i, stride);
            }
        }

        /* DMA Hardware Handshake */
        if (dst_hs) {
            qemu_set_irq(s->ack[dst_hs], 1);
        }

        /* update the channel transfer size */
        c->len -= len / (c->src_bw >> 3);

        if (c->len == 0) {
            /* update the channel transfer status */
            if (c->cmd & CMD_INTR_FIN_EN) {
                c->cmd |= CMD_INTR_FIN;
                ftapbbrg020_update_irq(s);
            }
            /* clear start bit */
            c->cmd &= ~CMD_START;
        }
    }

    /* update src/dst address */
    c->src = src;
    c->dst = dst;

    s->busy = -1;
}

static void ftapbbrg020_chan_reset(Ftapbbrg020Chan *c)
{
    c->cmd = 0;
    c->src = 0;
    c->dst = 0;
    c->len = 0;
}

static void ftapbbrg020_bh(void *opaque)
{
    Ftapbbrg020State *s = FTAPBBRG020(opaque);
    Ftapbbrg020Chan  *c = NULL;
    int i, jobs, done;

    ++s->bh_owner;
    jobs = 0;
    done = 0;
    for (i = 0; i < 4; ++i) {
        c = s->chan + i;
        if (c->cmd & CMD_START) {
            ++jobs;
            ftapbbrg020_chan_start(c);
            if (!(c->cmd & CMD_START)) {
                ++done;
            }
        }
    }
    --s->bh_owner;

    /*
     * Devices those with an infinite FIFO (always ready for R/W)
     * would trigger a new DMA handshake transaction here.
     * (i.e. ftnandc021, ftsdc010)
     */
    if ((jobs - done) && s->req) {
        qemu_bh_schedule(s->bh);
    }
}

static void ftapbbrg020_handle_req(void *opaque, int line, int level)
{
    Ftapbbrg020State *s = FTAPBBRG020(opaque);

    if (level) {
        /*
         * Devices those wait for data from external I/O
         * would trigger a new DMA handshake transaction here.
         * (i.e. ftssp010)
         */
        if (!(s->req & BIT(line))) {
            /* a simple workaround for BH reentry issue */
            if (!s->bh_owner) {
                qemu_bh_schedule(s->bh);
            }
        }
        s->req |= BIT(line);
    } else {
        s->req &= ~BIT(line);
        qemu_set_irq(s->ack[line], 0);
    }
}

static void ftapbbrg020_chip_reset(Ftapbbrg020State *s)
{
    int i;

    for (i = 0; i < 4; ++i) {
        ftapbbrg020_chan_reset(s->chan + i);
    }

    /* We can assume our GPIO have been wired up now */
    for (i = 0; i < 16; ++i) {
        qemu_set_irq(s->ack[i], 0);
    }
    s->req = 0;
}

static uint64_t
ftapbbrg020_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftapbbrg020State *s = FTAPBBRG020(opaque);
    Ftapbbrg020Chan  *c = NULL;
    uint32_t ret = 0;

    switch (addr) {
    case REG_SLAVE(0) ... REG_SLAVE(31):
        ret = s->soc->apb_slave[addr / 4];
        break;
    case REG_CHAN_BASE(0) ... REG_CHAN_BASE(3) + 0x0C:
        c = s->chan + REG_CHAN_ID(addr);
        switch (addr & 0x0f) {
        case REG_CHAN_CMD:
            return c->cmd;
        case REG_CHAN_SRC:
            return c->src;
        case REG_CHAN_DST:
            return c->dst;
        case REG_CHAN_CYC:
            return c->len;
        }
        break;
    case REG_REVISION:
        return 0x00010800;  /* rev 1.8.0 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftapbbrg020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftapbbrg020_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftapbbrg020State *s = FTAPBBRG020(opaque);
    Ftapbbrg020Chan  *c = NULL;

    switch (addr) {
    case REG_CHAN_BASE(0) ... REG_CHAN_BASE(3) + 0x0C:
        c = s->chan + REG_CHAN_ID(addr);
        switch (addr & 0x0f) {
        case REG_CHAN_CMD:
            c->cmd = (uint32_t)val;
            ftapbbrg020_update_irq(s);
            if (c->cmd & CMD_START) {
                ftapbbrg020_chan_cmd_decode(c);
                /* kick-off DMA engine */
                qemu_bh_schedule(s->bh);
            }
            break;
        case REG_CHAN_SRC:
            c->src = (uint32_t)val;
            break;
        case REG_CHAN_DST:
            c->dst = (uint32_t)val;
            break;
        case REG_CHAN_CYC:
            c->len = (uint32_t)val & 0x00ffffff;
            break;
        }
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftapbbrg020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftapbbrg020_mem_read,
    .write = ftapbbrg020_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftapbbrg020_reset(DeviceState *ds)
{
    Ftapbbrg020State *s = FTAPBBRG020(SYS_BUS_DEVICE(ds));
    Error *local_errp = NULL;

    s->soc = FARADAY_SOC(object_property_get_link(OBJECT(s),
                                                  "soc",
                                                  &local_errp));
    if (local_errp) {
        fprintf(stderr, "ftapbbrg020: Unable to get soc link\n");
        abort();
    }

    ftapbbrg020_chip_reset(s);
}

static void ftapbbrg020_realize(DeviceState *dev, Error **errp)
{
    Ftapbbrg020State *s = FTAPBBRG020(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    Ftapbbrg020Chan *c;
    int i;

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTAPBBRG020,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
    qdev_init_gpio_in(dev, ftapbbrg020_handle_req, 16);
    qdev_init_gpio_out(dev, s->ack, 16);

    s->busy = -1;
    s->dma = &address_space_memory;
    s->bh = qemu_bh_new(ftapbbrg020_bh, s);
    for (i = 0; i < 4; ++i) {
        c = s->chan + i;
        c->id   = i;
        c->chip = s;
    }
}

static const VMStateDescription vmstate_ftapbbrg020 = {
    .name = TYPE_FTAPBBRG020,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void ftapbbrg020_instance_init(Object *obj)
{
    Ftapbbrg020State *s = FTAPBBRG020(obj);

    object_property_add_link(obj,
                             "soc",
                             TYPE_FARADAY_SOC,
                             (Object **) &s->soc,
                             object_property_allow_set_link,
                             OBJ_PROP_LINK_UNREF_ON_RELEASE,
                             &error_abort);
}

static void ftapbbrg020_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftapbbrg020;
    dc->reset   = ftapbbrg020_reset;
    dc->realize = ftapbbrg020_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftapbbrg020_info = {
    .name          = TYPE_FTAPBBRG020,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftapbbrg020State),
    .instance_init = ftapbbrg020_instance_init,
    .class_init    = ftapbbrg020_class_init,
};

static void ftapbbrg020_register_types(void)
{
    type_register_static(&ftapbbrg020_info);
}

type_init(ftapbbrg020_register_types)
