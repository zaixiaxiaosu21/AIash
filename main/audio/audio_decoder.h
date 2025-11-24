#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

typedef struct audio_decoder audio_decoder_t;

audio_decoder_t *audio_decoder_create(RingbufHandle_t input_buffer,
                                      RingbufHandle_t output_buffer,
                                      int sample_rate,
                                      int channels);

void audio_decoder_destroy(audio_decoder_t *decoder);

void audio_decoder_start(audio_decoder_t *decoder);

void audio_decoder_stop(audio_decoder_t *decoder);