/*
 * QEMU model of the FTMAC110 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

/*******************************************************************/
/*               FTMAC110 DMA design issue                         */
/*                                             Dante Su 2010.02.03 */
/*                                                                 */
/* The DMA engine has a weird restriction that its Rx DMA engine   */
/* accepts only 16-bits aligned address, 32-bits aligned is still  */
/* invalid. However this restriction does not apply to Tx DMA.     */
/* Conclusion:                                                     */
/* (1) Tx DMA Buffer Address:                                      */
/*     1 bytes aligned: Invalid                                    */
/*     2 bytes aligned: O.K                                        */
/*     4 bytes aligned: O.K (-> u-boot ZeroCopy is possible)       */
/* (2) Rx DMA Buffer Address:                                      */
/*     1 bytes aligned: Invalid                                    */
/*     2 bytes aligned: O.K                                        */
/*     4 bytes aligned: Invalid                                    */
/*******************************************************************/

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "sysemu/sysemu.h"
#include "sysemu/dma.h"
#include "net/net.h"

#include "hw/arm/faraday.h"
#include "hw/ftmac110.h"

#ifndef DEBUG
#define DEBUG   0
#endif

#define DPRINTF(fmt, ...) \
    do { \
        if (DEBUG) { \
            fprintf(stderr, fmt , ## __VA_ARGS__); \
        } \
    } while (0)

#define TYPE_FTMAC110   "ftmac110"

#define CFG_MAXFRMLEN   1536    /* Max. frame length */
#define CFG_REGSIZE     (0x100 / 4)

typedef struct Ftmac110State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion mmio;

    QEMUBH *bh;
    qemu_irq irq;
    NICState *nic;
    NICConf conf;
    AddressSpace *dma;
    QEMUTimer *qtimer;

    bool phycr_rd;

    struct {
        uint8_t  buf[CFG_MAXFRMLEN];
        uint32_t len;
    } txbuff;

    uint32_t tx_idx;
    uint32_t rx_idx;

    /* HW register cache */
    uint32_t regs[CFG_REGSIZE];
} Ftmac110State;

#define FTMAC110(obj) \
    OBJECT_CHECK(Ftmac110State, obj, TYPE_FTMAC110)

#define MAC_REG32(s, off) \
    ((s)->regs[(off) / 4])

static int ftmac110_mcast_hash(int len, const uint8_t *p)
{
#define CRCPOLY_LE 0xedb88320
    int i;
    uint32_t crc = 0xFFFFFFFF;

    while (len--) {
        crc ^= *p++;
        for (i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
        }
    }

    /* Reverse CRC32 and return MSB 6 bits only */
    return bitrev8(crc >> 24) >> 2;
}

static void
ftmac110_read_rxdesc(Ftmac110State *s, hwaddr addr, Ftmac110RXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    if (addr & 0x0f) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftmac110: Rx desc is not 16-byte aligned!\n"
                 "It's fine in QEMU but the real HW would panic.\n");
    }

    dma_memory_read(s->dma, addr, desc, sizeof(*desc));

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = le32_to_cpu(*p);
    }

    if ((desc->buf & 0x1) || !(desc->buf % 4)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftmac110: rx buffer is not exactly 16-bit aligned.\n");
    }
}

static void
ftmac110_write_rxdesc(Ftmac110State *s, hwaddr addr, Ftmac110RXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = cpu_to_le32(*p);
    }

    dma_memory_write(s->dma, addr, desc, sizeof(*desc));
}

static void
ftmac110_read_txdesc(Ftmac110State *s, hwaddr addr, Ftmac110TXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    if (addr & 0x0f) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftmac110: Tx desc is not 16-byte aligned!\n"
                 "It's fine in QEMU but the real HW would panic.\n");
    }

    dma_memory_read(s->dma, addr, desc, sizeof(*desc));

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = le32_to_cpu(*p);
    }

    if (desc->buf & 0x1) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftmac110: tx buffer is not 16-bit aligned.\n");
    }
}

static void
ftmac110_write_txdesc(Ftmac110State *s, hwaddr addr, Ftmac110TXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = cpu_to_le32(*p);
    }

    dma_memory_write(s->dma, addr, desc, sizeof(*desc));
}

