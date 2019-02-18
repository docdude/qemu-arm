/*
 * QEMU model of the FTWDT010 WatchDog Timer
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTWDT010_H
#define HW_ARM_FTWDT010_H

#include "qemu/bitops.h"

/* Hardware registers */
#define REG_COUNTER     0x00    /* counter register */
#define REG_LOAD        0x04    /* (re)load register */
#define REG_RESTART     0x08    /* restart register */
#define REG_CR          0x0C    /* control register */
#define REG_SR          0x10    /* status register */
#define REG_SCR         0x14    /* status clear register */
#define REG_INTR_LEN    0x18    /* interrupt length register */
#define REG_REVR        0x1C    /* revision register */

#define CR_CLKS         BIT(4)  /* clock source */
#define CR_ESIG         BIT(3)  /* external signal enabled */
#define CR_INTR         BIT(2)  /* system reset interrupt enabled */
#define CR_SRST         BIT(1)  /* system reset enabled */
#define CR_EN           BIT(0)  /* chip enabled */

#define SR_SRST         BIT(1)  /* system reset */

#define WDT_MAGIC       0x5ab9  /* magic for watchdog restart */

#endif
