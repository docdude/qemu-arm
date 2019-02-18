/*
 * Faraday FTLCDC200 Color LCD Controller
 *
 * base is pl110.c
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This code is licensed under the GNU LGPL
 */

#include "hw/sysbus.h"
#include "hw/display/framebuffer.h"
#include "ui/console.h"
#include "ui/pixel_ops.h"

#include "qemu/bitops.h"
#include "hw/ftlcdc200.h"

enum ftlcdc200_irqpin {
    IRQ_ALL = 0,
    IRQ_VSTATUS,
    IRQ_BASEUPT,
    IRQ_FIFOUR,
    IRQ_BUSERR,
};

enum ftlcdc200_bppmode {
    BPP_1 = 0,
    BPP_2,
    BPP_4,
    BPP_8,
    BPP_16,
    BPP_32,
    BPP_16_565,
    BPP_12,
};

#define TYPE_FTLCDC200  "ftlcdc200"

typedef struct Ftlcdc200State {
    SysBusDevice busdev;
    MemoryRegion iomem;
    QemuConsole *con;

    qemu_irq irq[5];
    int cols;
    int rows;
    enum ftlcdc200_bppmode bpp;
    int invalidate;
    uint32_t palette[256];
    uint32_t raw_palette[128];

    /* hw register caches */
    uint32_t fer;   /* function enable register */
    uint32_t ppr;   /* panel pixel register */
    uint32_t ier;   /* interrupt enable register */
    uint32_t isr;   /* interrupt status register */
    uint32_t sppr;  /* serail panel pixel register */

    uint32_t fb[4]; /* frame buffer base address register */
    uint32_t ht;    /* horizontal timing control register */
    uint32_t vt0;   /* vertital timing control register 0 */
    uint32_t vt1;   /* vertital timing control register 1 */
    uint32_t pol;   /* polarity */

} Ftlcdc200State;

#define FTLCDC200(obj) \
    OBJECT_CHECK(Ftlcdc200State, obj, TYPE_FTLCDC200)

static const VMStateDescription vmstate_ftlcdc200 = {
    .name = TYPE_FTLCDC200,
    .version_id = 2,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_INT32(cols, Ftlcdc200State),
        VMSTATE_INT32(rows, Ftlcdc200State),
        VMSTATE_UINT32(bpp, Ftlcdc200State),
        VMSTATE_INT32(invalidate, Ftlcdc200State),
        VMSTATE_UINT32_ARRAY(palette, Ftlcdc200State, 256),
        VMSTATE_UINT32_ARRAY(raw_palette, Ftlcdc200State, 128),
        VMSTATE_UINT32(fer, Ftlcdc200State),
        VMSTATE_UINT32(ppr, Ftlcdc200State),
        VMSTATE_UINT32(ier, Ftlcdc200State),
        VMSTATE_UINT32(isr, Ftlcdc200State),
        VMSTATE_UINT32(sppr, Ftlcdc200State),
        VMSTATE_UINT32_ARRAY(fb, Ftlcdc200State, 4),
        VMSTATE_UINT32(ht, Ftlcdc200State),
        VMSTATE_UINT32(vt0, Ftlcdc200State),
        VMSTATE_UINT32(vt1, Ftlcdc200State),
        VMSTATE_UINT32(pol, Ftlcdc200State),
        VMSTATE_END_OF_LIST()
    }
};

#define BITS 8
#include "hw/ftlcdc200_template.h"
#define BITS 15
#include "hw/ftlcdc200_template.h"
#define BITS 16
#include "hw/ftlcdc200_template.h"
#define BITS 24
#include "hw/ftlcdc200_template.h"
#define BITS 32
#include "hw/ftlcdc200_template.h"

static int ftlcdc200_enabled(Ftlcdc200State *s)
{
    uint32_t mask = FER_EN | FER_ON;
    return ((s->fer & mask) == mask)
            && s->bpp && s->cols && s->rows && s->fb[0];
}

