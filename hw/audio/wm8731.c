/*
 * WM8731 audio codec.
 *
 * base is wm8750.c
 *
 * Copyright (c) 2013 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/hw.h"
#include "hw/i2c/i2c.h"
#include "hw/audio.h"
#include "audio/audio.h"

#define IN_PORT_N   2
#define OUT_PORT_N  1

#define TYPE_WM8731 "wm8731"

typedef struct WMRate {
    int adc;
    int adc_hz;
    int dac;
    int dac_hz;
} WMRate;

typedef struct WM8731State {
    AudioCodecState parent;

    uint8_t i2c_data[2];
    int i2c_len;
    QEMUSoundCard card;
    SWVoiceIn *adc_voice[IN_PORT_N];
    SWVoiceOut *dac_voice[OUT_PORT_N];
    void (*data_req)(void *, int, int);
    void *opaque;
    uint8_t data_in[4096];
    uint8_t data_out[4096];
    int idx_in, req_in;
    int idx_out, req_out;

    SWVoiceOut **out[2];
    uint8_t outvol[2];
    SWVoiceIn **in[2];
    uint8_t invol[2], inmute[2], mutemic;

    uint8_t mute;
    uint8_t power, format, active;
    const WMRate *rate;
    uint8_t rate_vmstate;
    int adc_hz, dac_hz, ext_adc_hz, ext_dac_hz, master;
} WM8731State;

#define WM8731(obj) \
    OBJECT_CHECK(WM8731State, obj, TYPE_WM8731)

#define WM8731_OUTVOL_TRANSFORM(x)  (x << 1)
#define WM8731_INVOL_TRANSFORM(x)   (x << 3)

static inline void wm8731_in_load(WM8731State *s)
{
    if (s->idx_in + s->req_in <= sizeof(s->data_in)) {
        return;
    }
    s->idx_in = audio_MAX(0, (int) sizeof(s->data_in) - s->req_in);
    AUD_read(*s->in[0], s->data_in + s->idx_in,
             sizeof(s->data_in) - s->idx_in);
}

static inline void wm8731_out_flush(WM8731State *s)
{
    int sent = 0;
    while (sent < s->idx_out) {
        sent += AUD_write(*s->out[0], s->data_out + sent, s->idx_out - sent)
                ? 0 : s->idx_out;
    }
    s->idx_out = 0;
}

static void wm8731_audio_in_cb(void *opaque, int avail_b)
{
    WM8731State *s = WM8731(opaque);
    s->req_in = avail_b;
    /* 16 bit samples */
    s->data_req(s->opaque, s->req_out >> 1, avail_b >> 1);
}

static void wm8731_audio_out_cb(void *opaque, int free_b)
{
    WM8731State *s = WM8731(opaque);

    if (s->idx_out >= free_b) {
        s->idx_out = free_b;
        s->req_out = 0;
        wm8731_out_flush(s);
    } else {
        s->req_out = free_b - s->idx_out;
    }
    /* 16 bit samples */
    s->data_req(s->opaque, s->req_out >> 1, s->req_in >> 1);
}

static const WMRate wm_rate_table[] = {
    {  256, 48000,  256, 48000 },    /* SR: 0000, BOSR: 0 */
    {  384, 48000,  384, 48000 },    /* SR: 0000, BOSR: 1 */
    {  256, 48000,  256,  8000 },    /* SR: 0001, BOSR: 0 */
    {  384, 48000,  384,  8000 },    /* SR: 0001, BOSR: 1 */
    {  256,  8000,  256, 48000 },    /* SR: 0010, BOSR: 0 */
    {  384,  8000,  384, 48000 },    /* SR: 0010, BOSR: 1 */
    {  256,  8000,  256,  8000 },    /* SR: 0011, BOSR: 0 */
    {  384,  8000,  384,  8000 },    /* SR: 0011, BOSR: 1 */
    {  256, 32000,  256, 32000 },    /* SR: 0110, BOSR: 0 */
    {  384, 32000,  384, 32000 },    /* SR: 0110, BOSR: 1 */
    {  128, 96000,  128, 96000 },    /* SR: 0111, BOSR: 0 */
    {  192, 96000,  192, 96000 },    /* SR: 0111, BOSR: 1 */
    {  256, 44100,  256, 44100 },    /* SR: 1000, BOSR: 0 */
    {  384, 44100,  384, 44100 },    /* SR: 1000, BOSR: 1 */
    {  256, 44100,  256,  8000 },    /* SR: 1001, BOSR: 0 */
    {  384, 44100,  384,  8000 },    /* SR: 1001, BOSR: 1 */
    {  256,  8000,  256, 44100 },    /* SR: 1010, BOSR: 0 */
    {  384,  8000,  384, 44100 },    /* SR: 1010, BOSR: 1 */
    {  256,  8000,  256,  8000 },    /* SR: 1011, BOSR: 0 */
    {  384,  8000,  384,  8000 },    /* SR: 1011, BOSR: 1 */
    {  128, 88200,  128, 88200 },    /* SR: 1011, BOSR: 0 */
    {  192, 88200,  192, 88200 },    /* SR: 1011, BOSR: 1 */
};

