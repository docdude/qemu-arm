/*
 * Faraday FTTMR010 Timer.
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTTMR010_H
#define HW_ARM_FTTMR010_H

#define REG_TMR_ID(off)     ((off) >> 4)
#define REG_TMR_BASE(id)    (0x00 + ((id) << 4))
#define REG_TMR_COUNTER     0x00
#define REG_TMR_RELOAD      0x04
#define REG_TMR_MATCH1      0x08
#define REG_TMR_MATCH2      0x0C

#define REG_CR              0x30    /* control register */
#define REG_ISR             0x34    /* interrupt status register */
#define REG_IMR             0x38    /* interrupt mask register */
#define REG_REVR            0x3C    /* revision register */

/* timer enable */
#define CR_TMR_EN(id)       (0x01 << ((id) * 3))
/* timer overflow interrupt enable */
#define CR_TMR_OFEN(id)     (0x04 << ((id) * 3))
/* timer count-up mode */
#define CR_TMR_COUNTUP(id)  (0x01 << (9 + (id)))

/* timer match 1 */
#define ISR_MATCH1(id)      (0x01 << ((id) * 3))
/* timer match 2 */
#define ISR_MATCH2(id)      (0x02 << ((id) * 3))
/* timer overflow */
#define ISR_OF(id)          (0x04 << ((id) * 3))

#endif