/* Update interrupts.  */
static void ftlcdc200_update_irq(Ftlcdc200State *s)
{
    uint32_t mask = s->ier & s->isr;

    if (mask) {
        qemu_irq_raise(s->irq[IRQ_ALL]);
        qemu_set_irq(s->irq[IRQ_FIFOUR],  (mask & ISR_FIFOUR) ? 1 : 0);
        qemu_set_irq(s->irq[IRQ_BASEUPT], (mask & ISR_NEXTFB) ? 1 : 0);
        qemu_set_irq(s->irq[IRQ_VSTATUS], (mask & ISR_VCOMP) ? 1 : 0);
        qemu_set_irq(s->irq[IRQ_BUSERR],  (mask & ISR_BUSERR) ? 1 : 0);
    } else {
        qemu_irq_lower(s->irq[IRQ_ALL]);
        qemu_irq_lower(s->irq[IRQ_VSTATUS]);
        qemu_irq_lower(s->irq[IRQ_BASEUPT]);
        qemu_irq_lower(s->irq[IRQ_FIFOUR]);
        qemu_irq_lower(s->irq[IRQ_BUSERR]);
    }
}

static void ftlcdc200_update_display(void *opaque)
{
    Ftlcdc200State *s = FTLCDC200(opaque);
    DisplaySurface *surface = qemu_console_surface(s->con);
    drawfn *fntable;
    drawfn fn;
    int dest_width;
    int src_width;
    int bpp_offset;
    int first;
    int last;

    if (!ftlcdc200_enabled(s)) {
        return;
    }

    switch (surface_bits_per_pixel(surface)) {
    case 0:
        return;
    case 8:
        fntable = ftlcdc200_draw_fn_8;
        dest_width = 1;
        break;
    case 15:
        fntable = ftlcdc200_draw_fn_15;
        dest_width = 2;
        break;
    case 16:
        fntable = ftlcdc200_draw_fn_16;
        dest_width = 2;
        break;
    case 24:
        fntable = ftlcdc200_draw_fn_24;
        dest_width = 3;
        break;
    case 32:
        fntable = ftlcdc200_draw_fn_32;
        dest_width = 4;
        break;
    default:
        fprintf(stderr, "ftlcdc200: Bad color depth\n");
        abort();
    }

    bpp_offset = 0;
    fn = fntable[s->bpp + bpp_offset];

    src_width = s->cols;
    switch (s->bpp) {
    case BPP_1:
        src_width >>= 3;
        break;
    case BPP_2:
        src_width >>= 2;
        break;
    case BPP_4:
        src_width >>= 1;
        break;
    case BPP_8:
        break;
    case BPP_16:
    case BPP_16_565:
    case BPP_12:
        src_width <<= 1;
        break;
    case BPP_32:
        src_width <<= 2;
        break;
    }
    dest_width *= s->cols;
    first = 0;
    framebuffer_update_display(surface,
                               sysbus_address_space(&s->busdev),
                               s->fb[0], s->cols, s->rows,
                               src_width, dest_width, 0,
                               s->invalidate,
                               fn, s->palette,
                               &first, &last);
    if (s->ier & (IER_VCOMP | IER_NEXTFB)) {
        s->isr |= (IER_VCOMP | IER_NEXTFB);
        ftlcdc200_update_irq(s);
    }
    if (first >= 0) {
        dpy_gfx_update(s->con, 0, first, s->cols, last - first + 1);
    }
    s->invalidate = 0;
}

static void ftlcdc200_invalidate_display(void *opaque)
{
    Ftlcdc200State *s = FTLCDC200(opaque);
    s->invalidate = 1;
    if (ftlcdc200_enabled(s)) {
        qemu_console_resize(s->con, s->cols, s->rows);
    }
}

