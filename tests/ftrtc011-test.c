/*
 * QTest testcase for the FTRTC011 real-time clock
 *
 * Copyright (c) 2013 Kuo-Jung Su
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
#include "libqtest.h"
#include "hw/ftrtc011.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define A369_FTRTC011_BASE      0x92100000
#define A369_FTRTC011_IRQ_ALARM 42  /* edge triggered */
#define A369_FTRTC011_IRQ_SEC   43  /* edge triggered */
#define A369_FTRTC011_IRQ_MIN   44  /* edge triggered */
#define A369_FTRTC011_IRQ_HOUR  45  /* edge triggered */

#define CFG_BASEYEAR            2010

static time_t ts_base;

static uint32_t rtc_read(uint32_t reg)
{
    return readl(A369_FTRTC011_BASE + reg);
}

static void rtc_write(uint32_t reg, uint32_t val)
{
    writel(A369_FTRTC011_BASE + reg, val);
}

static int rtc_get_irq(int irq)
{
#if 0   /* It looks to me that get_irq() doesn't work well
         * with edge interrupts.
         */
    return get_irq(irq);
#else
    switch (irq) {
    case A369_FTRTC011_IRQ_ALARM:
        return !!(rtc_read(REG_ISR) & ISR_ALARM);
    case A369_FTRTC011_IRQ_SEC:
        return !!(rtc_read(REG_ISR) & ISR_SEC);
    case A369_FTRTC011_IRQ_MIN:
        return !!(rtc_read(REG_ISR) & ISR_MIN);
    case A369_FTRTC011_IRQ_HOUR:
        return !!(rtc_read(REG_ISR) & ISR_HOUR);
    default:
        return 0;
    }
#endif
}

static int tm_cmp(struct tm *lhs, struct tm *rhs)
{
    time_t a, b;
    struct tm d1, d2;

    memcpy(&d1, lhs, sizeof(d1));
    memcpy(&d2, rhs, sizeof(d2));

    a = mktime(&d1);
    b = mktime(&d2);

    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    }

    return 0;
}

static void rtc_start(void)
{
    time_t ts;
    struct tm base;

    if (!ts_base) {
        base.tm_isdst = -1;
        base.tm_year  = CFG_BASEYEAR - 1900;
        base.tm_mon   = 0;
        base.tm_mday  = 1;
        base.tm_hour  = 0;
        base.tm_min   = 0;
        base.tm_sec   = 0;
        ts_base = mktime(&base);
    }

    ts = time(NULL) - ts_base;
    rtc_write(REG_WDAY, ts / 86400LL);
    ts %= 86400LL;
    rtc_write(REG_WHOUR, ts / 3600LL);
    ts %= 3600LL;
    rtc_write(REG_WMIN, ts / 60LL);
    ts %= 60LL;
    rtc_write(REG_WSEC, ts);

    rtc_write(REG_ISR, ISR_MASK);
    rtc_write(REG_CR, CR_EN | CR_LOAD | CR_INTR_MASK);
}

static void rtc_get_datetime(struct tm *date)
{
    time_t ts;
    int64_t sec, min, hour, day;

    if (!ts_base) {
        fprintf(stderr, "ts_base is not yet initialized!\n");
        exit(1);
    }

    sec  = rtc_read(REG_SEC);
    min  = rtc_read(REG_MIN);
    hour = rtc_read(REG_HOUR);
    day  = rtc_read(REG_DAY);
    ts   = ts_base + (86400LL * day) + (hour * 3600LL) + (min * 60LL) + sec;

    localtime_r(&ts, date);
}