static void wm8731_vol_update(WM8731State *s)
{
    AUD_set_volume_in(s->adc_voice[0], s->mute,
                    s->inmute[0] ? 0 : WM8731_INVOL_TRANSFORM(s->invol[0]),
                    s->inmute[1] ? 0 : WM8731_INVOL_TRANSFORM(s->invol[1]));
    AUD_set_volume_in(s->adc_voice[1], s->mute,
                    s->mutemic ? 0 : WM8731_INVOL_TRANSFORM(s->invol[0]),
                    s->mutemic ? 0 : WM8731_INVOL_TRANSFORM(s->invol[1]));

    /* Headphone: LOUT1VOL ROUT1VOL */
    AUD_set_volume_out(s->dac_voice[0], s->mute,
                    WM8731_OUTVOL_TRANSFORM(s->outvol[0]),
                    WM8731_OUTVOL_TRANSFORM(s->outvol[1]));
}

static void wm8731_set_format(WM8731State *s)
{
    int i;
    struct audsettings in_fmt;
    struct audsettings out_fmt;

    wm8731_out_flush(s);

    if (s->in[0] && *s->in[0]) {
        AUD_set_active_in(*s->in[0], 0);
    }
    if (s->out[0] && *s->out[0]) {
        AUD_set_active_out(*s->out[0], 0);
    }
    for (i = 0; i < IN_PORT_N; ++i) {
        if (s->adc_voice[i]) {
            AUD_close_in(&s->card, s->adc_voice[i]);
            s->adc_voice[i] = NULL;
        }
    }
    for (i = 0; i < OUT_PORT_N; ++i) {
        if (s->dac_voice[i]) {
            AUD_close_out(&s->card, s->dac_voice[i]);
            s->dac_voice[i] = NULL;
        }
    }

    /* Setup input */
    in_fmt.endianness = 0;
    in_fmt.nchannels = 2;
    in_fmt.freq = s->adc_hz;
    in_fmt.fmt = AUD_FMT_S16;

    s->adc_voice[0] = AUD_open_in(&s->card,
                                  s->adc_voice[0],
                                  TYPE_WM8731 ".line-in",
                                  s,
                                  wm8731_audio_in_cb,
                                  &in_fmt);
    s->adc_voice[1] = AUD_open_in(&s->card,
                                  s->adc_voice[1],
                                  TYPE_WM8731 ".microphone",
                                  s,
                                  wm8731_audio_in_cb,
                                  &in_fmt);

    /* Setup output */
    out_fmt.endianness = 0;
    out_fmt.nchannels = 2;
    out_fmt.freq = s->dac_hz;
    out_fmt.fmt = AUD_FMT_S16;

    s->dac_voice[0] = AUD_open_out(&s->card,
                                   s->dac_voice[0],
                                   TYPE_WM8731 ".headphone",
                                   s,
                                   wm8731_audio_out_cb,
                                   &out_fmt);

    wm8731_vol_update(s);

    /* We should connect the left and right channels to their
     * respective inputs/outputs but we have completely no need
     * for mixing or combining paths to different ports, so we
     * connect both channels to where the left channel is routed.  */
    if (s->in[0] && *s->in[0]) {
        AUD_set_active_in(*s->in[0], 1);
    }
    if (s->out[0] && *s->out[0]) {
        AUD_set_active_out(*s->out[0], 1);
    }
}

static void wm8731_clk_update(WM8731State *s, int ext)
{
    if (s->master || !s->ext_dac_hz) {
        s->dac_hz = s->rate->dac_hz;
    } else {
        s->dac_hz = s->ext_dac_hz;
    }
    if (s->master || !s->ext_adc_hz) {
        s->adc_hz = s->rate->adc_hz;
    } else {
        s->adc_hz = s->ext_adc_hz;
    }
    if (s->master || (!s->ext_dac_hz && !s->ext_adc_hz)) {
        if (!ext) {
            wm8731_set_format(s);
        }
    } else {
        if (ext) {
            wm8731_set_format(s);
        }
    }
}

