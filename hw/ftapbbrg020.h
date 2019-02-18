/*
 * QEMU model of the FTAPBBRG020 DMA Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 *
 * Note: The FTAPBBRG020 DMA decreasing address mode is not implemented.
 */

#ifndef HW_ARM_FTAPBBRG020_H
#define HW_ARM_FTAPBBRG020_H

#include "qemu/bitops.h"

#define REG_SLAVE(n)        ((n) * 4) /* Slave config (base & size) */
#define REG_REVISION        0xC8

/*
 * Channel base address
 * @ch: channle id (0 <= id <= 3)
 *      i.e. 0: Channel A
 *           1: Channel B
 *           2: Channel C
 *           3: Channel D
 */
#define REG_CHAN_ID(off)    (((off) - 0x80) >> 4)
#define REG_CHAN_BASE(ch)   (0x80 + ((ch) << 4))

#define REG_CHAN_SRC        0x00
#define REG_CHAN_DST        0x04
#define REG_CHAN_CYC        0x08
#define REG_CHAN_CMD        0x0C

#define CMD_START           BIT(0)
#define CMD_INTR_FIN        BIT(1)
#define CMD_INTR_FIN_EN     BIT(2)
#define CMD_BURST4          BIT(3)
#define CMD_INTR_ERR        BIT(4)
#define CMD_INTR_ERR_EN     BIT(5)
#define CMD_INTR_STATUS     (CMD_INTR_FIN | CMD_INTR_ERR)

#endif    /* HW_ARM_FTAPBB020_H */
