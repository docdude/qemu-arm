/*
 * QEMU model of the FTDMAC020 DMA Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 *
 * Note: The FTDMAC020 descending address mode is not implemented.
 */

#include "hw/sysbus.h"
#include "sysemu/dma.h"
#include "sysemu/sysemu.h"
#include "sysemu/blockdev.h"

#include "hw/ftdmac020.h"

#define TYPE_FTDMAC020    "ftdmac020"

enum ftdmac020_irqpin {
    IRQ_ALL = 0,
    IRQ_TC,
    IRQ_ERR,
};

typedef struct Ftdmac020State Ftdmac020State;

/**
 * struct Ftdmac020LLD - hardware link list descriptor.
 * @src: source physical address
 * @dst: destination physical addr
 * @next: phsical address to the next link list descriptor
 * @ctrl: control field
 * @size: transfer size
 *
 * should be word aligned
 */
typedef struct Ftdmac020LLD {
    uint32_t src;
    uint32_t dst;
    uint32_t next;
    uint32_t ctrl;
    uint32_t size;
} Ftdmac020LLD;

typedef struct Ftdmac020Chan {
    Ftdmac020State *chip;

    int id;
    int burst;
    int llp_cnt;
    int src_bw;
    int src_stride;
    int dst_bw;
    int dst_stride;

    /* HW register cache */
    uint32_t ccr;
    uint32_t cfg;
    uint32_t src;
    uint32_t dst;
    uint32_t llp;
    uint32_t len;
} Ftdmac020Chan;

typedef struct Ftdmac020State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    qemu_irq irq[3];

    Ftdmac020Chan chan[8];
    qemu_irq      ack[16];
    uint32_t      req;

    int busy;    /* Busy Channel ID */
    int bh_owner;
    QEMUBH *bh;
//    DMAContext *dma;
    AddressSpace *dma;    
    /* HW register cache */
    uint32_t tcisr;
    uint32_t eaisr;
    uint32_t tcsr;
    uint32_t easr;
    uint32_t cesr;
    uint32_t cbsr;
    uint32_t csr;
    uint32_t sync;
} Ftdmac020State;

#define FTDMAC020(obj) \
    OBJECT_CHECK(Ftdmac020State, obj, TYPE_FTDMAC020)

static void ftdmac020_update_irq(Ftdmac020State *s)
{
    uint32_t tc, err;

    /* 1. Checking TC interrupts */
    tc = s->tcisr & 0xff;
    qemu_set_irq(s->irq[IRQ_TC], tc ? 1 : 0);

    /* 2. Checking Error/Abort interrupts */
    err = s->eaisr & 0x00ff00ff;
    qemu_set_irq(s->irq[IRQ_ERR], err ? 1 : 0);

    /* 3. Checking interrupt summary (TC | Error | Abort) */
    qemu_set_irq(s->irq[IRQ_ALL], (tc || err) ? 1 : 0);
}

static void ftdmac020_chan_ccr_decode(Ftdmac020Chan *c)
{
    uint32_t tmp;

    /* 1. decode burst size */
    tmp = extract32(c->ccr, 16, 3);
    c->burst  = 1 << (tmp ? tmp + 1 : 0);

    /* 2. decode source width */
    tmp = extract32(c->ccr, 11, 2);
    c->src_bw = 8 << tmp;

    /* 3. decode destination width */
    tmp = extract32(c->ccr, 8, 2);
    c->dst_bw = 8 << tmp;

    /* 4. decode source address stride */
    tmp = extract32(c->ccr, 5, 2);
    if (tmp == 2) {
        c->src_stride = 0;
    } else {
        c->src_stride = c->src_bw >> 3;
    }

    /* 5. decode destination address stride */
    tmp = extract32(c->ccr, 3, 2);
    if (tmp == 2) {
        c->dst_stride = 0;
    } else {
        c->dst_stride = c->dst_bw >> 3;
    }
}

