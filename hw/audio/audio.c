/*
 * Audio Codec Class
 *
 * Copyright (c) 2013 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#include "hw/qdev.h"
#include "hw/i2c/i2c.h"
#include "hw/audio.h"

void audio_codec_data_req_set(DeviceState *dev,
                              void (*data_req)(void *, int, int),
                              void *opaque)
{
    AudioCodecClass *k = AUDIO_CODEC_GET_CLASS(dev);
    if (k->data_req_set) {
        k->data_req_set(dev, data_req, opaque);
    }
}

void audio_codec_dac_dat(void *opaque, uint32_t sample)
{
    AudioCodecClass *k = AUDIO_CODEC_GET_CLASS(opaque);
    if (k->dac_dat) {
        k->dac_dat(opaque, sample);
    }
}

uint32_t audio_codec_adc_dat(void *opaque)
{
    AudioCodecClass *k = AUDIO_CODEC_GET_CLASS(opaque);
    uint32_t ret = 0;
    if (k->adc_dat) {
        ret = k->adc_dat(opaque);
    }
    return ret;
}

void *audio_codec_dac_buffer(void *opaque, int samples)
{
    AudioCodecClass *k = AUDIO_CODEC_GET_CLASS(opaque);
    void *ret = NULL;
    if (k->dac_buffer) {
        ret = k->dac_buffer(opaque, samples);
    }
    return ret;
}

void audio_codec_dac_commit(void *opaque)
{
    AudioCodecClass *k = AUDIO_CODEC_GET_CLASS(opaque);
    if (k->dac_commit) {
        k->dac_commit(opaque);
    }
}

void audio_codec_set_bclk_in(void *opaque, int new_hz)
{
    AudioCodecClass *k = AUDIO_CODEC_GET_CLASS(opaque);
    if (k->set_bclk_in) {
        k->set_bclk_in(opaque, new_hz);
    }
}

static const TypeInfo audio_codec_info = {
    .name          = TYPE_AUDIO_CODEC,
    .parent        = TYPE_I2C_SLAVE,
    .instance_size = sizeof(AudioCodecState),
    .abstract      = true,
    .class_size    = sizeof(AudioCodecClass),
};

static void audio_codec_register_types(void)
{
    type_register_static(&audio_codec_info);
}

type_init(audio_codec_register_types)
