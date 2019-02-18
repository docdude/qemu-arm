/*
 * QEMU model of the FTSDC010 MMC/SD Host Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTSDC010_H
#define HW_ARM_FTSDC010_H

/* sd controller register */
#define REG_CMD                0x0000
#define REG_ARG                0x0004
#define REG_RSP0               0x0008    /* response */
#define REG_RSP1               0x000C
#define REG_RSP2               0x0010
#define REG_RSP3               0x0014
#define REG_RSPCMD             0x0018    /* responsed command */
#define REG_DCR                0x001C    /* data control */
#define REG_DTR                0x0020    /* data timeout */
#define REG_DLR                0x0024    /* data length */
#define REG_SR                 0x0028    /* status register */
#define REG_SCR                0x002C    /* status clear register */
#define REG_IER                0x0030    /* interrupt mask/enable register */
#define REG_PWR                0x0034    /* power control */
#define REG_CLK                0x0038    /* clock control */
#define REG_BUS                0x003C    /* bus width */
#define REG_DR                 0x0040    /* data register */
#define REG_GPOR               0x0048    /* general purpose output register */
#define REG_FEAR               0x009C    /* feature register */
#define REG_REVR               0x00A0    /* revision register */

/* bit mapping of command register */
#define CMD_IDX                0x0000003F   /* command code */
#define CMD_WAIT_RSP           0x00000040   /* 32-bit response */
#define CMD_LONG_RSP           0x00000080   /* 128-bit response */
#define CMD_APP                0x00000100   /* command is a ACMD */
#define CMD_EN                 0x00000200   /* command enabled */
#define CMD_RST                0x00000400   /* software reset */

/* bit mapping of response command register */
#define RSP_CMDIDX             0x0000003F
#define RSP_CMDAPP             0x00000040

/* bit mapping of data control register */
#define DCR_BKSZ               0x0000000F
#define DCR_WR                 0x00000010
#define DCR_RD                 0x00000000
#define DCR_DMA                0x00000020
#define DCR_EN                 0x00000040
#define DCR_THRES              0x00000080
#define DCR_BURST1             0x00000000
#define DCR_BURST4             0x00000100
#define DCR_BURST8             0x00000200
#define DCR_FIFO_RESET         0x00000400

/* bit mapping of status register */
#define SR_RSP_CRC            0x00000001
#define SR_DAT_CRC            0x00000002
#define SR_RSP_TIMEOUT        0x00000004
#define SR_DAT_TIMEOUT        0x00000008
#define SR_RSP_ERR            (SR_RSP_CRC | SR_RSP_TIMEOUT)
#define SR_DAT_ERR            (SR_DAT_CRC | SR_DAT_TIMEOUT)
#define SR_RSP                0x00000010
#define SR_DAT                0x00000020
#define SR_CMD                0x00000040
#define SR_DAT_END            0x00000080
#define SR_TXRDY              0x00000100
#define SR_RXRDY              0x00000200
#define SR_CARD_CHANGE        0x00000400
#define SR_CARD_REMOVED       0x00000800
#define SR_WPROT              0x00001000
#define SR_SDIO               0x00010000
#define SR_DAT0               0x00020000

/* bit mapping of clock register */
#define CLK_HISPD             0x00000200
#define CLK_OFF               0x00000100
#define CLK_SD                0x00000080

/* bit mapping of bus register */
#define BUS_CARD_DATA3        0x00000020
#define BUS_4BITS_SUPP        0x00000008
#define BUS_8BITS_SUPP        0x00000010

#endif