static void ftmac110_update_irq(Ftmac110State *s)
{
    qemu_set_irq(s->irq, !!(MAC_REG32(s, REG_ISR) & MAC_REG32(s, REG_IMR)));
}

static int ftmac110_can_receive(NetClientState *nc)
{
    int ret = 0;
    uint32_t val;
    Ftmac110State *s = qemu_get_nic_opaque(nc);
    Ftmac110RXD rxd;
    hwaddr off = MAC_REG32(s, REG_RXBAR) + s->rx_idx * sizeof(rxd);

    val = MAC_REG32(s, REG_MACCR);
    if ((val & MACCR_RCV_EN) && (val & MACCR_RDMA_EN)) {
        ftmac110_read_rxdesc(s, off, &rxd);
        ret = rxd.owner;
        if (!ret) {
            timer_mod(s->qtimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + 10);
        }
    }

    return ret;
}

static ssize_t ftmac110_receive(NetClientState *nc,
                                const uint8_t  *buf,
                                size_t          size)
{
    const uint8_t *ptr = buf;
    hwaddr off;
    size_t len;
    Ftmac110RXD rxd;
    Ftmac110State *s = qemu_get_nic_opaque(nc);
    int bcst, mcst, ftl, proto;

    MAC_REG32(s, REG_RXPKT) += 1;

    /*
     * Check if it's a long frame. (CRC32 is excluded)
     */
    proto = (buf[12] << 8) | buf[13];
    if (proto == 0x8100) {  /* 802.1Q VLAN */
        ftl = (size > 1518) ? 1 : 0;
    } else {
        ftl = (size > 1514) ? 1 : 0;
    }
    if (ftl) {
        MAC_REG32(s, REG_RXCRCFTL) = (MAC_REG32(s, REG_RXCRCFTL) + 1) & 0xffff;
        DPRINTF("ftmac110_receive: frame too long, drop it\n");
        return -1;
    }

    /* if it's a broadcast */
    if ((buf[0] == 0xff) && (buf[1] == 0xff) && (buf[2] == 0xff)
            && (buf[3] == 0xff) && (buf[4] == 0xff) && (buf[5] == 0xff)) {
        bcst = 1;
        MAC_REG32(s, REG_RXBCST) += 1;
        if (!(MAC_REG32(s, REG_MACCR) & MACCR_RCV_ALL)
            && !(MAC_REG32(s, REG_MACCR) & MACCR_RX_BROADPKT)) {
            DPRINTF("ftmac110_receive: bcst filtered\n");
            return -1;
        }
    } else {
        bcst = 0;
    }

    /* if it's a multicast */
    if ((buf[0] == 0x01) && (buf[1] == 0x00) && (buf[2] == 0x5e)
        && (buf[3] <= 0x7f)) {
        mcst = 1;
        MAC_REG32(s, REG_RXMCST) += 1;
        if (!(MAC_REG32(s, REG_MACCR) & MACCR_RCV_ALL)
            && !(MAC_REG32(s, REG_MACCR) & MACCR_RX_MULTIPKT)) {
            int hash, id;
            if (!(MAC_REG32(s, REG_MACCR) & MACCR_HT_MULTI_EN)) {
                DPRINTF("ftmac110_receive: mcst filtered\n");
                return -1;
            }
            hash = ftmac110_mcast_hash(6, buf);
            id = (hash >= 32) ? 1 : 0;
            if (!(MAC_REG32(s, REG_MHASH0 + id * 4) & BIT(hash % 32))) {
                DPRINTF("ftmac110_receive: mcst filtered\n");
                return -1;
            }
        }
    } else {
        mcst = 0;
    }

    /* check if the destination matches NIC mac address */
    if (!(MAC_REG32(s, REG_MACCR) & MACCR_RCV_ALL) && !bcst && !mcst) {
        if (memcmp(s->conf.macaddr.a, buf, 6)) {
            return -1;
        }
    }

    while (size > 0) {
        off = MAC_REG32(s, REG_RXBAR) + s->rx_idx * sizeof(rxd);
        ftmac110_read_rxdesc(s, off, &rxd);
        if (!rxd.owner) {
            MAC_REG32(s, REG_ISR) |= ISR_NORXBUF;
            DPRINTF("ftmac110: out of rxd!?\n");
            return -1;
        }

        if (ptr == buf) {
            rxd.frs = 1;
        } else {
            rxd.frs = 0;
        }

        len = size > rxd.bufsz ? rxd.bufsz : size;
        dma_memory_write(s->dma, rxd.buf, (uint8_t *)ptr, len);
        ptr  += len;
        size -= len;

        if (size <= 0) {
            rxd.lrs = 1;
        } else {
            rxd.lrs = 0;
        }

        rxd.len = len;
        rxd.bcast = bcst;
        rxd.mcast = mcst;
        rxd.owner = 0;

        /* write-back the rx descriptor */
        ftmac110_write_rxdesc(s, off, &rxd);

        if (rxd.end) {
            s->rx_idx = 0;
        } else {
            s->rx_idx += 1;
        }
    }

    /* update interrupt signal */
    MAC_REG32(s, REG_ISR) |= ISR_RPKT_OK | ISR_RPKT_FINISH;
    ftmac110_update_irq(s);

    return (ssize_t)(uint32_t)(ptr - buf);
}

