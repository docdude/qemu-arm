/*
 * QEMU model of the FTSSP010 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTSSP010_H
#define HW_ARM_FTSSP010_H

/* FTSSP010: Registers */
#define REG_CR0             0x00    /* control register 0 */
#define REG_CR1             0x04    /* control register 1 */
#define ReG_CR2             0x08    /* control register 2 */
#define REG_SR              0x0C    /* status register */
#define REG_ICR             0X10    /* interrupt control register */
#define REG_ISR             0x14    /* interrupt statis register */
#define REG_DR              0x18    /* data register */
#define REG_REVR            0x60    /* revision register */
#define REG_FEAR            0x64    /* feature register */

/* Control register 0  */

#define CR0_FFMT_MASK       (7 << 12)
#define CR0_FFMT_SSP        (0 << 12)
#define CR0_FFMT_SPI        (1 << 12)
#define CR0_FFMT_MICROWIRE  (2 << 12)
#define CR0_FFMT_I2S        (3 << 12)
#define CR0_FFMT_AC97       (4 << 12)
#define CR0_FLASH           (1 << 11)
#define CR0_FSDIST(x)       (((x) & 0x03) << 8)
#define CR0_LBM             (1 << 7)  /* Loopback mode */
#define CR0_LSB             (1 << 6)  /* LSB first */
#define CR0_FSPO            (1 << 5)  /* Frame sync atcive low */
#define CR0_FSJUSTIFY       (1 << 4)
#define CR0_OPM_SLAVE       (0 << 2)
#define CR0_OPM_MASTER      (3 << 2)
#define CR0_OPM_I2S_MSST    (3 << 2)  /* Master stereo mode */
#define CR0_OPM_I2S_MSMO    (2 << 2)  /* Master mono mode */
#define CR0_OPM_I2S_SLST    (1 << 2)  /* Slave stereo mode */
#define CR0_OPM_I2S_SLMO    (0 << 2)  /* Slave mono mode */
#define CR0_SCLKPO          (1 << 1)  /* SCLK Remain HIGH */
#define CR0_SCLKPH          (1 << 0)  /* Half CLK cycle */

/* Control Register 1 */

/* padding data length */
#define CR1_PDL(x)          (((x) & 0xff) << 24)
/* serial data length(actual data length-1) */
#define CR1_SDL(x)          ((((x) - 1) & 0x1f) << 16)
/*  clk divider */
#define CR1_CLKDIV(x)       ((x) & 0xffff)

/* Control Register 2 */
#define CR2_FSOS(x)         (((x) & 0x03) << 10)        /* FS/CS Select */
#define CR2_FS              (1 << 9)    /* FS/CS Signal Level */
#define CR2_TXEN            (1 << 8)    /* Tx Enable */
#define CR2_RXEN            (1 << 7)    /* Rx Enable */
#define CR2_SSPRST          (1 << 6)    /* SSP reset */
#define CR2_TXFCLR          (1 << 3)    /* TX FIFO Clear */
#define CR2_RXFCLR          (1 << 2)    /* RX FIFO Clear */
#define CR2_TXDOE           (1 << 1)    /* TX Data Output Enable */
#define CR2_SSPEN           (1 << 0)    /* SSP Enable */

/*
 * Status Register
 */
#define SR_TFVE(reg)        (((reg) & 0x1F) << 12)
#define SR_RFVE(reg)        (((reg) & 0x1F) << 4)
#define SR_BUSY             (1 << 2)
#define SR_TFNF             (1 << 1)    /* Tx FIFO Not Full */
#define SR_RFF              (1 << 0)    /* Rx FIFO Full */

/* Interrupr Control Register */
#define ICR_TFTHOD(x)       (((x) & 0x1f) << 12)/* TX FIFO Threshold */
#define ICR_RFTHOD(x)       (((x) & 0x1f) << 7) /* RX FIFO Threshold */
#define ICR_AC97I           0x40      /* AC97 intr enable */
#define ICR_TFDMA           0x20      /* TX DMA enable */
#define ICR_RFDMA           0x10      /* RX DMA enable */
#define ICR_TFTHI           0x08      /* TX FIFO intr enable */
#define ICR_RFTHI           0x04      /* RX FIFO intr enable */
#define ICR_TFURI           0x02      /* TX FIFO Underrun intr enable */
#define ICR_RFORI           0x01      /* RX FIFO Overrun intr enable */

/* Interrupr Status Register */
#define ISR_AC97I           0x10      /* AC97 interrupt */
#define ISR_TFTHI           0x08      /* TX FIFO interrupt */
#define ISR_RFTHI           0x04      /* RX FIFO interrupt */
#define ISR_TFURI           0x02      /* TX FIFO Underrun interrupt */
#define ISR_RFORI           0x01      /* RX FIFO Overrun interrupt */
/* Read-Clear status mask */
#define ISR_RCMASK          (ISR_RFORI | ISR_TFURI | ISR_AC97I)

#endif
