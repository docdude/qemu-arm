/*
 * Faraday FTTSC010 touchscreen driver
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTTSC010_H
#define HW_ARM_FTTSC010_H

#include "qemu/bitops.h"

#define REG_CR      0x00  /* Control Register */
#define REG_ISR     0x04  /* Interrupt Status Register */
#define REG_IMR     0x08  /* Interrupt Mask Register */
#define REG_REVR    0x0C  /* Revision Register */
#define REG_CSR     0x30  /* Clock & Sample Rate Register */
#define REG_PFR     0x34  /* Panel Function Register */
#define REG_DCR     0x38  /* Delay Control Register */
#define REG_XYR     0x3C  /* Touchscreen X,Y-Axis Register */
#define REG_ZR      0x4C  /* Touchscreen Z-Axis (Pressure) Register */

#define CR_AS       BIT(31) /* Auto-scan enable */
#define CR_RD1      BIT(30) /* Single read enabled */
#define CR_CHDLY(x) BIT(16 + (x % 7)) /* ADC channel x delay enabled */
#define CR_APWRDN   BIT(9)  /* ADC auto power down mode */
#define CR_PWRDN    BIT(8)  /* ADC and IPGA power down */
#define CR_ICS(x)   ((x) & 0xf) /* ADC analog input channel select */

#define CSR_SDIV(x) (((x) & 0xff) << 8) /* ADC sample clock divider */
#define CSR_MDIV(x) (((x) & 0xff) << 0) /* ADC main clock divider */

#define DCR_DLY(x)  ((x) & 0xffff)      /* ADC conversion delay */

#define ISR_AS      BIT(10) /* Auto-scan cpmplete */

#endif