static void ftmac110_transmit(Ftmac110State *s, uint32_t bar, uint32_t *idx)
{
    hwaddr off;
    uint8_t *buf;
    int ftl, proto;
    Ftmac110TXD txd;

    if ((MAC_REG32(s, REG_MACCR) & (MACCR_XMT_EN | MACCR_XDMA_EN))
            != (MACCR_XMT_EN | MACCR_XDMA_EN)) {
        return;
    }

    do {
        off = bar + (*idx) * sizeof(txd);
        ftmac110_read_txdesc(s, off, &txd);
        if (!txd.owner) {
            MAC_REG32(s, REG_ISR) |= ISR_NOTXBUF;
            break;
        }
        if (txd.fts) {
            s->txbuff.len = 0;
        }
        if (txd.len + s->txbuff.len > sizeof(s->txbuff.buf)) {
            fprintf(stderr, "ftmac110: tx buffer overflow!\n");
            abort();
        }
        buf = s->txbuff.buf + s->txbuff.len;
        dma_memory_read(s->dma, txd.buf, (uint8_t *)buf, txd.len);
        s->txbuff.len += txd.len;
        /* Check if it's a long frame. (CRC32 is excluded) */
        proto = (s->txbuff.buf[12] << 8) | s->txbuff.buf[13];
        if (proto == 0x8100) {
            ftl = (s->txbuff.len > 1518) ? 1 : 0;
        } else {
            ftl = (s->txbuff.len > 1514) ? 1 : 0;
        }
        if (ftl) {
            fprintf(stderr, "ftmac110_transmit: frame too long\n");
            abort();
        }
        if (txd.lts) {
            MAC_REG32(s, REG_TXPKT) += 1;
            if (MAC_REG32(s, REG_MACCR) & MACCR_LOOP_EN) {
                ftmac110_receive(qemu_get_queue(s->nic),
                                 s->txbuff.buf,
                                 s->txbuff.len);
            } else {
                qemu_send_packet(qemu_get_queue(s->nic),
                                 s->txbuff.buf,
                                 s->txbuff.len);
            }
        }
        if (txd.end) {
            *idx = 0;
        } else {
            *idx += 1;
        }
        if (txd.tx2fic) {
            MAC_REG32(s, REG_ISR) |= ISR_XPKT_OK;
        }
        if (txd.txic) {
            MAC_REG32(s, REG_ISR) |= ISR_XPKT_FINISH;
        }
        txd.owner = 0;
        ftmac110_write_txdesc(s, off, &txd);
    } while (1);
}

static void ftmac110_bh(void *opaque)
{
    Ftmac110State *s = FTMAC110(opaque);

    if (MAC_REG32(s, REG_TXBAR)) {
        ftmac110_transmit(s, MAC_REG32(s, REG_TXBAR), &s->tx_idx);
    }

    ftmac110_update_irq(s);
}

