/*
 * Faraday FTINTC020 Programmable Interrupt Controller.
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under GNU GPL v2+.
 */

#ifndef HW_ARM_FTINTC020_H
#define HW_ARM_FTINTC020_H

/* IRQ/FIO: 0 ~ 31 */
#define REG_IRQ32SRC    0x00    /* IRQ source register */
#define REG_IRQ32ENA    0x04    /* IRQ enable register */
#define REG_IRQ32SCR    0x08    /* IRQ status clear register */
#define REG_IRQ32MDR    0x0C    /* IRQ trigger mode register */
#define REG_IRQ32LVR    0x10    /* IRQ trigger level register */
#define REG_IRQ32SR     0x14    /* IRQ status register */

#define REG_FIQ32SRC    0x20    /* FIQ source register */
#define REG_FIQ32ENA    0x24    /* FIQ enable register */
#define REG_FIQ32SCR    0x28    /* FIQ status clear register */
#define REG_FIQ32MDR    0x2C    /* FIQ trigger mode register */
#define REG_FIQ32LVR    0x30    /* FIQ trigger level register */
#define REG_FIQ32SR     0x34    /* FIQ status register */

/* Extended IRQ/FIO: 32 ~ 63 */
#define REG_IRQ64SRC    0x60    /* Extended IRQ source register */
#define REG_IRQ64ENA    0x64    /* Extended IRQ enable register */
#define REG_IRQ64SCR    0x68    /* Extended IRQ status clear register */
#define REG_IRQ64MDR    0x6C    /* Extended IRQ trigger mode register */
#define REG_IRQ64LVR    0x70    /* Extended IRQ trigger level register */
#define REG_IRQ64SR     0x74    /* Extended IRQ status register */

#define REG_FIQ64SRC    0x80    /* Extended FIQ source register */
#define REG_FIQ64ENA    0x84    /* Extended FIQ enable register */
#define REG_FIQ64SCR    0x88    /* Extended FIQ status clear register */
#define REG_FIQ64MDR    0x8C    /* Extended FIQ trigger mode register */
#define REG_FIQ64LVR    0x90    /* Extended FIQ trigger level register */
#define REG_FIQ64SR     0x94    /* Extended FIQ status register */

/* Common register offset to IRQ/FIQ */
#define REG_SRC         0x00    /* Source register */
#define REG_ENA         0x04    /* Enable register */
#define REG_SCR         0x08    /* Status clear register */
#define REG_MDR         0x0C    /* Trigger mode register */
#define REG_LVR         0x10    /* Trigger level register */
#define REG_SR          0x14    /* Status register */
#define REG_MASK        0x1f

#define REG_IRQ(n)      (((n) < 32) ? REG_IRQ32SRC : REG_IRQ64SRC)
#define REG_FIQ(n)      (((n) < 32) ? REG_FIQ32SRC : REG_FIQ64SRC)

#endif