static void rtc_test_check_time(int wiggle)
{
    struct tm start, date[4], end;
    struct tm *datep;
    time_t ts;

    rtc_start();

    /*
     * This check assumes a few things.
     * First, we cannot guarantee that we get a consistent reading
     * from the wall clock because we may hit an edge of the clock
     * while reading.
     * To work around this, we read four clock readings such that
     * at least two of them should match.  We need to assume that one
     * reading is corrupt so we need four readings to ensure that we have at
     * least two consecutive identical readings
     *
     * It's also possible that we'll cross an edge reading the host clock so
     * simply check to make sure that the clock reading is within the period of
     * when we expect it to be.
     */

    ts = time(NULL);
    localtime_r(&ts, &start);

    rtc_get_datetime(&date[0]);
    rtc_get_datetime(&date[1]);
    rtc_get_datetime(&date[2]);
    rtc_get_datetime(&date[3]);

    ts = time(NULL);
    localtime_r(&ts, &end);

    if (tm_cmp(&date[0], &date[1]) == 0) {
        datep = &date[0];
    } else if (tm_cmp(&date[1], &date[2]) == 0) {
        datep = &date[1];
    } else if (tm_cmp(&date[2], &date[3]) == 0) {
        datep = &date[2];
    } else {
        g_assert_not_reached();
    }

    if (!(tm_cmp(&start, datep) <= 0 && tm_cmp(datep, &end) <= 0)) {
        time_t t, s;

        start.tm_isdst = datep->tm_isdst;

        t = mktime(datep);
        s = mktime(&start);
        if (t < s) {
            g_test_message("RTC is %ld second(s) behind wall-clock\n",
                           (s - t));
        } else {
            g_test_message("RTC is %ld second(s) ahead of wall-clock\n",
                           (t - s));
        }

        g_assert_cmpint(ABS(t - s), <=, wiggle);
    }
}

static int wiggle = 2;

static void rtc_test_intr_alarm(void)
{
    time_t ts;
    int i;

    rtc_start();

    g_assert(!rtc_get_irq(A369_FTRTC011_IRQ_ALARM));

    /* schedule an alarm at 2 sec later */
    ts = time(NULL) + 2 - ts_base;
    ts %= 86400LL;
    rtc_write(REG_ALARM_HOUR, ts / 3600LL);
    ts %= 3600LL;
    rtc_write(REG_ALARM_MIN,  ts / 60LL);
    ts %= 60LL;
    rtc_write(REG_ALARM_SEC,  ts);

    for (i = 0; i < 2 + wiggle; i++) {
        /*
         * Since it's an edge interrupt (pulse),
         * is it always possible to get the high level signal observed?
         */
        if (rtc_get_irq(A369_FTRTC011_IRQ_ALARM)) {
            break;
        }

        clock_step(1000000000LL);
    }

    g_assert(i < 2 + wiggle);
}

static void rtc_test_intr_sec(void)
{
    int i;

    rtc_start();

    g_assert(!rtc_get_irq(A369_FTRTC011_IRQ_SEC));

    for (i = 2; i > 0; i--) {

        if (rtc_get_irq(A369_FTRTC011_IRQ_SEC)) {
            break;
        }

        clock_step(1000000000);
    }

    g_assert(i > 0);
}

static void rtc_test_intr_min(void)
{
    int i;

    rtc_start();

    g_assert(!get_irq(A369_FTRTC011_IRQ_MIN));

    for (i = 60 + 2; i > 0; i--) {

        if (rtc_get_irq(A369_FTRTC011_IRQ_MIN)) {
            break;
        }

        clock_step(1000000000);
    }

    g_assert(i > 0);
}

static void rtc_test_intr_hour(void)
{
    int i;

    rtc_start();

    g_assert(!get_irq(A369_FTRTC011_IRQ_HOUR));

    for (i = 3600 + 2; i > 0; i--) {

        if (rtc_get_irq(A369_FTRTC011_IRQ_HOUR)) {
            break;
        }

        clock_step(1000000000);
    }

    g_assert(i > 0);
}