static void ftmac110_chip_reset(Ftmac110State *s)
{
    s->phycr_rd = false;
    s->txbuff.len = 0;
    s->tx_idx = 0;
    s->rx_idx = 0;
    memset(s->regs, 0, sizeof(s->regs));

    MAC_REG32(s, REG_APTC) = 0x00000001; /* timing control */
    MAC_REG32(s, REG_DBLAC) = 0x00022f72; /* dma burst, arbitration */
    MAC_REG32(s, REG_REVR) = 0x00000700; /* rev. = 0.7.0 */
    MAC_REG32(s, REG_FEAR) = 0x00000006; /* support full/half duplex */

    if (s->bh) {
        qemu_bh_cancel(s->bh);
    }

    if (s->qtimer) {
        timer_del(s->qtimer);
    }

    ftmac110_update_irq(s);
}

static uint64_t
ftmac110_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftmac110State *s = FTMAC110(opaque);
    uint32_t dev, reg, ret = 0;

    if (addr > REG_RXCRCFTL) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftmac110: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        return ret;
    }

    switch (addr) {
    case REG_ISR:
        ret = MAC_REG32(s, REG_ISR);
        MAC_REG32(s, REG_ISR) = 0;
        ftmac110_update_irq(s);
        break;
    case REG_IMR:
        return MAC_REG32(s, REG_IMR);
    case REG_HMAC:
        return s->conf.macaddr.a[1]
               | (s->conf.macaddr.a[0] << 8);
    case REG_LMAC:
        return s->conf.macaddr.a[5]
               | (s->conf.macaddr.a[4] << 8)
               | (s->conf.macaddr.a[3] << 16)
               | (s->conf.macaddr.a[2] << 24);
    case REG_MACSR:
        ret = MAC_REG32(s, REG_MACSR);
        MAC_REG32(s, REG_MACSR) = 0;
        break;
    case REG_PHYCR:
        dev = extract32(MAC_REG32(s, REG_PHYCR), 16, 5);
        reg = extract32(MAC_REG32(s, REG_PHYCR), 21, 5);
        if (!dev && s->phycr_rd) {
            /* Emulating a Davicom PHY with 100Mbps link state */
            switch (reg) {
            case 0:    /* PHY control register */
                return MAC_REG32(s, REG_PHYCR) | 0x3100;
            case 1:    /* PHY status register */
                return MAC_REG32(s, REG_PHYCR) | 0x786d;
            case 2:    /* PHY ID 1 register */
                return MAC_REG32(s, REG_PHYCR) | 0x0181;
            case 3:    /* PHY ID 2 register */
                return MAC_REG32(s, REG_PHYCR) | 0xb8a0;
            case 4:    /* Autonegotiation advertisement register */
                return MAC_REG32(s, REG_PHYCR) | 0x01e1;
            case 5:    /* Autonegotiation partner abilities register */
                return MAC_REG32(s, REG_PHYCR) | 0x45e1;
            }
        }
        break;
    default:
        ret = s->regs[addr / 4];
        break;
    }

    return ret;
}

