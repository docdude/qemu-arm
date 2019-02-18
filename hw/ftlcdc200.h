/*
 * Faraday FTLCDC200 Color LCD Controller
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under the GNU LGPL
 */

#ifndef HW_ARM_FTLCDC2XX_H
#define HW_ARM_FTLCDC2XX_H

/* HW Registers */

#define REG_FER         0x00000 /* LCD Function Enable Register */
#define REG_PPR         0x00004 /* LCD Panel Pixel Register */
#define REG_IER         0x00008 /* LCD Interrupt Enable Register */
#define REG_ISCR        0x0000C /* LCD Interrupt Status Clear Register */
#define REG_ISR         0x00010 /* LCD Interrupt Status Register */
#define REG_FB0         0x00018 /* LCD Framebuffer Base Register 0 */
#define REG_FB1         0x00024 /* LCD Framebuffer Base Register 1 */
#define REG_FB2         0x00030 /* LCD Framebuffer Base Register 2 */
#define REG_FB3         0x0003C /* LCD Framebuffer Base Register 3 */

#define REG_HT          0x00100 /* LCD Horizontal Timing Control Register */
#define REG_VT0         0x00104 /* LCD Vertical Timing Control Register 0 */
#define REG_VT1         0x00108 /* LCD Vertical Timing Control Register 1 */
#define REG_POL         0x0010C /* LCD Polarity Control Register */

#define REG_SPPR        0x00200 /* LCD Serial Panel Pixel Register */
#define REG_CCIR        0x00204 /* LCD CCIR565 Register */

/* LCD Function Enable Register */
#define FER_EN          (1 << 0)    /* chip enabled */
#define FER_ON          (1 << 1)    /* screen on */
#define FER_YUV420      (3 << 2)
#define FER_YUV422      (2 << 2)
#define FER_YUV         (1 << 3)    /* 1:YUV, 0:RGB */

/* LCD Panel Pixel Register */
#define PPR_BPP_1       (0 << 0)
#define PPR_BPP_2       (1 << 0)
#define PPR_BPP_4       (2 << 0)
#define PPR_BPP_8       (3 << 0)
#define PPR_BPP_16      (4 << 0)
#define PPR_BPP_24      (5 << 0)
#define PPR_BPP_MASK    (7 << 0)
#define PPR_PWROFF      (1 << 3)
#define PPR_BGR         (1 << 4)
#define PPR_ENDIAN_LBLP (0 << 5)
#define PPR_ENDIAN_BBBP (1 << 5)
#define PPR_ENDIAN_LBBP (2 << 5)
#define PPR_ENDIAN_MASK (3 << 5)
#define PPR_RGB1        (PPR_BPP_1)
#define PPR_RGB2        (PPR_BPP_2)
#define PPR_RGB4        (PPR_BPP_4)
#define PPR_RGB8        (PPR_BPP_8)
#define PPR_RGB12       (PPR_BPP_16 | (2 << 7))
#define PPR_RGB16_555   (PPR_BPP_16 | (1 << 7))
#define PPR_RGB16_565   (PPR_BPP_16 | (0 << 7))
#define PPR_RGB24       (PPR_BPP_24)
#define PPR_RGB32       (PPR_BPP_24)
#define PPR_RGB_MASK    (PPR_BPP_MASK | (3 << 7))
#define PPR_VCOMP_VSYNC (0 << 9)
#define PPR_VCOMP_VBP   (1 << 9)
#define PPR_VCOMP_VAIMG (2 << 9)
#define PPR_VCOMP_VFP   (3 << 9)
#define PPR_VCOMP_MASK  (3 << 9)

/* LCD Interrupt Enable Register */
#define IER_FIFOUR      (1 << 0)
#define IER_NEXTFB      (1 << 1)
#define IER_VCOMP       (1 << 2)
#define IER_BUSERR      (1 << 3)

/* LCD Interrupt Status Register */
#define ISR_FIFOUR      (1 << 0)
#define ISR_NEXTFB      (1 << 1)
#define ISR_VCOMP       (1 << 2)
#define ISR_BUSERR      (1 << 3)

/* LCD Horizontal Timing Control Register */
#define HT_HBP(x)       ((((x) - 1) & 0xff) << 24)
#define HT_HFP(x)       ((((x) - 1) & 0xff) << 16)
#define HT_HSYNC(x)     ((((x) - 1) & 0xff) << 8)
#define HT_PL(x)        (((x >> 4) - 1) & 0xff)

/* LCD Vertical Timing Control Register 0 */
#define VT0_VFP(x)      (((x) & 0xff) << 24)
#define VT0_VSYNC(x)    ((((x) - 1) & 0x3f) << 16)
#define VT0_LF(x)       (((x) - 1) & 0xfff)

/* LCD Polarity Control Register */
#define POL_IVS         (1 << 0)
#define POL_IHS         (1 << 1)
#define POL_ICK         (1 << 2)
#define POL_IDE         (1 << 3)
#define POL_IPWR        (1 << 4)
#define POL_DIV(x)      ((((x) - 1) & 0x7f) << 8)

/* LCD Serial Panel Pixel Register */
#define SPPR_SERIAL     (1 << 0)
#define SPPR_DELTA      (1 << 1)
#define SPPR_CS_RGB     (0 << 2)
#define SPPR_CS_BRG     (1 << 2)
#define SPPR_CS_GBR     (2 << 2)
#define SPPR_LSR        (1 << 4)
#define SPPR_AUO052     (1 << 5)

#endif /* HW_ARM_FTLCDC2XX_H */
