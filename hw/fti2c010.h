/*
 * QEMU model of the FTI2C010 Controller
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTI2C010_H
#define HW_ARM_FTI2C010_H

#include "qemu/bitops.h"

/*
 * FTI2C010 registers
 */
#define REG_CR              0x00    /* control register */
#define REG_SR              0x04    /* status register */
#define REG_CDR             0x08    /* clock division register */
#define REG_DR              0x0C    /* data register */
#define REG_SAR             0x10    /* slave address register */
#define REG_TGSR            0x14    /* time & glitch suppression register */
#define REG_BMR             0x18    /* bus monitor register */
#define REG_REVR            0x30    /* revision register */

/*
 * REG_CR
 */
#define CR_STARTIEN         0x4000  /* start condition (slave only) */
#define CR_ALIEN            0x2000  /* Arbitration lose (master only) */
#define CR_SAMIEN           0x1000  /* slave address match (slave only) */
#define CR_STOPIEN          0x800   /* stop condition (slave only) */
#define CR_BERRIEN          0x400   /* NACK response (master only) */
#define CR_DRIEN            0x200   /* data receive */
#define CR_DTIEN            0x100   /* data transmit */
#define CR_TBEN             0x80    /* transfer byte enable */
#define CR_NACK             0x40    /* acknowledge mode: 0=ACK, 1=NACK */
#define CR_STOP             0x20    /* stop (master only) */
#define CR_START            0x10    /* start (master only) */
#define CR_GCEN             0x8     /* general call message (slave only) */
#define CR_SCLEN            0x4     /* enable clock (master only) */
#define CR_I2CEN            0x2     /* enable I2C */
#define CR_I2CRST           0x1     /* reset I2C */

#define CR_SLAVE_MASK       (CR_GCEN | CR_SAMIEN | CR_STOPIEN | CR_STARTIEN)

#define CR_MASTER_INTR      (CR_ALIEN | CR_BERRIEN | CR_DRIEN | CR_DTIEN)
#define CR_MASTER_EN        (CR_SCLEN | CR_I2CEN)
#define CR_MASTER_MODE      (CR_MASTER_INTR | CR_MASTER_EN)

/*
 * REG_SR
 */
#define SR_START            0x800
#define SR_AL               0x400
#define SR_GC               0x200
#define SR_SAM              0x100
#define SR_STOP             0x80
#define SR_BERR             0x40
#define SR_DR               0x20    /* received one new data byte */
#define SR_DT               0x10    /* trandmitted one data byte */
#define SR_BB               0x8     /* set when i2c bus is busy */
#define SR_I2CB             0x4     /* set when fti2c010 is busy */
#define SR_ACK              0x2
#define SR_RW               0x1

/*
 * REG_TGSR
 */
#define TGSR_TSR(x)         ((x) & 0x3f)            /* setup/hold time */
#define TGSR_GSR(x)         (((x) & 0x07) << 10)    /* glitch suppression */

#endif
