/*
 * Faraday FTKBC010 emulator for A369.
 *
 * Copyright (c) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * In A369 EVB, the FTKBC010 is configured as a keypad controller.
 * It acts like a group of hard wired buttons on the board, each of them
 * is monitored by the FTKBC010, and coordinated as (x, y).
 * However there is a pinmux issue in A369 EVB, the Y-axis usually
 * malfunctioned, so there are only 3 button emulated here.
 *
 * This code is licensed under GNU GPL v2+
 */

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/devices.h"
#include "ui/console.h"
#include "sysemu/sysemu.h"

#include "hw/ftkbc010.h"

#define CFG_REGSIZE     (0x3c / 4)

/* Key codes */
#define KEYCODE_ESC             1
#define KEYCODE_BACKSPACE       14
#define KEYCODE_ENTER           28
#define KEYCODE_SPACE           57
#define KEYCODE_MENU            139    /* Menu (show menu) */

#define TYPE_A369KPD            "a369-kpd"

typedef struct A369KPDState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;
    qemu_irq irq;

    /* HW registers */
    uint32_t regs[CFG_REGSIZE];
} A369KPDState;

#define A369KPD(obj) \
    OBJECT_CHECK(A369KPDState, obj, TYPE_A369KPD)

#define KBC_REG32(s, off) \
    ((s)->regs[(off) / 4])

static void a369kpd_update_irq(A369KPDState *s)
{
    uint32_t ier = 0;

    /* keypad interrupt */
    ier |= (KBC_REG32(s, REG_CR) & CR_KPDEN) ? ISR_KPDI : 0;
    /* tx interrupt */
    ier |= (KBC_REG32(s, REG_CR) & CR_TXIEN) ? ISR_TXI : 0;
    /* rx interrupt */
    ier |= (KBC_REG32(s, REG_CR) & CR_RXIEN) ? ISR_RXI : 0;

    qemu_set_irq(s->irq, (ier & KBC_REG32(s, REG_ISR)) ? 1 : 0);
}

static uint64_t
a369kpd_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    A369KPDState *s = A369KPD(opaque);
    uint64_t ret = 0;

    switch (addr) {
    case REG_CR ... REG_ASPR:
        ret = s->regs[addr / 4];
        break;
    case REG_REVR:
        ret = 0x00010403;  /* rev. = 1.4.3 */
        break;
    case REG_FEAR:
        ret = 0x00000808;  /* 8x8 scan code for keypad */
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "a369kpd: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
a369kpd_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    A369KPDState *s = A369KPD(opaque);

    switch (addr) {
    case REG_CR:
        KBC_REG32(s, REG_CR) = (uint32_t)val;
        /* if ftkbc010 enabled */
        if (!(KBC_REG32(s, REG_CR) & CR_EN)) {
            break;
        }
        /* if keypad interrupt cleared */
        if (KBC_REG32(s, REG_CR) & CR_KPDIC) {
            KBC_REG32(s, REG_CR) &= ~CR_KPDIC;
            KBC_REG32(s, REG_ISR) &= ~ISR_KPDI;
        }
        /* if rx interrupt cleared */
        if (KBC_REG32(s, REG_CR) & CR_RXICLR) {
            KBC_REG32(s, REG_CR) &= ~CR_RXICLR;
            KBC_REG32(s, REG_ISR) &= ~ISR_RXI;
        }
        /* if tx interrupt cleared */
        if (KBC_REG32(s, REG_CR) & CR_TXICLR) {
            KBC_REG32(s, REG_CR) &= ~CR_TXICLR;
            KBC_REG32(s, REG_ISR) &= ~ISR_TXI;
        }
        a369kpd_update_irq(s);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "a369kpd: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps mmio_ops = {
    .read  = a369kpd_mem_read,
    .write = a369kpd_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void a369kpd_key_event(void *opaque, int scancode)
{
    A369KPDState *s = A369KPD(opaque);
    int x, y, released = 0;

    /* key release from qemu */
    if (scancode & 0x80) {
        released = 1;
    }

    /* strip qemu key release bit */
    scancode &= ~0x80;

    /* keypad interrupt */
    if (!released && (KBC_REG32(s, REG_CR) & CR_KPDEN)) {
        switch (scancode) {
        case KEYCODE_ESC:
        case KEYCODE_BACKSPACE:
            x = 1;
            break;
        case KEYCODE_ENTER:
        case KEYCODE_MENU:
        case KEYCODE_SPACE:
            x = 3;
            break;
        default:
            x = 2;    /* KEY_HOME */
            break;
        }
        y = 0;
        KBC_REG32(s, REG_KPDXR) = ~BIT(x);
        KBC_REG32(s, REG_KPDYR) = ~BIT(y);
        KBC_REG32(s, REG_ISR)  |= ISR_KPDI;
        a369kpd_update_irq(s);
    }
}

static void a369kpd_reset(DeviceState *ds)
{
    A369KPDState *s = A369KPD(SYS_BUS_DEVICE(ds));

    memset(s->regs, 0, sizeof(s->regs));
    KBC_REG32(s, REG_KPDXR) = 0xffffffff;
    KBC_REG32(s, REG_KPDYR) = 0xffffffff;

    qemu_irq_lower(s->irq);
}

static void a369kpd_realize(DeviceState *dev, Error **errp)
{
    A369KPDState *s = A369KPD(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->iomem,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_A369KPD,
                          0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    qemu_add_kbd_event_handler(a369kpd_key_event, s);
}

static const VMStateDescription vmstate_ftkbc010 = {
    .name = TYPE_A369KPD,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, A369KPDState, CFG_REGSIZE),
        VMSTATE_END_OF_LIST(),
    }
};

static void a369kpd_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc  = TYPE_A369KPD;
    dc->vmsd  = &vmstate_ftkbc010;
    dc->reset = a369kpd_reset;
    dc->realize = a369kpd_realize;
}

static const TypeInfo a369kpd_info = {
    .name          = TYPE_A369KPD,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(A369KPDState),
    .class_init    = a369kpd_class_init,
};

static void a369kpd_register_types(void)
{
    type_register_static(&a369kpd_info);
}

type_init(a369kpd_register_types)