static void ftlcdc200_update_palette(Ftlcdc200State *s, int n)
{
    DisplaySurface *surface = qemu_console_surface(s->con);
    int i;
    uint32_t raw;
    unsigned int r, g, b;

    raw = s->raw_palette[n];
    n <<= 1;
    for (i = 0; i < 2; i++) {
        r = extract32(raw,  0, 5) << 3;
        g = extract32(raw,  5, 5) << 3;
        b = extract32(raw, 10, 5) << 3;
        /* The I bit is ignored.  */
        raw >>= 6;
        switch (surface_bits_per_pixel(surface)) {
        case 8:
            s->palette[n] = rgb_to_pixel8(r, g, b);
            break;
        case 15:
            s->palette[n] = rgb_to_pixel15(r, g, b);
            break;
        case 16:
            s->palette[n] = rgb_to_pixel16(r, g, b);
            break;
        case 24:
        case 32:
            s->palette[n] = rgb_to_pixel32(r, g, b);
            break;
        }
        n++;
    }
}

static void ftlcdc200_resize(Ftlcdc200State *s, int width, int height)
{
    if (width != s->cols || height != s->rows) {
        if (ftlcdc200_enabled(s)) {
            qemu_console_resize(s->con, width, height);
        }
    }
    s->cols = width;
    s->rows = height;
}

static uint64_t
ftlcdc200_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftlcdc200State *s = FTLCDC200(opaque);

    switch (addr) {
    case REG_FER:
        return s->fer;
    case REG_PPR:
        return s->ppr;
    case REG_IER:
        return s->ier;
    case REG_ISR:
        return s->isr;
    case REG_FB0:
        return s->fb[0];
    case REG_FB1:
        return s->fb[1];
    case REG_FB2:
        return s->fb[2];
    case REG_FB3:
        return s->fb[3];
    case REG_HT:
        return s->ht;
    case REG_VT0:
        return s->vt0;
    case REG_VT1:
        return s->vt1;
    case REG_POL:
        return s->pol;
    case REG_SPPR:
        return s->sppr;
    case 0xA00 ... 0xBFC:   /* palette.  */
        return s->raw_palette[(addr - 0xA00) >> 2];
    /* we don't care */
    case REG_CCIR:
    case 0x300 ... 0x310:   /* image parameters */
    case 0x400 ... 0x40C:   /* color management */
    case 0x600 ... 0x8FC:   /* gamma correction */
    case 0xC00 ... 0xD3C:   /* cstn parameters */
    case 0x1100 ... 0x112C: /* scalar control */
    case 0x2000 ... 0xBFFC: /* osd control */
        return 0;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftlcdc200: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        return 0;
    }
}