static void wm8731_reset(I2CSlave *i2c)
{
    WM8731State *s = WM8731(i2c);
    s->rate = &wm_rate_table[0];
    wm8731_clk_update(s, 1);
    s->in[0] = &s->adc_voice[0];
    s->invol[0] = 0x17;
    s->invol[1] = 0x17;
    s->out[0] = &s->dac_voice[0];
    s->outvol[0] = 0x39;
    s->outvol[1] = 0x39;
    s->inmute[0] = 0;
    s->inmute[1] = 0;
    s->mutemic = 1;
    s->mute = 1;
    s->power = 0x9f;
    s->format = 0x02;    /* I2S, 16-bits */
    s->active = 0;
    s->idx_in = sizeof(s->data_in);
    s->req_in = 0;
    s->idx_out = 0;
    s->req_out = 0;
    wm8731_vol_update(s);
    s->i2c_len = 0;
}

static void wm8731_event(I2CSlave *i2c, enum i2c_event event)
{
    WM8731State *s = WM8731(i2c);

    switch (event) {
    case I2C_START_SEND:
        s->i2c_len = 0;
        break;
    case I2C_FINISH:
#ifdef VERBOSE
        if (s->i2c_len < 2) {
            printf("wm8731_event: message too short (%i bytes)\n",
                    s->i2c_len);
        }
#endif
        break;
    default:
        break;
    }
}

#define WM8731_LINVOL   0x00
#define WM8731_RINVOL   0x01
#define WM8731_LOUT1V   0x02
#define WM8731_ROUT1V   0x03
#define WM8731_APANA    0x04
#define WM8731_APDIGI   0x05
#define WM8731_PWR      0x06
#define WM8731_IFACE    0x07
#define WM8731_SRATE    0x08
#define WM8731_ACTIVE   0x09
#define WM8731_RESET    0x0f

static int wm8731_tx(I2CSlave *i2c, uint8_t data)
{
    WM8731State *s = WM8731(i2c);
    uint8_t cmd;
    uint16_t value;

    if (s->i2c_len >= 2) {
#ifdef VERBOSE
        printf("wm8731_tx: long message (%i bytes)\n", s->i2c_len);
#endif
        return 1;
    }
    s->i2c_data[s->i2c_len++] = data;
    if (s->i2c_len != 2) {
        return 0;
    }

    cmd = s->i2c_data[0] >> 1;
    value = ((s->i2c_data[0] << 8) | s->i2c_data[1]) & 0x1ff;

    switch (cmd) {
    case WM8731_LINVOL:
        s->invol[0] = value & 0x1f;         /* LINVOL */
        s->inmute[0] = (value >> 7) & 1;    /* LINMUTE */
        wm8731_vol_update(s);
        break;
    case WM8731_RINVOL:
        s->invol[1] = value & 0x1f;         /* RINVOL */
        s->inmute[1] = (value >> 7) & 1;    /* RINMUTE */
        wm8731_vol_update(s);
        break;
    case WM8731_LOUT1V:
        s->outvol[0] = value & 0x7f;        /* LHPVOL */
        wm8731_vol_update(s);
        break;
    case WM8731_ROUT1V:
        s->outvol[1] = value & 0x7f;        /* RHPVOL */
        wm8731_vol_update(s);
        break;
    case WM8731_APANA:
        s->mutemic = (value >> 1) & 1;      /* MUTEMIC */
        if (value & 0x04) {
            s->in[0] = &s->adc_voice[1];    /* MIC */
        } else {
            s->in[0] = &s->adc_voice[0];    /* LINE-IN */
        }
        break;
    case WM8731_APDIGI:
        if (s->mute != ((value >> 3) & 1)) {
            s->mute = (value >> 3) & 1;     /* DACMU */
            wm8731_vol_update(s);
        }
        break;
    case WM8731_PWR:
        s->power = (uint8_t)(value & 0xff);
        wm8731_set_format(s);
        break;
    case WM8731_IFACE:
        s->format = value;
        s->master = (value >> 6) & 1;       /* MS */
        wm8731_clk_update(s, s->master);
        break;
    case WM8731_SRATE:
        s->rate = &wm_rate_table[(value >> 1) & 0x1f];
        wm8731_clk_update(s, 0);
        break;
    case WM8731_ACTIVE:
        s->active = (uint8_t)(value & 1);
        break;
    case WM8731_RESET:    /* Reset */
        if (value == 0) {
            wm8731_reset(I2C_SLAVE(&s->parent));
        }
        break;

#ifdef VERBOSE
    default:
        printf("wm8731_tx: unknown register %02x\n", cmd);
#endif
    }

    return 0;
}