static void
ftmac110_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftmac110State *s = FTMAC110(opaque);

    if (addr > REG_BPR) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftmac110: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        return;
    }

    switch (addr) {
    case REG_IMR:
        MAC_REG32(s, REG_IMR) = (uint32_t)val;
        ftmac110_update_irq(s);
        break;
    case REG_HMAC:
        s->conf.macaddr.a[1] = extract32((uint32_t)val, 0, 8);
        s->conf.macaddr.a[0] = extract32((uint32_t)val, 8, 8);
        break;
    case REG_LMAC:
        s->conf.macaddr.a[5] = extract32((uint32_t)val, 0, 8);
        s->conf.macaddr.a[4] = extract32((uint32_t)val, 8, 8);
        s->conf.macaddr.a[3] = extract32((uint32_t)val, 16, 8);
        s->conf.macaddr.a[2] = extract32((uint32_t)val, 24, 8);
        break;
    case REG_MACCR:
        MAC_REG32(s, REG_MACCR) = (uint32_t)val;
        if (val & MACCR_SW_RST) {
            ftmac110_chip_reset(s);
            MAC_REG32(s, REG_MACCR) &= ~MACCR_SW_RST;
        }
        if ((val & MACCR_RCV_EN) && (val & MACCR_RDMA_EN)) {
            if (ftmac110_can_receive(qemu_get_queue(s->nic))) {
                qemu_flush_queued_packets(qemu_get_queue(s->nic));
            }
        } else {
            timer_del(s->qtimer);
        }
        break;
    case REG_PHYCR:
        s->phycr_rd = (val & PHYCR_MDIORD) ? true : false;
        MAC_REG32(s, REG_PHYCR) = (uint32_t)val
                                & ~(PHYCR_MDIOWR | PHYCR_MDIORD);
        break;
    case REG_TXPD:
        qemu_bh_schedule(s->bh);
        break;
    case REG_RXPD:
    case REG_REVR:
    case REG_FEAR:
        break;
    default:
        s->regs[addr / 4] = (uint32_t)val;
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftmac110_mem_read,
    .write = ftmac110_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftmac110_cleanup(NetClientState *nc)
{
    Ftmac110State *s = qemu_get_nic_opaque(nc);

    s->nic = NULL;
}

static NetClientInfo net_ftmac110_info = {
    .type = NET_CLIENT_OPTIONS_KIND_NIC,
    .size = sizeof(NICState),
    .can_receive = ftmac110_can_receive,
    .receive = ftmac110_receive,
    .cleanup = ftmac110_cleanup,
};

static void ftmac110_reset(DeviceState *ds)
{
    Ftmac110State *s = FTMAC110(SYS_BUS_DEVICE(ds));

    ftmac110_chip_reset(s);
}

static void ftmac110_watchdog(void *opaque)
{
    Ftmac110State *s = FTMAC110(opaque);

    if (ftmac110_can_receive(qemu_get_queue(s->nic))) {
        qemu_flush_queued_packets(qemu_get_queue(s->nic));
    }
}

static void ftmac110_realize(DeviceState *dev, Error **errp)
{
    Ftmac110State *s = FTMAC110(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->mmio, OBJECT(s), &mmio_ops, s, TYPE_FTMAC110, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);

    qemu_macaddr_default_if_unset(&s->conf.macaddr);
    s->nic = qemu_new_nic(&net_ftmac110_info,
                          &s->conf,
                          object_get_typename(OBJECT(dev)),
                          dev->id,
                          s);
    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->conf.macaddr.a);

    s->qtimer = timer_new_ms(QEMU_CLOCK_VIRTUAL, ftmac110_watchdog, s);
    s->dma = &address_space_memory;
    s->bh = qemu_bh_new(ftmac110_bh, s);

    ftmac110_chip_reset(s);
}

static const VMStateDescription vmstate_ftmac110 = {
    .name = TYPE_FTMAC110,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, Ftmac110State, CFG_REGSIZE),
        VMSTATE_END_OF_LIST()
    }
};

static Property ftmac110_properties[] = {
    DEFINE_NIC_PROPERTIES(Ftmac110State, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static void ftmac110_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd  = &vmstate_ftmac110;
    dc->props = ftmac110_properties;
    dc->reset = ftmac110_reset;
    dc->realize = ftmac110_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftmac110_info = {
    .name           = TYPE_FTMAC110,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(Ftmac110State),
    .class_init     = ftmac110_class_init,
};

static void ftmac110_register_types(void)
{
    type_register_static(&ftmac110_info);
}

/* Legacy helper function.  Should go away when machine config files are
   implemented.  */
void ftmac110_init(NICInfo *nd, uint32_t base, qemu_irq irq)
{
    DeviceState *dev;
    SysBusDevice *s;

    qemu_check_nic_model(nd, TYPE_FTMAC110);
    dev = qdev_create(NULL, TYPE_FTMAC110);
    qdev_set_nic_properties(dev, nd);
    qdev_init_nofail(dev);
    s = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(s, 0, base);
    sysbus_connect_irq(s, 0, irq);
}

type_init(ftmac110_register_types)