static void ftdmac020_chan_start(Ftdmac020Chan *c)
{
    Ftdmac020State *s = c->chip;
    Ftdmac020LLD desc;
    hwaddr src, dst;
    uint8_t buf[4096] __attribute__ ((aligned (8)));
    int i, len, stride, src_hs, dst_hs;

    if (!(c->ccr & CCR_START)) {
        return;
    }

    s->busy = c->id;

    /* DMA src/dst address */
    src = c->src;
    dst = c->dst;

    /* DMA hardware handshake id */
    src_hs = (c->cfg & CFG_SRC_HANDSHAKE_EN) ? extract32(c->cfg, 3, 4) : -1;
    dst_hs = (c->cfg & CFG_DST_HANDSHAKE_EN) ? extract32(c->cfg, 9, 4) : -1;

    /* DMA src/dst sanity check */
    if (cpu_physical_memory_is_io(src) && c->src_stride) {
        fprintf(stderr,
            "ftdmac020: src is an I/O device with non-fixed address?\n");
        abort();
    }
    if (cpu_physical_memory_is_io(dst) && c->dst_stride) {
        fprintf(stderr,
            "ftdmac020: dst is an I/O device with non-fixed address?\n");
        abort();
    }

    while (c->len > 0) {
        /*
         * Postpone this DMA action
         * if the corresponding dma request is not asserted
         */
        if ((src_hs >= 0) && !(s->req & BIT(src_hs))) {
            break;
        }
        if ((dst_hs >= 0) && !(s->req & BIT(dst_hs))) {
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
        if (src_hs >= 0) {
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
        if (dst_hs >= 0) {
            qemu_set_irq(s->ack[dst_hs], 1);
        }

        /* update the channel transfer size */
        c->len -= len / (c->src_bw >> 3);

        if (c->len == 0) {
            /* update the channel transfer status */
            if (!(c->ccr & CCR_MASK_TC)) {
                s->tcsr |= BIT(c->id);
                if (!(c->cfg & CFG_MASK_TCI)) {
                    s->tcisr |= BIT(c->id);
                }
                ftdmac020_update_irq(s);
            }
            /* try to load next lld */
            if (c->llp) {
                c->llp_cnt += 1;
                dma_memory_read(s->dma, c->llp, &desc, sizeof(desc));

                desc.src  = le32_to_cpu(desc.src);
                desc.dst  = le32_to_cpu(desc.dst);
                desc.next = le32_to_cpu(desc.next);
                desc.size = le32_to_cpu(desc.size);
                desc.ctrl = le32_to_cpu(desc.ctrl);

                c->src = desc.src;
                c->dst = desc.dst;
                c->llp = desc.next & 0xfffffffc;
                c->len = desc.size & 0x003fffff;
                c->ccr = (c->ccr & 0x78f8c081)
                       | (extract32(desc.ctrl, 29, 3) << 24)
                       | ((desc.ctrl & BIT(28)) ? CCR_MASK_TC : 0)
                       | (extract32(desc.ctrl, 25, 3) << 11)
                       | (extract32(desc.ctrl, 22, 3) << 8)
                       | (extract32(desc.ctrl, 20, 2) << 5)
                       | (extract32(desc.ctrl, 18, 2) << 3)
                       | (extract32(desc.ctrl, 16, 2) << 1);
                ftdmac020_chan_ccr_decode(c);

                src = c->src;
                dst = c->dst;
            } else {
                /* clear dma start bit */
                c->ccr &= ~CCR_START;
            }
        }
    }

    /* update dma src/dst address */
    c->src = src;
    c->dst = dst;

    s->busy = -1;
}

static void ftdmac020_chan_reset(Ftdmac020Chan *c)
{
    c->ccr = 0;
    c->cfg = 0;
    c->src = 0;
    c->dst = 0;
    c->llp = 0;
    c->len = 0;
}

static void ftdmac020_bh(void *opaque)
{
    Ftdmac020State *s = FTDMAC020(opaque);
    Ftdmac020Chan  *c = NULL;
    int i, jobs, done;

    ++s->bh_owner;
    jobs = 0;
    done = 0;
    for (i = 0; i < 8; ++i) {
        c = s->chan + i;
        if (c->ccr & CCR_START) {
            ++jobs;
            ftdmac020_chan_start(c);
            if (!(c->ccr & CCR_START)) {
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

static void ftdmac020_handle_req(void *opaque, int line, int level)
{
    Ftdmac020State *s = FTDMAC020(opaque);

    if (level) {
        /*
         * Devices those wait for data from externaI/O
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

static void ftdmac020_chip_reset(Ftdmac020State *s)
{
    int i;

    s->tcisr = 0;
    s->eaisr = 0;
    s->tcsr = 0;
    s->easr = 0;
    s->cesr = 0;
    s->cbsr = 0;
    s->csr  = 0;
    s->sync = 0;

    for (i = 0; i < 8; ++i) {
        ftdmac020_chan_reset(s->chan + i);
    }

    /* We can assume our GPIO have been wired up now */
    for (i = 0; i < 16; ++i) {
        qemu_set_irq(s->ack[i], 0);
    }
    s->req = 0;
}

static uint64_t ftdmac020_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftdmac020State *s = FTDMAC020(opaque);
    Ftdmac020Chan  *c = NULL;
    uint32_t i, ret = 0;

    switch (addr) {
    case REG_ISR:
        /* 1. Checking TC interrupts */
        ret |= s->tcisr & 0xff;
        /* 2. Checking Error interrupts */
        ret |= s->eaisr & 0xff;
        /* 3. Checking Abort interrupts */
        ret |= (s->eaisr >> 16) & 0xff;
        break;
    case REG_TCISR:
        return s->tcisr;
    case REG_EAISR:
        return s->eaisr;
    case REG_TCSR:
        return s->tcsr;
    case REG_EASR:
        return s->easr;
    case REG_CESR:
        for (i = 0; i < 8; ++i) {
            c = s->chan + i;
            ret |= (c->ccr & CCR_START) ? BIT(i) : 0;
        }
        break;
    case REG_CBSR:
        return (s->busy > 0) ? BIT(s->busy) : 0;
    case REG_CSR:
        return s->csr;
    case REG_SYNC:
        return s->sync;
    case REG_REVISION:
        /* rev. = 1.13.0 */
        return 0x00011300;
    case REG_FEATURE:
        /* fifo = 32 bytes, support linked list, 8 channels, AHB0 only */
        return 0x00008105;
    case REG_CHAN_BASE(0) ... REG_CHAN_BASE(7) + 0x14:
        c = s->chan + REG_CHAN_ID(addr);
        switch (addr & 0x1f) {
        case REG_CHAN_CCR:
            return c->ccr;
        case REG_CHAN_CFG:
            ret = c->cfg;
            ret |= (s->busy == c->id) ? (1 << 8) : 0;
            ret |= (c->llp_cnt & 0x0f) << 16;
            break;
        case REG_CHAN_SRC:
            return c->src;
        case REG_CHAN_DST:
            return c->dst;
        case REG_CHAN_LLP:
            return c->llp;
        case REG_CHAN_LEN:
            return c->len;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                "ftdmac020: undefined memory access@%#" HWADDR_PRIx "\n",
                addr);
            break;
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftdmac020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void ftdmac020_mem_write(void    *opaque,
                                hwaddr   addr,
                                uint64_t val,
                                unsigned size)
{
    Ftdmac020State *s = FTDMAC020(opaque);
    Ftdmac020Chan  *c = NULL;

    switch (addr) {
    case REG_TCCLR:
        s->tcisr &= ~((uint32_t)val);
        s->tcsr &= ~((uint32_t)val);
        ftdmac020_update_irq(s);
        break;
    case REG_EACLR:
        s->eaisr &= ~((uint32_t)val);
        s->easr &= ~((uint32_t)val);
        ftdmac020_update_irq(s);
        break;
    case REG_CSR:
        s->csr = (uint32_t)val;
        break;
    case REG_SYNC:
        /* In QEMU, devices are all in the same clock domain
         * so there is nothing needs to be done.
         */
        s->sync = (uint32_t)val;
        break;
    case REG_CHAN_BASE(0) ... REG_CHAN_BASE(7) + 0x14:
        c = s->chan + REG_CHAN_ID(addr);
        switch (addr & 0x1f) {
        case REG_CHAN_CCR:
            if (!(c->ccr & CCR_START) && (val & CCR_START)) {
                c->llp_cnt = 0;
            }
            c->ccr = (uint32_t)val & 0x87FFBFFF;
            if (c->ccr & CCR_START) {
                ftdmac020_chan_ccr_decode(c);
                /* kick-off DMA engine */
                qemu_bh_schedule(s->bh);
            }
            break;
        case REG_CHAN_CFG:
            c->cfg = (uint32_t)val & 0x3eff;
            break;
        case REG_CHAN_SRC:
            c->src = (uint32_t)val;
            break;
        case REG_CHAN_DST:
            c->dst = (uint32_t)val;
            break;
        case REG_CHAN_LLP:
            c->llp = (uint32_t)val & 0xfffffffc;
            break;
        case REG_CHAN_LEN:
            c->len = (uint32_t)val & 0x003fffff;
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                "ftdmac020: undefined memory access@%#" HWADDR_PRIx "\n",
                addr);
            break;
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftdmac020: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftdmac020_mem_read,
    .write = ftdmac020_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftdmac020_reset(DeviceState *ds)
{
    Ftdmac020State *s = FTDMAC020(SYS_BUS_DEVICE(ds));

    ftdmac020_chip_reset(s);
}

static void ftdmac020_realize(DeviceState *dev, Error **errp)
{
    Ftdmac020State *s = FTDMAC020(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    int i;

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTDMAC020,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    for (i = 0; i < 3; ++i) {
        sysbus_init_irq(sbd, &s->irq[i]);
    }
    qdev_init_gpio_in(dev, ftdmac020_handle_req, 16);
    qdev_init_gpio_out(dev, s->ack, 16);

    s->busy = -1;
    s->dma = &address_space_memory;
    s->bh = qemu_bh_new(ftdmac020_bh, s);
    for (i = 0; i < 8; ++i) {
        Ftdmac020Chan *c = s->chan + i;
        c->id   = i;
        c->chip = s;
    }
}

static const VMStateDescription vmstate_ftdmac020 = {
    .name = TYPE_FTDMAC020,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(tcisr, Ftdmac020State),
        VMSTATE_UINT32(eaisr, Ftdmac020State),
        VMSTATE_UINT32(tcsr, Ftdmac020State),
        VMSTATE_UINT32(easr, Ftdmac020State),
        VMSTATE_UINT32(cesr, Ftdmac020State),
        VMSTATE_UINT32(cbsr, Ftdmac020State),
        VMSTATE_UINT32(csr, Ftdmac020State),
        VMSTATE_UINT32(sync, Ftdmac020State),
        VMSTATE_END_OF_LIST()
    }
};

static void ftdmac020_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftdmac020;
    dc->reset   = ftdmac020_reset;
    dc->realize = ftdmac020_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftdmac020_info = {
    .name           = TYPE_FTDMAC020,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(Ftdmac020State),
    .class_init     = ftdmac020_class_init,
};

static void ftdmac020_register_types(void)
{
    type_register_static(&ftdmac020_info);
}

type_init(ftdmac020_register_types)
