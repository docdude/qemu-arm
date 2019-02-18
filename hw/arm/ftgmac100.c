/*
 * QEMU model of the FTGMAC100 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "sysemu/sysemu.h"
#include "sysemu/dma.h"
#include "net/net.h"

#include "hw/arm/faraday.h"
#include "hw/ftgmac100.h"

#ifndef DEBUG
#define DEBUG   0
#endif

#define DPRINTF(fmt, ...) \
    do { \
        if (DEBUG) { \
            fprintf(stderr, fmt , ## __VA_ARGS__); \
        } \
    } while (0)

#define TYPE_FTGMAC100  "ftgmac100"

#define CFG_MAXFRMLEN   9220    /* Max. frame length */
#define CFG_REGSIZE     (0x100 / 4)

typedef struct Ftgmac100State {
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

    uint32_t hptx_idx;
    uint32_t tx_idx;
    uint32_t rx_idx;

    /* HW register cache */
    uint32_t regs[CFG_REGSIZE];
} Ftgmac100State;

#define FTGMAC100(obj) \
    OBJECT_CHECK(Ftgmac100State, obj, TYPE_FTGMAC100)

#define MAC_REG32(s, off) \
    ((s)->regs[(off) / 4])

static int ftgmac100_mcast_hash(Ftgmac100State *s, const uint8_t *data)
{
#define CRCPOLY_BE    0x04c11db7
    int i, len;
    uint32_t crc = 0xFFFFFFFF;

    len = (MAC_REG32(s, REG_MACCR) & MACCR_GMODE) ? 5 : 6;

    while (len--) {
        uint32_t c = *(data++);
        for (i = 0; i < 8; ++i) {
            crc = (crc << 1) ^ ((((crc >> 31) ^ c) & 0x01) ? CRCPOLY_BE : 0);
            c >>= 1;
        }
    }
    crc = ~crc;

    /* Reverse CRC32 and return MSB 6 bits only */
    return bitrev8(crc >> 24) >> 2;
}

static void
ftgmac100_read_txdesc(Ftgmac100State *s, hwaddr addr, Ftgmac100TXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    if (addr & 0x0f) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftgmac100: Tx desc is not 16-byte aligned!\n"
                 "It's fine in QEMU but the real HW would panic.\n");
    }

    dma_memory_read(s->dma, addr, desc, sizeof(*desc));

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = le32_to_cpu(*p);
    }

    if (desc->buf & 0x01) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftgmac100: tx buffer is not 16-bit aligned!\n");
    }
}

static void
ftgmac100_write_txdesc(Ftgmac100State *s, hwaddr addr, Ftgmac100TXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = cpu_to_le32(*p);
    }

    dma_memory_write(s->dma, addr, desc, sizeof(*desc));
}

static void
ftgmac100_read_rxdesc(Ftgmac100State *s, hwaddr addr, Ftgmac100RXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    if (addr & 0x0f) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftgmac100: Rx desc is not 16-byte aligned!\n"
                 "It's fine in QEMU but the real HW would panic.\n");
    }

    dma_memory_read(s->dma, addr, desc, sizeof(*desc));

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = le32_to_cpu(*p);
    }

    if (desc->buf & 0x01) {
        qemu_log_mask(LOG_GUEST_ERROR,
                 "ftgmac100: rx buffer is not 16-bit aligned!\n");
    }
}

static void
ftgmac100_write_rxdesc(Ftgmac100State *s, hwaddr addr, Ftgmac100RXD *desc)
{
    int i;
    uint32_t *p = (uint32_t *)desc;

    for (i = 0; i < sizeof(*desc); i += 4) {
        *p = cpu_to_le32(*p);
    }

    dma_memory_write(s->dma, addr, desc, sizeof(*desc));
}

static void ftgmac100_update_irq(Ftgmac100State *s)
{
    qemu_set_irq(s->irq, !!(MAC_REG32(s, REG_ISR) & MAC_REG32(s, REG_IMR)));
}

