#pragma once
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
typedef enum
{
    AUDIO_SR_EVENT_WAKEUP,  // 唤醒事件
    AUDIO_SR_EVENT_SPEECH,  // 语音检测事件
    AUDIO_SR_EVENT_SILENCE, // 静音检测事件
} audio_sr_event_t;
typedef struct audio_sr audio_sr_t;
audio_sr_t *audio_sr_create(RingbufHandle_t output);
void audio_sr_destroy(audio_sr_t *sr);
void audio_sr_start(audio_sr_t *sr);
void audio_sr_stop(audio_sr_t *sr);
void audio_sr_register_callback(audio_sr_t *sr,audio_sr_event_t event, esp_event_handler_t callback, void *arg);