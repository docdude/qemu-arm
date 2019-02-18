/*
 * QEMU model of the FTMAC110 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTMAC110_H
#define HW_ARM_FTMAC110_H

#define REG_ISR             0x00    /* interrupt status */
#define REG_IMR             0x04    /* interrupt mask */
#define REG_HMAC            0x08    /* mac address */
#define REG_LMAC            0x0c
#define REG_MHASH0          0x10    /* multicast hash table */
#define REG_MHASH1          0x14
#define REG_TXPD            0x18    /* kick tx dma */
#define REG_RXPD            0x1c    /* kick rx dma */
#define REG_TXBAR           0x20    /* tx list/ring base address */
#define REG_RXBAR           0x24    /* rx list/ring base address */
#define REG_ITC             0x28    /* interrupt timer control */
#define REG_APTC            0x2C    /* auto polling timer control */
#define REG_DBLAC           0x30    /* dma burst length, arbitration control */
#define REG_REVR            0x34    /* revision */
#define REG_FEAR            0x38    /* feature */

#define REG_MACCR           0x88    /* mac control register */
#define REG_MACSR           0x8C    /* mac status register */
#define REG_PHYCR           0x90    /* phy control register */
#define REG_PHYDR           0x94    /* phy data register */
#define REG_FCR             0x98    /* flow control register */
#define REG_BPR             0x9c    /* back pressure register */

#define REG_TXPKT           0xf8    /* tx packet counter */
#define REG_RXPKT           0xf4    /* rx packet counter */
#define REG_RXBCST          0xec    /* rx broadcast packet counter */
#define REG_RXMCST          0xf0    /* rx multicast packet counter */
#define REG_RXRUNT          0xe0    /* rx err: runt */
#define REG_RXCRCFTL        0xc4    /* rx err: BIT[31-16]#crc; BIT[15-0]#ftl */

/* interrupt status register */
#define ISR_PHYSTS_CHG      (1UL<<9)/* phy status change */
#define ISR_AHB_ERR         (1UL<<8)/* AHB error */
#define ISR_RPKT_LOST       (1UL<<7)/* rx packet lost */
#define ISR_RPKT_OK         (1UL<<6)/* rx packet ok (fifo) */
#define ISR_XPKT_LOST       (1UL<<5)/* tx packet lost */
#define ISR_XPKT_OK         (1UL<<4)/* tx packet ok (fifo) */
#define ISR_NOTXBUF         (1UL<<3)/* out of tx buffer */
#define ISR_XPKT_FINISH     (1UL<<2)/* tx packet finished (phy) */
#define ISR_NORXBUF         (1UL<<1)/* out of rx buffer */
#define ISR_RPKT_FINISH     (1UL<<0)/* rx packet finished (ram) */

/* MAC control register */
#define MACCR_100M              (1UL<<18)/* 100Mbps */
#define MACCR_RX_BROADPKT       (1UL<<17)/* recv all broadcast packets */
#define MACCR_RX_MULTIPKT       (1UL<<16)/* recv all multicast packets */
#define MACCR_FULLDUP           (1UL<<15)/* full duplex */
#define MACCR_CRC_APD           (1UL<<14)/* crc append */
#define MACCR_RCV_ALL           (1UL<<12)/* recv all packets */
#define MACCR_RX_FTL            (1UL<<11)/* recv ftl packets */
#define MACCR_RX_RUNT           (1UL<<10)/* recv runt packets */
#define MACCR_HT_MULTI_EN       (1UL<<9) /* recv multicast by hash */
#define MACCR_RCV_EN            (1UL<<8) /* rx enabled */
#define MACCR_ENRX_IN_HALFTX    (1UL<<6) /* rx while tx in half duplex */
#define MACCR_XMT_EN            (1UL<<5) /* tx enabled */
#define MACCR_CRC_DIS           (1UL<<4) /* discard crc erro */
#define MACCR_LOOP_EN           (1UL<<3) /* loopback */
#define MACCR_SW_RST            (1UL<<2) /* software reset */
#define MACCR_RDMA_EN           (1UL<<1) /* rx dma enabled */
#define MACCR_XDMA_EN           (1UL<<0) /* tx dma enabled */

/*
 * MDIO
 */
#define PHYCR_MDIOWR            (1 << 27)/* mdio write */
#define PHYCR_MDIORD            (1 << 26)/* mdio read */

/*
 * Tx/Rx descriptors
 */
typedef struct Ftmac110RXD {
    /* RXDES0 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t owner:1;   /* BIT31: owner - 1:HW, 0: SW */
    uint32_t rsvd3:1;
    uint32_t frs:1;     /* first rx segment */
    uint32_t lrs:1;     /* last rx segment */
    uint32_t rsvd2:5;
    uint32_t error:5;   /* rx error */
    uint32_t bcast:1;   /* broadcast */
    uint32_t mcast:1;   /* multicast */
    uint32_t rsvd1:5;
    uint32_t len:11;    /* received packet length */
#else
    uint32_t len:11;
    uint32_t rsvd1:5;
    uint32_t mcast:1;
    uint32_t bcast:1;
    uint32_t error:5;
    uint32_t rsvd2:5;
    uint32_t lrs:1;
    uint32_t frs:1;
    uint32_t rsvd3:1;
    uint32_t owner:1;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* RXDES1 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t end:1;     /* end of descriptor list/ring */
    uint32_t rsvd4:20;
    uint32_t bufsz:11;  /* max. packet length */
#else
    uint32_t bufsz:11;
    uint32_t rsvd4:20;
    uint32_t end:1;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* RXDES2 */
    uint32_t buf;

    /* RXDES3 */
    void     *skb;
} __attribute__ ((aligned (16))) Ftmac110RXD;

typedef struct Ftmac110TXD {
    /* TXDES0 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t owner:1;   /* BIT31: owner - 1:HW, 0: SW */
    uint32_t rsvd1:29;
    uint32_t error:2;
#else
    uint32_t error:2;
    uint32_t rsvd1:29;
    uint32_t owner:1;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* TXDES1 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t end:1;     /* end of descriptor list/ring */
    uint32_t txic:1;    /* interrupt when data has been copied to phy */
    uint32_t tx2fic:1;  /* interrupt when data has been copied to fifo */
    uint32_t fts:1;     /* first tx segment */
    uint32_t lts:1;     /* last tx segment */
    uint32_t rsvd2:16;
    uint32_t len:11;    /* packet length */
#else
    uint32_t len:11;
    uint32_t rsvd2:16;
    uint32_t lts:1;
    uint32_t fts:1;
    uint32_t tx2fic:1;
    uint32_t txic:1;
    uint32_t end:1;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* TXDES2 */
    uint32_t buf;

    /* TXDES3 */
    void     *skb;

} __attribute__ ((aligned (16))) Ftmac110TXD;

#endif  /* HW_ARM_FTMAC110_H */