static int ftgmac100_can_receive(NetClientState *nc)
{
    int ret = 0;
    uint32_t val;
    Ftgmac100State *s = qemu_get_nic_opaque(nc);
    Ftgmac100RXD rxd;
    hwaddr off = MAC_REG32(s, REG_RXBAR) + s->rx_idx * sizeof(rxd);

    val = MAC_REG32(s, REG_MACCR);
    if ((val & MACCR_RCV_EN) && (val & MACCR_RDMA_EN)) {
        ftgmac100_read_rxdesc(s, off, &rxd);
        ret = !rxd.owner;
        if (!ret) {
            timer_mod(s->qtimer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + 10);
        }
    }

    return ret;
}

static ssize_t ftgmac100_receive(NetClientState *nc,
                                 const uint8_t  *buf,
                                 size_t          size)
{
    const uint8_t *ptr = buf;
    hwaddr off;
    size_t len;
    Ftgmac100RXD rxd;
    Ftgmac100State *s = qemu_get_nic_opaque(nc);
    int bcst, mcst, ftl, proto;

    MAC_REG32(s, REG_RXPKT) += 1;

    /*
     * Check if it's a long frame. (CRC32 is excluded)
     */
    ftl = 0;
    proto = (buf[12] << 8) | buf[13];
    if (MAC_REG32(s, REG_MACCR) & MACCR_JUMBO_LF) {
        if (proto == 0x8100) {  /* 802.1Q VLAN */
            ftl = (size > 9216) ? 1 : 0;
        } else {
            ftl = (size > 9212) ? 1 : 0;
        }
    } else {
        if (proto == 0x8100) {  /* 802.1Q VLAN */
            ftl = (size > 1518) ? 1 : 0;
        } else {
            ftl = (size > 1514) ? 1 : 0;
        }
    }
    if (ftl) {
        MAC_REG32(s, REG_RXCRCFTL) = (MAC_REG32(s, REG_RXCRCFTL) + 1) & 0xffff;
        DPRINTF("ftgmac100_receive: frame too long, drop it\n");
        return -1;
    }

    /* if it's a broadcast */
    if ((buf[0] == 0xff) && (buf[1] == 0xff) && (buf[2] == 0xff)
            && (buf[3] == 0xff) && (buf[4] == 0xff) && (buf[5] == 0xff)) {
        bcst = 1;
        MAC_REG32(s, REG_RXBCST) += 1;
        if (!(MAC_REG32(s, REG_MACCR) & MACCR_RCV_ALL)
            && !(MAC_REG32(s, REG_MACCR) & MACCR_RX_BROADPKT)) {
            DPRINTF("ftgmac100_receive: bcst filtered\n");
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
                DPRINTF("ftgmac100_receive: mcst filtered\n");
                return -1;
            }
            hash = ftgmac100_mcast_hash(s, buf);
            id = (hash >= 32) ? 1 : 0;
            if (!(MAC_REG32(s, REG_MHASH0 + id * 4) & BIT(hash % 32))) {
                DPRINTF("ftgmac100_receive: mcst filtered\n");
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
        ftgmac100_read_rxdesc(s, off, &rxd);
        if (rxd.owner) {
            MAC_REG32(s, REG_ISR) |= ISR_NORXBUF;
            DPRINTF("ftgmac100: out of rxd!?\n");
            return -1;
        }
        len = size > rxd.len ? rxd.len : size;
        rxd.frs = (ptr == buf) ? 1 : 0;

        if (rxd.frs && proto == 0x8100 && len >= 18) {
            rxd.vlan = 1;
            rxd.vlantag = (buf[14] << 8) | buf[15];
            if (MAC_REG32(s, REG_MACCR) & MACCR_VLAN_RM) {
                /* copy src/dst mac address */
                dma_memory_write(s->dma, rxd.buf, ptr, 12);
                /* copy proto + payload, the VLAN tag is ignored */
                dma_memory_write(s->dma, rxd.buf + 12, ptr + 16, len - 16);
            } else {
                dma_memory_write(s->dma, rxd.buf, ptr, len);
            }
        } else {
            rxd.vlan = 0;
            rxd.vlantag = 0;
            dma_memory_write(s->dma, rxd.buf, ptr, len);
        }
        ptr  += len;
        size -= len;

        rxd.lrs = (size <= 0) ? 1 : 0;
        rxd.len = len;
        rxd.bcast = bcst;
        rxd.mcast = mcst;
        rxd.owner = 1;

        /* write-back the rx descriptor */
        ftgmac100_write_rxdesc(s, off, &rxd);

        if (rxd.end) {
            s->rx_idx = 0;
        } else {
            s->rx_idx += 1;
        }
    }

    /* update interrupt signal */
    MAC_REG32(s, REG_ISR) |= ISR_RPKT_OK | ISR_RPKT_FINISH;
    ftgmac100_update_irq(s);

    return (ssize_t)(uint32_t)(ptr - buf);
}

static void
ftgmac100_transmit(Ftgmac100State *s, uint32_t bar, uint32_t *idx)
{
    hwaddr off;
    uint8_t *buf;
    int ftl, proto;
    Ftgmac100TXD txd;

    if ((MAC_REG32(s, REG_MACCR) & (MACCR_XMT_EN | MACCR_XDMA_EN))
            != (MACCR_XMT_EN | MACCR_XDMA_EN)) {
        return;
    }

    do {
        off = bar + (*idx) * sizeof(txd);
        ftgmac100_read_txdesc(s, off, &txd);
        if (!txd.owner) {
            MAC_REG32(s, REG_ISR) |= ISR_NOTXBUF;
            break;
        }
        if (txd.fts) {
            s->txbuff.len = 0;
        }
        if (txd.len + s->txbuff.len > sizeof(s->txbuff.buf)) {
            fprintf(stderr, "ftgmac100: tx buffer overflow!\n");
            abort();
        }
        buf = s->txbuff.buf + s->txbuff.len;
        dma_memory_read(s->dma, txd.buf, (uint8_t *)buf, txd.len);
        s->txbuff.len += txd.len;
        proto = (s->txbuff.buf[12] << 8) | s->txbuff.buf[13];
        /* Insert VLAN Tag */
        if (txd.fts && txd.vlan && proto != 0x8100) {
            if (s->txbuff.len + 4 > sizeof(s->txbuff.buf)) {
                fprintf(stderr, "ftgmac100: tx buffer overflow!\n");
                abort();
            }
            memmove(s->txbuff.buf + 16,
                    s->txbuff.buf + 12,
                    s->txbuff.len - 12);
            s->txbuff.buf[12] = 0x81;
            s->txbuff.buf[13] = 0x00;
            s->txbuff.buf[14] = (uint8_t)(txd.vlantag >> 8);
            s->txbuff.buf[15] = (uint8_t)(txd.vlantag >> 0);
            s->txbuff.len += 4;
            proto = 0x8100;
        }
        /* Check if it's a long frame. (CRC32 is excluded) */
        if (MAC_REG32(s, REG_MACCR) & MACCR_JUMBO_LF) {
            if (proto == 0x8100) {
                ftl = (s->txbuff.len > 9216) ? 1 : 0;
            } else {
                ftl = (s->txbuff.len > 9212) ? 1 : 0;
            }
        } else {
            if (proto == 0x8100) {
                ftl = (s->txbuff.len > 1518) ? 1 : 0;
            } else {
                ftl = (s->txbuff.len > 1514) ? 1 : 0;
            }
        }
        if (ftl) {
            fprintf(stderr, "ftgmac100_transmit: frame too long\n");
            abort();
        }
        if (txd.lts) {
            MAC_REG32(s, REG_TXPKT) += 1;
            if (MAC_REG32(s, REG_MACCR) & MACCR_LOOP_EN) {
                ftgmac100_receive(qemu_get_queue(s->nic),
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
        ftgmac100_write_txdesc(s, off, &txd);
    } while (1);
}

static void ftgmac100_bh(void *opaque)
{
    Ftgmac100State *s = FTGMAC100(opaque);

    /* 1. process high priority tx ring */
    if (MAC_REG32(s, REG_HPTXBAR)
        && (MAC_REG32(s, REG_MACCR) & MACCR_HPTXR_EN)) {
        ftgmac100_transmit(s, MAC_REG32(s, REG_HPTXBAR), &s->hptx_idx);
    }

    /* 2. process normal priority tx ring */
    if (MAC_REG32(s, REG_TXBAR)) {
        ftgmac100_transmit(s, MAC_REG32(s, REG_TXBAR), &s->tx_idx);
    }

    /* 3. update interrupt signal */
    ftgmac100_update_irq(s);
}

static void ftgmac100_chip_reset(Ftgmac100State *s)
{
    s->phycr_rd = false;
    s->txbuff.len = 0;
    s->hptx_idx = 0;
    s->tx_idx = 0;
    s->rx_idx = 0;
    memset(s->regs, 0, sizeof(s->regs));

    MAC_REG32(s, REG_APTC) = 0x00000001; /* timing control */
    MAC_REG32(s, REG_DBLAC) = 0x00022f72; /* dma burst, arbitration */
    MAC_REG32(s, REG_DMAFIFO) = 0x0c000000; /* tx/rx fifo empty */
    MAC_REG32(s, REG_REVR) = 0x00000600; /* rev. = 0.6.0 */
    MAC_REG32(s, REG_FEAR) = 0x0000001b; /* fifo = 16KB */
    MAC_REG32(s, REG_TPAFCR) = 0x000000f1; /* tx priority, arbitration */
    MAC_REG32(s, REG_RBSR) = 0x00000640; /* rx buffer size */

    if (s->bh) {
        qemu_bh_cancel(s->bh);
    }

    if (s->qtimer) {
        timer_del(s->qtimer);
    }

    ftgmac100_update_irq(s);
}

static uint64_t
ftgmac100_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftgmac100State *s = FTGMAC100(opaque);
    uint32_t dev, reg, ret = 0;

    if (addr > REG_RXNOBCOL) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftgmac100: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        return ret;
    }

    switch (addr) {
    case REG_HMAC:
        ret = s->conf.macaddr.a[1] | (s->conf.macaddr.a[0] << 8);
        break;
    case REG_LMAC:
        ret = s->conf.macaddr.a[5] | (s->conf.macaddr.a[4] << 8)
            | (s->conf.macaddr.a[3] << 16) | (s->conf.macaddr.a[2] << 24);
        break;
    case REG_PHYDR:
        dev = extract32(MAC_REG32(s, REG_PHYCR), 16, 5);
        reg = extract32(MAC_REG32(s, REG_PHYCR), 21, 5);
        /* Emulating a Marvell 88E1111 with 1Gbps link state */
        if (!dev && s->phycr_rd) {
            switch (reg) {
            case 0:    /* PHY control register */
                return 0x1140 << 16;
            case 1:    /* PHY status register */
                return 0x796d << 16;
            case 2:    /* PHY ID 1 register */
                return 0x0141 << 16;
            case 3:    /* PHY ID 2 register */
                return 0x0cc2 << 16;
            case 4:    /* Autonegotiation advertisement register */
                return 0x0de1 << 16;
            case 5:    /* Autonegotiation partner abilities register */
                return 0x45e1 << 16;
            case 17:/* Marvell 88E1111 */
                ret = (2 << 14) | (1 << 13) | (1 << 11) | (1 << 10);
                return ret << 16;
            }
        }
        break;
    case REG_RXPTR:
        ret = s->rx_idx;
        break;
    case REG_TXPTR:
        ret = s->tx_idx;
        break;
    case REG_HPTXPTR:
        ret = s->hptx_idx;
        break;
    default:
        ret = s->regs[addr / 4];
        break;
    }

    return ret;
}

static void
ftgmac100_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftgmac100State *s = FTGMAC100(opaque);

    if (addr >= REG_TXPTR) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftgmac100: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        return;
    }

    switch (addr) {
    case REG_ISR:
        MAC_REG32(s, REG_ISR) &= ~((uint32_t)val);
        ftgmac100_update_irq(s);
        break;
    case REG_IMR:
        MAC_REG32(s, REG_IMR) = (uint32_t)val;
        ftgmac100_update_irq(s);
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
            ftgmac100_chip_reset(s);
            MAC_REG32(s, REG_MACCR) &= ~MACCR_SW_RST;
        }
        if ((val & MACCR_RCV_EN) && (val & MACCR_RDMA_EN)) {
            if (ftgmac100_can_receive(qemu_get_queue(s->nic))) {
                qemu_flush_queued_packets(qemu_get_queue(s->nic));
            }
        } else {
            timer_del(s->qtimer);
        }
        break;
    case REG_MACSR:
        MAC_REG32(s, REG_MACSR) &= ~((uint32_t)val);
        break;
    case REG_PHYCR:
        s->phycr_rd = (val & PHYCR_MDIORD) ? true : false;
        MAC_REG32(s, REG_PHYCR) = (uint32_t)val
                                & (~(PHYCR_MDIOWR | PHYCR_MDIORD));
        break;
    case REG_TXPD:
    case REG_HPTXPD:
        qemu_bh_schedule(s->bh);
        break;
    case REG_RXPD:
    case REG_DMAFIFO:
    case REG_REVR:
    case REG_FEAR:
        break;
    default:
        s->regs[addr / 4] = (uint32_t)val;
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftgmac100_mem_read,
    .write = ftgmac100_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftgmac100_cleanup(NetClientState *nc)
{
    Ftgmac100State *s = qemu_get_nic_opaque(nc);

    s->nic = NULL;
}

static NetClientInfo net_ftgmac100_info = {
    .type = NET_CLIENT_OPTIONS_KIND_NIC,
    .size = sizeof(NICState),
    .can_receive = ftgmac100_can_receive,
    .receive = ftgmac100_receive,
    .cleanup = ftgmac100_cleanup,
};

static void ftgmac100_reset(DeviceState *ds)
{
    Ftgmac100State *s = FTGMAC100(SYS_BUS_DEVICE(ds));

    ftgmac100_chip_reset(s);
}

static void ftgmac100_watchdog(void *opaque)
{
    Ftgmac100State *s = FTGMAC100(opaque);

    if (ftgmac100_can_receive(qemu_get_queue(s->nic))) {
        qemu_flush_queued_packets(qemu_get_queue(s->nic));
    }
}

static void ftgmac100_realize(DeviceState *dev, Error **errp)
{
    Ftgmac100State *s = FTGMAC100(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->mmio, OBJECT(s), &mmio_ops, s, TYPE_FTGMAC100, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);

    qemu_macaddr_default_if_unset(&s->conf.macaddr);
    s->nic = qemu_new_nic(&net_ftgmac100_info,
                          &s->conf,
                          object_get_typename(OBJECT(dev)),
                          dev->id,
                          s);
    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->conf.macaddr.a);

    s->qtimer = timer_new_ms(QEMU_CLOCK_VIRTUAL, ftgmac100_watchdog, s);
    s->dma = &address_space_memory;
    s->bh = qemu_bh_new(ftgmac100_bh, s);

    ftgmac100_chip_reset(s);
}