static void set_time(int h, int m, int s)
{
    rtc_write(REG_WDAY, rtc_read(REG_DAY));
    rtc_write(REG_WHOUR, h);
    rtc_write(REG_WMIN, m);
    rtc_write(REG_WSEC, s);

    /* update rtc counter */
    rtc_write(REG_CR, rtc_read(REG_CR) | CR_LOAD);
}

#define assert_time(h, m, s) \
    do { \
        g_assert_cmpint(rtc_read(REG_HOUR), ==, h); \
        g_assert_cmpint(rtc_read(REG_MIN), ==, m); \
        g_assert_cmpint(rtc_read(REG_SEC), ==, s); \
    } while (0)

static void rtc_test_basic_24h(void)
{
    rtc_start();

    /* set decimal 24 hour mode */
    set_time(9, 59, 0);
    clock_step(1000000000LL);
    assert_time(9, 59, 1);
    clock_step(59000000000LL);
    assert_time(10, 0, 0);

    /* test hour wraparound */
    set_time(9, 59, 0);
    clock_step(60000000000LL);
    assert_time(10, 0, 0);

    /* test day wraparound */
    set_time(23, 59, 0);
    clock_step(60000000000LL);
    assert_time(0, 0, 0);
}

static void rtc_test_set_year(void)
{
    char t[256];
    int i, year;
    time_t ts;
    struct tm date;

    rtc_start();

    for (i = 0; i < 1000; ++i) {
        year = CFG_BASEYEAR + g_test_rand_int_range(0, 20);

        date.tm_isdst = -1;
        date.tm_year  = year - 1900;
        date.tm_mon   = g_test_rand_int_range(0, 11);
        date.tm_mday  = g_test_rand_int_range(1, 25);
        date.tm_hour  = g_test_rand_int_range(0, 23);
        date.tm_min   = g_test_rand_int_range(0, 59);
        date.tm_sec   = g_test_rand_int_range(0, 59);

        asctime_r(&date, t);

        ts = mktime(&date) - ts_base;
        rtc_write(REG_WDAY, ts / 86400LL);
        ts %= 86400LL;
        rtc_write(REG_WHOUR, ts / 3600LL);
        ts %= 3600LL;
        rtc_write(REG_WMIN, ts / 60LL);
        ts %= 60LL;
        rtc_write(REG_WSEC, ts);

        /* update rtc counter */
        rtc_write(REG_CR, rtc_read(REG_CR) | CR_LOAD);

        rtc_get_datetime(&date);

        asctime_r(&date, t);

        g_assert(date.tm_year == year - 1900);
    }
}

/* success if no crash or abort */
static void rtc_test_fuzz(void)
{
    unsigned int i;

    rtc_start();

    for (i = 0; i < 1000; i++) {
        uint8_t reg, val;

        reg = (uint8_t)g_test_rand_int_range(0x00, 0x34) & 0xfc;
        val = (uint8_t)g_test_rand_int_range(0, 256);

        rtc_write(reg, val);
        rtc_read(reg);
    }
}

int main(int argc, char **argv)
{
    QTestState *s = NULL;
    int ret;

    g_test_init(&argc, &argv, NULL);

    s = qtest_start("-M a369 -nographic -kernel /dev/zero -rtc clock=vm");
    qtest_irq_intercept_in(s, "ftrtc011");
    qtest_add_func("/rtc/check-time", rtc_test_check_time);
    qtest_add_func("/rtc/interrupt/alarm", rtc_test_intr_alarm);
    qtest_add_func("/rtc/interrupt/sec", rtc_test_intr_sec);
    qtest_add_func("/rtc/interrupt/min", rtc_test_intr_min);
    qtest_add_func("/rtc/interrupt/hour", rtc_test_intr_hour);
    qtest_add_func("/rtc/basic-24h", rtc_test_basic_24h);
    qtest_add_func("/rtc/set-year", rtc_test_set_year);
    qtest_add_func("/rtc/fuzz-registers", rtc_test_fuzz);
    ret = g_test_run();

    if (s) {
        qtest_quit(s);
    }

    return ret;
}
