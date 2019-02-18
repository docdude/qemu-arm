/*
 * Faraday FTPWMTMR010 Timer.
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTPWMTMR010_H
#define HW_ARM_FTPWMTMR010_H

#include "qemu/bitops.h"

#define REG_SR              0x00    /* status register */
#define REG_REVR            0x90    /* revision register */

#define REG_TIMER_BASE(id)  (0x00 + ((id) << 4))
#define REG_TIMER_CTRL      0x00
#define REG_TIMER_CNTB      0x04
#define REG_TIMER_CMPB      0x08
#define REG_TIMER_CNTO      0x0C

#define TIMER_CTRL_EXTCK        BIT(0)  /* external clock */
#define TIMER_CTRL_START        BIT(1)
#define TIMER_CTRL_UPDATE       BIT(2)
#define TIMER_CTRL_AUTORELOAD   BIT(4)
#define TIMER_CTRL_INTR         BIT(5)  /* interrupt enabled */
#define TIMER_CTRL_INTR_EDGE    BIT(6)  /* interrupt type: 1=edge 0=level */

#endif
