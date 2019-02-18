/*
 * QEMU model of the FTGMAC100 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTGMAC100_H
#define HW_ARM_FTGMAC100_H

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
#define REG_HPTXPD          0x28    /* kick high priority tx dma */
#define REG_HPTXBAR         0x2C    /* high priority tx list/ring base */
#define REG_ITC             0x30    /* interrupt timer control */
#define REG_APTC            0x34    /* auto polling timer control */
#define REG_DBLAC           0x38    /* dma burst length, arbitration control */
#define REG_DMAFIFO         0x3C    /* dma fifo */
#define REG_REVR            0x40    /* revision */
#define REG_FEAR            0x44    /* feature */
#define REG_TPAFCR          0x48    /* tx priority arbitration&fifo control */
#define REG_RBSR            0x4C    /* rx buffer size register */
#define REG_MACCR           0x50    /* mac control register */
#define REG_MACSR           0x54    /* mac status register */
#define REG_TSTMODE         0x58    /* test mode register */
#define REG_PHYCR           0x60    /* phy control register */
#define REG_PHYDR           0x64    /* phy data register */
#define REG_FCR             0x68    /* flow control register */
#define REG_BPR             0x6c    /* back pressure register */
#define REG_WOLCR           0x70    /* wake-on-lan control */
#define REG_WOLSR           0x74    /* wake-on-lan status */
#define REG_WFCRC           0x78    /* wake frame crc */
#define REG_WFBM1           0x80    /* wake frame byte mask */
#define REG_WFBM2           0x84
#define REG_WFBM3           0x88
#define REG_WFBM4           0x8c
#define REG_TXPTR           0x90    /* tx pointer */
#define REG_HPTXPTR         0x94    /* high priority tx pointer */
#define REG_RXPTR           0x98    /* rx pointer */
#define REG_TXPKT           0xa0    /* tx counter */
#define REG_TXERR0          0xa4    /* tx error */
#define REG_TXERR1          0xa8
#define REG_TXERR2          0xac
#define REG_RXPKT           0xb0    /* rx counter */
#define REG_RXBCST          0xb4    /* rx bcst counter */
#define REG_RXMCST          0xb8    /* rx mcst counter */
#define REG_RXRUNT          0xc0    /* rx err: runt */
#define REG_RXCRCFTL        0xc4    /* rx err: BIT[31-16]#crc; BIT[15-0]#ftl */
#define REG_RXNOBCOL        0xc8    /* rx err: BIT[31-16]#nob; BIT[15-0]#col */

/* interrupt status register */
#define ISR_NOHTXB          (1UL<<10)   /* out of high priority tx buffer */
#define ISR_PHYSTS_CHG      (1UL<<9)    /* phy status change */
#define ISR_AHB_ERR         (1UL<<8)    /* AHB error */
#define ISR_XPKT_LOST       (1UL<<7)    /* tx packet lost */
#define ISR_NOTXBUF         (1UL<<6)    /* out of tx buffer */
#define ISR_XPKT_OK         (1UL<<5)    /* tx packet ok (fifo) */
#define ISR_XPKT_FINISH     (1UL<<4)    /* tx packet finished (phy) */
#define ISR_RPKT_LOST       (1UL<<3)    /* rx packet lost */
#define ISR_NORXBUF         (1UL<<2)    /* out of rx buffer */
#define ISR_RPKT_OK         (1UL<<1)    /* rx packet ok (fifo) */
#define ISR_RPKT_FINISH     (1UL<<0)    /* rx packet finished (ram) */

/* MAC control register */
#define MACCR_SW_RST            (1UL<<31)   /* software reset */
#define MACCR_100M              (1UL<<19)   /* 100Mbps */
#define MACCR_CRC_DIS           (1UL<<18)   /* discard crc error */
#define MACCR_RX_BROADPKT       (1UL<<17)   /* recv all broadcast packets */
#define MACCR_RX_MULTIPKT       (1UL<<16)   /* recv all multicast packets */
#define MACCR_HT_MULTI_EN       (1UL<<15)   /* recv multicast by hash */
#define MACCR_RCV_ALL           (1UL<<14)   /* recv all packets */
#define MACCR_JUMBO_LF          (1UL<<13)   /* support jumbo frame */
#define MACCR_RX_RUNT           (1UL<<12)   /* recv runt packets */
#define MACCR_CRC_APD           (1UL<<10)   /* crc append */
#define MACCR_GMODE             (1UL<<9)    /* giga mode */
#define MACCR_FULLDUP           (1UL<<8)    /* full duplex */
#define MACCR_ENRX_IN_HALFTX    (1UL<<7)    /* rx while tx in half duplex */
#define MACCR_LOOP_EN           (1UL<<6)    /* loopback */
#define MACCR_HPTXR_EN          (1UL<<5)    /* high priority tx enabled */
#define MACCR_VLAN_RM           (1UL<<4)    /* vlan removal enabled */
#define MACCR_RCV_EN            (1UL<<3)    /* rx enabled */
#define MACCR_XMT_EN            (1UL<<2)    /* tx enabled */
#define MACCR_RDMA_EN           (1UL<<1)    /* rx dma enabled */
#define MACCR_XDMA_EN           (1UL<<0)    /* tx dma enabled */

/*
 * MDIO
 */