static int wm8731_rx(I2CSlave *i2c)
{
    return 0x00;
}

static void wm8731_pre_save(void *opaque)
{
    WM8731State *s = WM8731(opaque);

    s->rate_vmstate = s->rate - wm_rate_table;
}

static int wm8731_post_load(void *opaque, int version_id)
{
    WM8731State *s = WM8731(opaque);

    s->rate = &wm_rate_table[s->rate_vmstate & 0x1f];
    return 0;
}

static const VMStateDescription vmstate_wm8731 = {
    .name = TYPE_WM8731,
    .version_id = 0,
    .minimum_version_id = 0,
    .minimum_version_id_old = 0,
    .pre_save = wm8731_pre_save,
    .post_load = wm8731_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(i2c_data, WM8731State, 2),
        VMSTATE_INT32(i2c_len, WM8731State),
        VMSTATE_INT32(idx_in, WM8731State),
        VMSTATE_INT32(req_in, WM8731State),
        VMSTATE_INT32(idx_out, WM8731State),
        VMSTATE_INT32(req_out, WM8731State),
        VMSTATE_UINT8_ARRAY(outvol, WM8731State, 2),
        VMSTATE_UINT8_ARRAY(invol, WM8731State, 2),
        VMSTATE_UINT8_ARRAY(inmute, WM8731State, 2),
        VMSTATE_UINT8(mutemic, WM8731State),
        VMSTATE_UINT8(mute, WM8731State),
        VMSTATE_UINT8(format, WM8731State),
        VMSTATE_UINT8(power, WM8731State),
        VMSTATE_UINT8(active, WM8731State),
        VMSTATE_UINT8(rate_vmstate, WM8731State),
        VMSTATE_END_OF_LIST()
    }
};

static int wm8731_init(I2CSlave *i2c)
{
    WM8731State *s = WM8731(i2c);

    AUD_register_card(TYPE_WM8731, &s->card);
    wm8731_reset(I2C_SLAVE(&s->parent));

    return 0;
}

static void wm8731_data_req_set(DeviceState *dev,
                void (*data_req)(void *, int, int), void *opaque)
{
    WM8731State *s = WM8731(dev);
    s->data_req = data_req;
    s->opaque = opaque;
}

static void wm8731_dac_dat(void *opaque, uint32_t sample)
{
    WM8731State *s = WM8731(opaque);
    /* 16-bit samples */
    *(uint16_t *) &s->data_out[s->idx_out] = (uint16_t)sample;
    s->req_out -= 2;
    s->idx_out += 2;
    if (s->idx_out >= sizeof(s->data_out) || s->req_out <= 0) {
        wm8731_out_flush(s);
    }
}

static uint32_t wm8731_adc_dat(void *opaque)
{
    WM8731State *s = WM8731(opaque);
    uint16_t sample;

    if (s->idx_in >= sizeof(s->data_in)) {
        wm8731_in_load(s);
    }

    sample = *(uint16_t *) &s->data_in[s->idx_in];
    s->req_in -= 2;
    s->idx_in += 2;
    return sample;
}

static void wm8731_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *sc = I2C_SLAVE_CLASS(klass);
    AudioCodecClass *ac = AUDIO_CODEC_CLASS(klass);

    ac->data_req_set = wm8731_data_req_set;
    ac->dac_dat = wm8731_dac_dat;
    ac->adc_dat = wm8731_adc_dat;

    sc->init  = wm8731_init;
    sc->event = wm8731_event;
    sc->recv  = wm8731_rx;
    sc->send  = wm8731_tx;
    dc->vmsd  = &vmstate_wm8731;
}

static const TypeInfo wm8731_info = {
    .name          = TYPE_WM8731,
    .parent        = TYPE_AUDIO_CODEC,
    .instance_size = sizeof(WM8731State),
    .class_init    = wm8731_class_init,
};

static void wm8731_register_types(void)
{
    type_register_static(&wm8731_info);
}

type_init(wm8731_register_types)
