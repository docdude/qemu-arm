/*
 * Audio Codec Class
 *
 * Copyright (c) 2013 Faraday Technology
 * Written by Dante Su <dantesu@faraday-tech.com>
 *
 * This file is licensed under GNU GPL v2+.
 */

#ifndef QEMU_AUDIO_CODEC_H
#define QEMU_AUDIO_CODEC_H

#include "qdev.h"
#include "i2c/i2c.h"

typedef I2CSlave    AudioCodecState;

#define TYPE_AUDIO_CODEC "audio-codec"
#define AUDIO_CODEC(obj) \
     OBJECT_CHECK(AudioCodecState, (obj), TYPE_AUDIO_CODEC)

typedef struct AudioCodecClass {
    /*< private >*/
    I2CSlaveClass parent;

    /*< public >*/
    void     (*data_req_set)(DeviceState *dev,
                             void (*data_req)(void *, int, int),
                             void *opaque);
    void     (*dac_dat)(void *opaque, uint32_t sample);
    uint32_t (*adc_dat)(void *opaque);
    void    *(*dac_buffer)(void *opaque, int samples);
    void     (*dac_commit)(void *opaque);
    void     (*set_bclk_in)(void *opaque, int new_hz);
} AudioCodecClass;

#define AUDIO_CODEC_CLASS(klass) \
     OBJECT_CLASS_CHECK(AudioCodecClass, (klass), TYPE_AUDIO_CODEC)
#define AUDIO_CODEC_GET_CLASS(obj) \
     OBJECT_GET_CLASS(AudioCodecClass, (obj), TYPE_AUDIO_CODEC)

void audio_codec_data_req_set(DeviceState *dev,
                              void (*data_req)(void *, int, int),
                              void *opaque);

void audio_codec_dac_dat(void *opaque, uint32_t sample);

uint32_t audio_codec_adc_dat(void *opaque);

void *audio_codec_dac_buffer(void *opaque, int samples);

void audio_codec_dac_commit(void *opaque);

void audio_codec_set_bclk_in(void *opaque, int new_hz);

#endif