#define PHYCR_MDIOWR            (1 << 27)   /* mdio write */
#define PHYCR_MDIORD            (1 << 26)   /* mdio read */

/*
 * Tx/Rx Descriptors
 */
typedef struct Ftgmac100RXD {
    /* RXDES0 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t owner:1;   /* BIT31: owner - 1:SW, 0: HW */
    uint32_t rsvd3:1;
    uint32_t frs:1;     /* first rx segment */
    uint32_t lrs:1;     /* last rx segment */
    uint32_t rsvd2:2;
    uint32_t pausefrm:1;/* pause frame received */
    uint32_t pauseopc:1;/* pause frame OP code */
    uint32_t fifofull:1;/* packet dropped due to FIFO full */
    uint32_t oddnb:1;   /* odd nibbles */
    uint32_t runt:1;    /* runt packet (< 64 bytes) */
    uint32_t ftl:1;     /* frame too long */
    uint32_t crcerr:1;  /* crc error */
    uint32_t rxerr:1;   /* rx error */
    uint32_t bcast:1;   /* broadcast */
    uint32_t mcast:1;   /* multicast */
    uint32_t end:1;     /* end of descriptor list/ring */
    uint32_t rsvd1:1;
    uint32_t len:14;    /* max./received packet length */
#else
    uint32_t len:14;
    uint32_t rsvd1:1;
    uint32_t end:1;
    uint32_t mcast:1;
    uint32_t bcast:1;
    uint32_t rxerr:1;
    uint32_t crcerr:1;
    uint32_t ftl:1;
    uint32_t runt:1;
    uint32_t oddnb:1;
    uint32_t fifofull:1;
    uint32_t pauseopc:1;
    uint32_t pausefrm:1;
    uint32_t rsvd2:2;
    uint32_t lrs:1;
    uint32_t frs:1;
    uint32_t rsvd3:1;
    uint32_t owner:1;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* RXDES1 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t rsvd5:4;
    uint32_t ipcs:1;    /* IPv4 checksum failed */
    uint32_t udpcs:1;   /* IPv4 UDP checksum failed */
    uint32_t tcpcs:1;   /* IPv4 TCP checksum failed */
    uint32_t vlan:1;    /* VLAN tag detected */
    uint32_t nofrag:1;  /* Not Fragmented */
    uint32_t llc:1;     /* LLC packet detected */
    uint32_t proto:2;   /* Protocol Type */
    uint32_t rsvd4:4;
    uint32_t vlantag:16;/* VLAN tag */
#else
    uint32_t vlantag:16;
    uint32_t rsvd4:4;
    uint32_t proto:2;
    uint32_t llc:1;
    uint32_t nofrag:1;
    uint32_t vlan:1;
    uint32_t tcpcs:1;
    uint32_t udpcs:1;
    uint32_t ipcs:1;
    uint32_t rsvd5:4;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* RXDES2 */
    void    *skb;

    /* RXDES3 */
    uint32_t buf;
} __attribute__ ((aligned (16))) Ftgmac100RXD;

typedef struct Ftgmac100TXD {
    /* TXDES0 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t owner:1;   /* BIT31: owner - 1:HW, 0: SW */
    uint32_t rsvd4:1;
    uint32_t fts:1;     /* first tx segment */
    uint32_t lts:1;     /* last tx segment */
    uint32_t rsvd3:8;
    uint32_t crcerr:1;  /* crc error */
    uint32_t rsvd2:3;
    uint32_t end:1;     /* end of descriptor list/ring */
    uint32_t rsvd1:1;
    uint32_t len:14;    /* packet length */
#else
    uint32_t len:14;
    uint32_t rsvd1:1;
    uint32_t end:1;
    uint32_t rsvd2:3;
    uint32_t crcerr:1;
    uint32_t rsvd3:8;
    uint32_t lts:1;
    uint32_t fts:1;
    uint32_t rsvd4:1;
    uint32_t owner:1;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* TXDES1 */
#ifdef HOST_WORDS_BIGENDIAN
    uint32_t txic:1;        /* interrupt when data has been copied to phy */
    uint32_t tx2fic:1;      /* interrupt when data has been copied to fifo */
    uint32_t rsvd6:7;
    uint32_t llc:1;         /* is a LLC packet */
    uint32_t rsvd5:2;
    uint32_t ipcs:1;        /* Enable IPv4 checksum offload */
    uint32_t udpcs:1;       /* Enable IPv4 UDP checksum offload */
    uint32_t tcpcs:1;       /* Enable IPv4 TCP checksum offload */
    uint32_t vlan:1;        /* Enable VLAN tag insertion */
    uint32_t vlantag:16;    /* VLAN tag */
#else
    uint32_t vlantag:16;
    uint32_t vlan:1;
    uint32_t tcpcs:1;
    uint32_t udpcs:1;
    uint32_t ipcs:1;
    uint32_t rsvd5:2;
    uint32_t llc:1;
    uint32_t rsvd6:7;
    uint32_t tx2fic:1;
    uint32_t txic:1;
#endif  /* #ifdef HOST_WORDS_BIGENDIAN */

    /* TXDES2 */
    void    *skb;

    /* TXDES3 */
    uint32_t buf;
} __attribute__ ((aligned (16))) Ftgmac100TXD;

#endif    /* #ifndef HW_ARM_FTGMAC100_H */
