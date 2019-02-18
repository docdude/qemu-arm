/*
 * QEMU model of the FTRTC011 RTC Timer
 *
 * Copyright (C) 2012 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "sysemu/sysemu.h"

#include "hw/ftrtc011.h"

enum ftrtc011_irqpin {
    IRQ_ALARM_LEVEL = 0,
    IRQ_ALARM_EDGE,
    IRQ_SEC,
    IRQ_MIN,
    IRQ_HOUR,
    IRQ_DAY,
};

#define TYPE_FTRTC011   "ftrtc011"

#define CFG_REGSIZE     (0x3c / 4)

typedef struct Ftrtc011State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion mmio;

    qemu_irq irq[6];

    QEMUTimer *qtimer;
    int64_t rtc_base;
    int64_t rtc_start;

    /* HW register caches */
    uint32_t regs[CFG_REGSIZE];
} Ftrtc011State;

#define FTRTC011(obj) \
    OBJECT_CHECK(Ftrtc011State, obj, TYPE_FTRTC011)

#define RTC_REG32(s, off) \
    ((s)->regs[(off) / 4])

/* Update interrupts.  */
static void ftrtc011_update_irq(Ftrtc011State *s)
{
    uint32_t mask = extract32(RTC_REG32(s, REG_CR), 1, 5)
                  & RTC_REG32(s, REG_ISR);

    qemu_set_irq(s->irq[IRQ_ALARM_LEVEL], !!(mask & ISR_ALARM));

    if (mask) {
        if (mask & ISR_SEC) {
            qemu_irq_pulse(s->irq[IRQ_SEC]);
        }
        if (mask & ISR_MIN) {
            qemu_irq_pulse(s->irq[IRQ_MIN]);
        }
        if (mask & ISR_HOUR) {
            qemu_irq_pulse(s->irq[IRQ_HOUR]);
        }
        if (mask & ISR_DAY) {
            qemu_irq_pulse(s->irq[IRQ_DAY]);
        }
        if (mask & ISR_ALARM) {
            qemu_irq_pulse(s->irq[IRQ_ALARM_EDGE]);
        }
    }
}

static void ftrtc011_timer_rebase(Ftrtc011State *s)
{
    int64_t ticks = get_ticks_per_sec();
    int64_t elapsed = RTC_REG32(s, REG_SEC)
                    + (60LL * RTC_REG32(s, REG_MIN))
                    + (3600LL * RTC_REG32(s, REG_HOUR))
                    + (86400LL * RTC_REG32(s, REG_DAY));

    s->rtc_base  = elapsed;
    s->rtc_start = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    /* adjust to the beginning of the current second */
    s->rtc_start = s->rtc_start - (s->rtc_start % ticks);
}

