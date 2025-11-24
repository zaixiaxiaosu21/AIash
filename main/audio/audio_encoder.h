#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

typedef struct audio_encoder audio_encoder_t;

audio_encoder_t *audio_encoder_create(RingbufHandle_t input_buffer,
                                      RingbufHandle_t output_buffer,
                                      int sample_rate,
                                      int bit_per_sample,
                                      int channels);

void audio_encoder_destroy(audio_encoder_t *encoder);

void audio_encoder_start(audio_encoder_t *encoder);

void audio_encoder_stop(audio_encoder_t *encoder);