static void
ftlcdc200_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftlcdc200State *s = FTLCDC200(opaque);
    int n;

    /* For simplicity invalidate the display whenever a control register
       is written to.  */
    s->invalidate = 1;

    switch (addr) {
    case REG_FER:
        s->fer = (uint32_t)val;
        if (ftlcdc200_enabled(s)) {
            qemu_console_resize(s->con, s->cols, s->rows);
        }
        break;
    case REG_PPR:
        s->ppr = (uint32_t)val;
        switch (s->ppr & PPR_RGB_MASK) {
        case PPR_RGB1:
            s->bpp = BPP_1;
            break;
        case PPR_RGB2:
            s->bpp = BPP_2;
            break;
        case PPR_RGB4:
            s->bpp = BPP_4;
            break;
        case PPR_RGB8:
            s->bpp = BPP_8;
            break;
        case PPR_RGB12:
            s->bpp = BPP_12;
            break;
        case PPR_RGB16_555:
            s->bpp = BPP_16;
            break;
        case PPR_RGB16_565:
            s->bpp = BPP_16_565;
            break;
        case PPR_RGB24:
        default:
            s->bpp = BPP_32;
            break;
        }
        if (ftlcdc200_enabled(s)) {
            qemu_console_resize(s->con, s->cols, s->rows);
        }
        break;
    case REG_IER:
        s->ier = (uint32_t)val;
        ftlcdc200_update_irq(s);
        break;
    case REG_ISCR:
        s->isr &= ~((uint32_t)val);
        ftlcdc200_update_irq(s);
        break;
    case REG_FB0:
        s->fb[0] = (uint32_t)val;
        break;
    case REG_FB1:
        s->fb[1] = (uint32_t)val;
        break;
    case REG_FB2:
        s->fb[2] = (uint32_t)val;
        break;
    case REG_FB3:
        s->fb[3] = (uint32_t)val;
        break;
    case REG_HT:
        s->ht = (uint32_t)val;
        n = ((s->ht & 0xff) + 1) << 4;
        ftlcdc200_resize(s, n, s->rows);
        break;
    case REG_VT0:
        s->vt0 = (uint32_t)val;
        n = (s->vt0 & 0xfff) + 1;
        ftlcdc200_resize(s, s->cols, n);
        break;
    case REG_VT1:
        s->vt1 = (uint32_t)val;
        break;
    case REG_POL:
        s->pol = (uint32_t)val;
        break;
    case REG_SPPR:
        s->sppr = (uint32_t)val;
        break;
    case 0xA00 ... 0xBFC:   /* palette.  */
        n = (addr - 0xA00) >> 2;
        s->raw_palette[(addr - 0xA00) >> 2] = val;
        ftlcdc200_update_palette(s, n);
        break;
    case 0x300 ... 0x310:   /* image parameters */
    case 0x400 ... 0x40C:   /* color management */
    case 0x600 ... 0x8FC:   /* gamma correction */
    case 0xC00 ... 0xD3C:   /* cstn parameters */
    case 0x1100 ... 0x112C: /* scalar control */
    case 0x2000 ... 0xBFFC: /* osd control */
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftlcdc200: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftlcdc200_mem_read,
    .write = ftlcdc200_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void ftlcdc200_reset(DeviceState *ds)
{
    Ftlcdc200State *s = FTLCDC200(SYS_BUS_DEVICE(ds));

    s->fer   = 0;
    s->ppr   = 0;
    s->ier   = 0;
    s->isr   = 0;
    s->sppr  = 0;
    s->fb[0] = 0;
    s->fb[1] = 0;
    s->fb[2] = 0;
    s->fb[3] = 0;
    s->ht    = 0;
    s->vt0   = 0;
    s->vt1   = 0;
    s->pol   = 0;
    s->cols  = 0;
    s->rows  = 0;
    s->bpp   = 0;
    s->invalidate = 1;

    memset(s->raw_palette, 0, sizeof(s->raw_palette));

    /* Make sure we redraw, and at the right size */
    ftlcdc200_invalidate_display(s);
}

static const GraphicHwOps ftlcdc200_ops = {
    .invalidate  = ftlcdc200_invalidate_display,
    .gfx_update  = ftlcdc200_update_display,
};

static void ftlcdc200_realize(DeviceState *dev, Error **errp)
{
    Ftlcdc200State *s = FTLCDC200(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTLCDC200,
                          0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq[IRQ_ALL]);
    sysbus_init_irq(sbd, &s->irq[IRQ_VSTATUS]);
    sysbus_init_irq(sbd, &s->irq[IRQ_BASEUPT]);
    sysbus_init_irq(sbd, &s->irq[IRQ_FIFOUR]);
    sysbus_init_irq(sbd, &s->irq[IRQ_BUSERR]);
    s->con = graphic_console_init(dev, 0, &ftlcdc200_ops, s);
}

static void ftlcdc200_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset   = ftlcdc200_reset;
    dc->vmsd    = &vmstate_ftlcdc200;
    dc->realize = ftlcdc200_realize;
 //   dc->no_user = 1;
}

static const TypeInfo ftlcdc200_info = {
    .name          = TYPE_FTLCDC200,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Ftlcdc200State),
    .class_init    = ftlcdc200_class_init,
};

static void ftlcdc200_register_types(void)
{
    type_register_static(&ftlcdc200_info);
}

type_init(ftlcdc200_register_types)