static void ftrtc011_timer_update(Ftrtc011State *s)
{
    int64_t elapsed;
    uint8_t sec, min, hr;
    uint32_t day;

    /* check if RTC is enabled */
    if (!(RTC_REG32(s, REG_CR) & CR_EN)) {
        return;
    }

    /*
     * 2013.03.11 Kuo-Jung Su
     * Under QTest, in the very beginning of test,
     * qemu_get_clock_ns(rtc_clock) would somehow jumps
     * from 0 sec into 436474 sec.
     * So I have to apply this simple work-aroud to make it work.
     */
    if (!s->rtc_start) {
        ftrtc011_timer_rebase(s);
    }

    /*
     * Although the timer is supposed to tick per second,
     * there is no guarantee that the tick interval is exactly
     * 1 and only 1 second.
     */
    elapsed = s->rtc_base
            + (qemu_clock_get_ns(QEMU_CLOCK_REALTIME) - s->rtc_start) / 1000000000LL;
    day = (uint32_t)(elapsed / 86400LL);
    elapsed %= 86400LL;
    hr = (uint8_t)(elapsed / 3600LL);
    elapsed %= 3600LL;
    min = (uint8_t)(elapsed / 60LL);
    elapsed %= 60LL;
    sec = elapsed;

    /* sec interrupt */
    if ((RTC_REG32(s, REG_SEC) != sec)
        && (RTC_REG32(s, REG_CR) & CR_INTR_SEC)) {
        RTC_REG32(s, REG_ISR) |= ISR_SEC;
    }
    /* min interrupt */
    if ((RTC_REG32(s, REG_MIN) != min)
        && (RTC_REG32(s, REG_CR) & CR_INTR_MIN)) {
        RTC_REG32(s, REG_ISR) |= ISR_MIN;
    }
    /* hr interrupt */
    if ((RTC_REG32(s, REG_HOUR) != hr)
        && (RTC_REG32(s, REG_CR) & CR_INTR_HOUR)) {
        RTC_REG32(s, REG_ISR) |= ISR_HOUR;
    }
    /* day interrupt */
    if ((RTC_REG32(s, REG_DAY) != day)
        && (RTC_REG32(s, REG_CR) & CR_INTR_DAY)) {
        RTC_REG32(s, REG_ISR) |= ISR_DAY;
    }

    /* update RTC registers */
    RTC_REG32(s, REG_SEC) = (uint8_t)sec;
    RTC_REG32(s, REG_MIN) = (uint8_t)min;
    RTC_REG32(s, REG_HOUR) = (uint8_t)hr;
    RTC_REG32(s, REG_DAY) = (uint16_t)day;

    /* alarm interrupt */
    if (RTC_REG32(s, REG_CR) & CR_INTR_ALARM) {
        if ((RTC_REG32(s, REG_SEC)
            | (RTC_REG32(s, REG_MIN) << 8)
            | (RTC_REG32(s, REG_HOUR) << 16)) ==
                (RTC_REG32(s, REG_ALARM_SEC)
                | (RTC_REG32(s, REG_ALARM_MIN) << 8)
                | (RTC_REG32(s, REG_ALARM_HOUR) << 16))) {
            RTC_REG32(s, REG_ISR) |= ISR_ALARM;
        }
    }

    /* set interrupt signal */
    ftrtc011_update_irq(s);
}

static void ftrtc011_timer_resche(Ftrtc011State *s)
{
    uint32_t cr = RTC_REG32(s, REG_CR);
    int64_t  ticks = get_ticks_per_sec();
    int64_t  timeout = 0;

    if ((cr & CR_EN) && (cr & CR_INTR_MASK)) {
        timeout = qemu_clock_get_ns(QEMU_CLOCK_REALTIME) + ticks;
    }

    if (timeout > 0) {
        timer_mod(s->qtimer, timeout);
    }
}

static uint64_t
ftrtc011_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    Ftrtc011State *s = FTRTC011(opaque);
    uint32_t ret = 0;

    switch (addr) {
    case REG_SEC ... REG_DAY:
        ftrtc011_timer_update(s);
        /* fall-through */
    case REG_ALARM_SEC ... REG_ISR:
        ret = s->regs[addr / 4];
        break;
    case REG_REVR:
        ret = 0x00010000;  /* rev. 1.0.0 */
        break;
    case REG_CURRENT:
        ftrtc011_timer_update(s);
        ret |= RTC_REG32(s, REG_DAY) << 17;
        ret |= RTC_REG32(s, REG_HOUR) << 12;
        ret |= RTC_REG32(s, REG_MIN) << 6;
        ret |= RTC_REG32(s, REG_SEC);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftrtc011: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    return ret;
}

static void
ftrtc011_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Ftrtc011State *s = FTRTC011(opaque);

    switch (addr) {
    case REG_ALARM_SEC ... REG_ALARM_HOUR:
        /* fall-through */
    case REG_WSEC ... REG_WHOUR:
        s->regs[addr / 4] = (uint32_t)val & 0xff;
        break;
    case REG_WDAY:
        s->regs[addr / 4] = (uint32_t)val & 0xffff;
        break;
    case REG_CR:
        /* check if RTC is enabled */
        if (val & CR_EN) {
            /* update the RTC counter with the user supplied values */
            if (val & CR_LOAD) {
                RTC_REG32(s, REG_SEC) = RTC_REG32(s, REG_WSEC);
                RTC_REG32(s, REG_MIN) = RTC_REG32(s, REG_WMIN);
                RTC_REG32(s, REG_HOUR) = RTC_REG32(s, REG_WHOUR);
                RTC_REG32(s, REG_DAY) = RTC_REG32(s, REG_WDAY);
                val &= ~CR_LOAD;
                ftrtc011_timer_rebase(s);
            } else if (!(RTC_REG32(s, REG_CR) & CR_EN)) {
                ftrtc011_timer_rebase(s);
            }
        } else {
            timer_del(s->qtimer);
        }
        RTC_REG32(s, REG_CR) = (uint32_t)val;
        /* reschedule a RTC timer update */
        ftrtc011_timer_resche(s);
        break;
    case REG_ISR:
        RTC_REG32(s, REG_ISR) &= ~((uint32_t)val);
        ftrtc011_update_irq(s);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "ftrtc011: undefined memory access@%#" HWADDR_PRIx "\n", addr);
        break;
    }

    if (RTC_REG32(s, REG_ALARM_SEC) > 59
        || RTC_REG32(s, REG_ALARM_MIN) > 59
        || RTC_REG32(s, REG_ALARM_HOUR) > 23
        || RTC_REG32(s, REG_WSEC) > 59
        || RTC_REG32(s, REG_WMIN) > 59
        || RTC_REG32(s, REG_WHOUR) > 23) {
        goto werr;
    }

    return;

