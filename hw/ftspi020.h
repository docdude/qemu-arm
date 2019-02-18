/*
 * Faraday FTSPI020 Flash Controller
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTSPI020_H
#define HW_ARM_FTSPI020_H

#include "qemu/bitops.h"

/* Number of CS lines */
#define CFG_NR_CSLINES      4

/******************************************************************************
 * FTSPI020 registers
 *****************************************************************************/
#define REG_CMD0            0x00    /* Flash address */
#define REG_CMD1            0x04
#define REG_CMD2            0x08    /* Flash data counter */
#define REG_CMD3            0x0c
#define REG_CR              0x10    /* Control Register */
#define REG_TR              0x14    /* AC Timing Register */
#define REG_SR              0x18    /* Status Register */
#define REG_ICR             0x20    /* Interrupt Control Register */
#define REG_ISR             0x24    /* Interrupt Status Register */
#define REG_RDSR            0x28    /* Read Status Register */
#define REG_REVR            0x50    /* Revision Register */
#define REG_FEAR            0x54    /* Feature Register */
#define REG_DR              0x100   /* Data Register */

#define CMD1_CTRD           BIT(28) /* Enable 1 byte continuous read */
#define CMD1_INST_LEN(x)    (((x) & 0x03) << 24)/* instruction length */
#define CMD1_DCLK_LEN(x)    (((x) & 0xff) << 16)/* dummy clock length */
#define CMD1_ADDR_LEN(x)    (((x) & 0x07) << 0) /* address length */

#define CMD3_INST_OPC(x)    (((x) & 0xff) << 24)/* instruction op code */
#define CMD3_CTRD_OPC(x)    (((x) & 0xff) << 16)/* cont. read op code */
#define CMD3_CS(x)          (((x) & 0x0f) << 8) /* chip select */
#define CMD3_OPM_STD        (0)         /* standard 1-bit serial mode */
#define CMD3_OPM_DUAL       (1 << 5)    /* fast read dual */
#define CMD3_OPM_QUAD       (2 << 5)    /* fast read quad */
#define CMD3_OPM_DUALIO     (3 << 5)    /* fast read dual io */
#define CMD3_OPM_QUADIO     (4 << 5)    /* fast read quad io */
#define CMD3_DTR            BIT(4)  /* Enable double transfer rate */
#define CMD3_RDSR_HW        (0)     /* Enable HW polling RDSR */
#define CMD3_RDSR_SW        BIT(3)  /* Disable HW polling RDSR */
#define CMD3_RDSR           BIT(2)  /* Indicate it's a RDSR command */
#define CMD3_READ           0       /* Indicate it's a read command */
#define CMD3_WRITE          BIT(1)  /* Indicate it's a write command */
#define CMD3_INTR           BIT(0)  /* Enable interrupt and status update */

#define CR_BUSYBIT(x)       (((x) & 0x07) << 16) /* Busy bit in the RDSR */
#define CR_ABORT            BIT(8)
#define CR_MODE0            0       /* SPI MODE0 */
#define CR_MODE3            BIT(4)  /* SPI MODE3 */
#define CR_CLKDIV(n)        ((n) & 0x03)    /* Clock divider = 2 * (n + 1) */

#define TR_TRACE(x)         (((x) & 0x0f) << 4) /* trace delay */
#define TR_CS(x)            (((x) & 0x0f) << 0) /* cs delay */

#define SR_RX_READY         BIT(1)  /* Rx Ready */
#define SR_TX_READY         BIT(0)  /* Tx Ready */

#define ICR_RX_THRES(x)     (((x) & 0x03) << 12)/* rx interrupt threshold */
#define ICR_TX_THRES(x)     (((x) & 0x03) << 8) /* tx interrupt threshold */
#define ICR_DMA             BIT(0)  /* Enable DMA HW handshake */

#define ISR_CMDFIN          BIT(0)  /* Command finished interrupt */

#define FEAR_CLKM_BYPORT    0       /* clock mode = byport */
#define FEAR_CLKM_SYNC      BIT(25) /* clock mode = sync */
#define FEAR_SUPP_DTR       BIT(24) /* support double transfer rate */
#define FEAR_CMDQ(x)        (((x) & 0x07) << 16)    /* cmd queue depth */
#define FEAR_RXFIFO(x)      (((x) & 0xff) << 8)     /* rx fifo depth */
#define FEAR_TXFIFO(x)      (((x) & 0xff) << 0)     /* tx fifo depth */

#endif  /* HW_ARM_FTSPI020_H */
