/*
 * QEMU model of the FTDMAC020 DMA Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 *
 * Note: The FTDMAC020 decreasing address mode is not implemented.
 */

#ifndef HW_ARM_FTDMAC020_H
#define HW_ARM_FTDMAC020_H

#include "qemu/bitops.h"

#define REG_ISR         0x00    /* Interrupt Status Register */
#define REG_TCISR       0x04    /* Terminal Count Interrupt Status Register */
#define REG_TCCLR       0x08    /* Terminal Count Status Clear Register */
#define REG_EAISR       0x0c    /* Error/Abort Interrupt Status Register */
#define REG_EACLR       0x10    /* Error/Abort Status Clear Register */
#define REG_TCSR        0x14    /* Terminal Count Status Register */
#define REG_EASR        0x18    /* Error/Abort Status Register */
#define REG_CESR        0x1c    /* Channel Enable Status Register */
#define REG_CBSR        0x20    /* Channel Busy Status Register */
#define REG_CSR         0x24    /* Configuration Status Register */
#define REG_SYNC        0x28    /* Synchronization Register */
#define REG_REVISION    0x30
#define REG_FEATURE     0x34

#define REG_CHAN_ID(addr)   (((addr) - 0x100) >> 5)
#define REG_CHAN_BASE(ch)   (0x100 + ((ch) << 5))

#define REG_CHAN_CCR        0x00
#define REG_CHAN_CFG        0x04
#define REG_CHAN_SRC        0x08
#define REG_CHAN_DST        0x0C
#define REG_CHAN_LLP        0x10
#define REG_CHAN_LEN        0x14

/*
 * Feature register
 */
#define FEATURE_NCHAN(f)    (((f) >> 12) & 0xF)
#define FEATURE_BRIDGE(f)   ((f) & BIT(10))
#define FEATURE_DUALBUS(f)  ((f) & BIT(9))
#define FEATURE_LLP(f)      ((f) & BIT(8))
#define FEATURE_FIFOAW(f)   ((f) & 0xF)

/*
 * Channel control register
 */
#define CCR_START           BIT(0)
#define CCR_DST_M1          BIT(1)  /* dst is a slave device of AHB1 */
#define CCR_SRC_M1          BIT(2)  /* src is a slave device of AHB1 */
#define CCR_DST_INC         (0 << 3)/* dst addr: next = curr++ */
#define CCR_DST_DEC         (1 << 3)/* dst addr: next = curr-- */
#define CCR_DST_FIXED       (2 << 3)
#define CCR_SRC_INC         (0 << 5)/* src addr: next = curr++ */
#define CCR_SRC_DEC         (1 << 5)/* src addr: next = curr-- */
#define CCR_SRC_FIXED       (2 << 5)
#define CCR_HANDSHAKE       BIT(7)  /* DMA HW handshake enabled */
#define CCR_DST_WIDTH_8     (0 << 8)
#define CCR_DST_WIDTH_16    (1 << 8)
#define CCR_DST_WIDTH_32    (2 << 8)
#define CCR_DST_WIDTH_64    (3 << 8)
#define CCR_SRC_WIDTH_8     (0 << 11)
#define CCR_SRC_WIDTH_16    (1 << 11)
#define CCR_SRC_WIDTH_32    (2 << 11)
#define CCR_SRC_WIDTH_64    (3 << 11)
#define CCR_ABORT           BIT(15)
#define CCR_BURST_1         (0 << 16)
#define CCR_BURST_4         (1 << 16)
#define CCR_BURST_8         (2 << 16)
#define CCR_BURST_16        (3 << 16)
#define CCR_BURST_32        (4 << 16)
#define CCR_BURST_64        (5 << 16)
#define CCR_BURST_128       (6 << 16)
#define CCR_BURST_256       (7 << 16)
#define CCR_PRIVILEGED      BIT(19)
#define CCR_BUFFERABLE      BIT(20)
#define CCR_CACHEABLE       BIT(21)
#define CCR_PRIO_0          (0 << 22)
#define CCR_PRIO_1          (1 << 22)
#define CCR_PRIO_2          (2 << 22)
#define CCR_PRIO_3          (3 << 22)
#define CCR_FIFOTH_1        (0 << 24)
#define CCR_FIFOTH_2        (1 << 24)
#define CCR_FIFOTH_4        (2 << 24)
#define CCR_FIFOTH_8        (3 << 24)
#define CCR_FIFOTH_16       (4 << 24)
#define CCR_MASK_TC         BIT(31) /* DMA Transfer terminated(finished) */

/*
 * Channel configuration register
 */
#define CFG_MASK_TCI            BIT(0)    /* disable tc interrupt */
#define CFG_MASK_EI             BIT(1)    /* disable error interrupt */
#define CFG_MASK_AI             BIT(2)    /* disable abort interrupt */
#define CFG_SRC_HANDSHAKE(x)    (((x) & 0xf) << 3)
#define CFG_SRC_HANDSHAKE_EN    BIT(7)
#define CFG_BUSY                BIT(8)
#define CFG_DST_HANDSHAKE(x)    (((x) & 0xf) << 9)
#define CFG_DST_HANDSHAKE_EN    BIT(13)
#define CFG_LLP_CNT(cfg)        (((cfg) >> 16) & 0xf)

#endif    /* HW_ARM_FTDMAC020_H */
