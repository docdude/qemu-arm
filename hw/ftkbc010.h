/*
 * Faraday FTKBC010 Keyboard/Keypad Controller
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+
 */
#ifndef HW_ARM_FTKBC010_H
#define HW_ARM_FTKBC010_H

#include "qemu/bitops.h"

#define REG_CR      0x00    /* control register */
#define REG_SRDR    0x04    /* sample rate division register */
#define REG_RSCR    0x08    /* request to send counter register */
#define REG_SR      0x0C    /* status register */
#define REG_ISR     0x10    /* interrupt status register */
#define REG_KBDRR   0x14    /* keyboard receive register */
#define REG_KBDTR   0x18    /* keyboard transmit register */
#define REG_IMR     0x1C    /* interrupt mask register */
#define REG_KPDXR   0x30    /* keypad X-Axis register */
#define REG_KPDYR   0x34    /* keypad Y-Axis register */
#define REG_ASPR    0x38    /* auto-scan period register */
#define REG_REVR    0x50    /* revision register */
#define REG_FEAR    0x54    /* feature register */

#define CR_KPDIC    BIT(10) /* Write 1 to clear Keypad interupt */
#define CR_KPDAS    BIT(9)  /* Keypad audo-scan enabled */
#define CR_KPDEN    BIT(8)  /* Keypad function enabled */
#define CR_RXICLR   BIT(7)  /* Write 1 to clear Keyboard/Mouse Rx interrupt */
#define CR_TXICLR   BIT(6)  /* Write 1 to clear Keyboard/Mouse Tx interrupt */
#define CR_NOLC     BIT(5)  /* No line control bit */
#define CR_RXIEN    BIT(4)  /* Keyboard/Mouse Rx interrupt enabled */
#define CR_TXIEN    BIT(3)  /* Keyboard/Mouse Tx interrupt enabled */
#define CR_EN       BIT(2)  /* Chip enabled */
#define CR_DATDN    BIT(1)  /* Data disabled */
#define CR_CLKDN    BIT(0)  /* Clock disabled */

#define ISR_KPDI    BIT(2)  /* Keypad interupt */
#define ISR_TXI     BIT(1)  /* Keyboard/Mouse Tx interrupt enabled */
#define ISR_RXI     BIT(0)  /* Keyboard/Mouse Rx interrupt enabled */

#endif