static const VMStateDescription vmstate_ftgmac100 = {
    .name = TYPE_FTGMAC100,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, Ftgmac100State, CFG_REGSIZE),
        VMSTATE_END_OF_LIST()
    }
};

static Property ftgmac100_properties[] = {
    DEFINE_NIC_PROPERTIES(Ftgmac100State, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static void ftgmac100_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd  = &vmstate_ftgmac100;
    dc->props = ftgmac100_properties;
    dc->reset = ftgmac100_reset;
    dc->realize = ftgmac100_realize;
 //   dc->no_user = 1;
}

static const TypeInfo ftgmac100_info = {
    .name           = TYPE_FTGMAC100,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(Ftgmac100State),
    .class_init     = ftgmac100_class_init,
};

static void ftgmac100_register_types(void)
{
    type_register_static(&ftgmac100_info);
}

/* Legacy helper function.  Should go away when machine config files are
   implemented.  */
void ftgmac100_init(NICInfo *nd, uint32_t base, qemu_irq irq)
{
    DeviceState *dev;
    SysBusDevice *s;

    qemu_check_nic_model(nd, TYPE_FTGMAC100);
    dev = qdev_create(NULL, TYPE_FTGMAC100);
    qdev_set_nic_properties(dev, nd);
    qdev_init_nofail(dev);
    s = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(s, 0, base);
    sysbus_connect_irq(s, 0, irq);
}

type_init(ftgmac100_register_types)
