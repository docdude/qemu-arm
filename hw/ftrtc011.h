/*
 * QEMU model of the FTRTC011 RTC Timer
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */
#ifndef HW_ARM_FTRTC011_H
#define HW_ARM_FTRTC011_H

#include "qemu/bitops.h"

/* Hardware registers */
#define REG_SEC         0x00
#define REG_MIN         0x04
#define REG_HOUR        0x08
#define REG_DAY         0x0C

#define REG_ALARM_SEC   0x10    /* Alarm: sec */
#define REG_ALARM_MIN   0x14    /* Alarm: min */
#define REG_ALARM_HOUR  0x18    /* Alarm: hour */

#define REG_CR          0x20    /* Control Register */
#define REG_WSEC        0x24    /* Write SEC Register */
#define REG_WMIN        0x28    /* Write MIN Register */
#define REG_WHOUR       0x2C    /* Write HOUR Register */
#define REG_WDAY        0x30    /* Write DAY Register */
#define REG_ISR         0x34    /* Interrupt Status Register */

#define REG_REVR        0x3C    /* Revision Register */
#define REG_CURRENT     0x44    /* Group-up day/hour/min/sec as a register */

#define CR_LOAD         BIT(6)  /* Update counters by Wxxx registers */
#define CR_INTR_ALARM   BIT(5)  /* Alarm interrupt enabled */
#define CR_INTR_DAY     BIT(4)  /* DDAY interrupt enabled */
#define CR_INTR_HOUR    BIT(3)  /* HOUR interrupt enabled */
#define CR_INTR_MIN     BIT(2)  /* MIN interrupt enabled */
#define CR_INTR_SEC     BIT(1)  /* SEC interrupt enabled */
#define CR_INTR_MASK    (CR_INTR_SEC | CR_INTR_MIN \
                        | CR_INTR_HOUR | CR_INTR_DAY | CR_INTR_ALARM)
#define CR_EN           BIT(0)  /* RTC enabled */

#define ISR_LOAD        BIT(5)  /* CR_LOAD finished (no interrupt occurs) */
#define ISR_ALARM       BIT(4)
#define ISR_DAY         BIT(3)
#define ISR_HOUR        BIT(2)
#define ISR_MIN         BIT(1)
#define ISR_SEC         BIT(0)
#define ISR_MASK        (ISR_SEC | ISR_MIN \
                        | ISR_HOUR | ISR_DAY | ISR_ALARM | ISR_LOAD)

#endif