werr:
    qemu_log_mask(LOG_GUEST_ERROR,
            "ftrtc011: %u is an invalid value to reg@0x%02x.\n",
            (uint32_t)val, (uint32_t)addr);
}

static const MemoryRegionOps mmio_ops = {
    .read  = ftrtc011_mem_read,
    .write = ftrtc011_mem_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void ftrtc011_timer_tick(void *opaque)
{
    Ftrtc011State *s = FTRTC011(opaque);
    ftrtc011_timer_update(s);
    ftrtc011_timer_resche(s);
}

static void ftrtc011_reset(DeviceState *ds)
{
    Ftrtc011State *s = FTRTC011(SYS_BUS_DEVICE(ds));

    timer_del(s->qtimer);
    memset(s->regs, 0, sizeof(s->regs));
    ftrtc011_update_irq(s);
}

static void ftrtc011_save(QEMUFile *f, void *opaque)
{
    int i;
    Ftrtc011State *s = FTRTC011(opaque);

    for (i = 0; i < sizeof(s->regs) / 4; ++i) {
        qemu_put_be32(f, s->regs[i]);
    }
}

static int ftrtc011_load(QEMUFile *f, void *opaque, int version_id)
{
    int i;
    Ftrtc011State *s = FTRTC011(opaque);

    for (i = 0; i < sizeof(s->regs) / 4; ++i) {
        s->regs[i] = qemu_get_be32(f);
    }

    return 0;
}

static void ftrtc011_realize(DeviceState *dev, Error **errp)
{
    Ftrtc011State *s = FTRTC011(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    s->qtimer = timer_new_ns(QEMU_CLOCK_REALTIME, ftrtc011_timer_tick, s);

    memory_region_init_io(&s->mmio,
			  OBJECT(s),
                          &mmio_ops,
                          s,
                          TYPE_FTRTC011,
                          0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq[IRQ_ALARM_LEVEL]);
    sysbus_init_irq(sbd, &s->irq[IRQ_ALARM_EDGE]);
    sysbus_init_irq(sbd, &s->irq[IRQ_SEC]);
    sysbus_init_irq(sbd, &s->irq[IRQ_MIN]);
    sysbus_init_irq(sbd, &s->irq[IRQ_HOUR]);
    sysbus_init_irq(sbd, &s->irq[IRQ_DAY]);

    register_savevm(dev, TYPE_FTRTC011, -1, 0,
                    ftrtc011_save, ftrtc011_load, s);
}

static const VMStateDescription vmstate_ftrtc011 = {
    .name = TYPE_FTRTC011,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, Ftrtc011State, CFG_REGSIZE),
        VMSTATE_END_OF_LIST()
    }
};

static void ftrtc011_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd    = &vmstate_ftrtc011;
    dc->reset   = ftrtc011_reset;
    dc->realize = ftrtc011_realize;
//    dc->no_user = 1;
}

static const TypeInfo ftrtc011_info = {
    .name           = TYPE_FTRTC011,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(Ftrtc011State),
    .class_init     = ftrtc011_class_init,
};

static void ftrtc011_register_types(void)
{
    type_register_static(&ftrtc011_info);
}

type_init(ftrtc011_register_types)
