#pragma once
#include "application.h"
#include "protocol/ota.h"
#include "protocol/protocol.h"
#include "audio/audio_process.h"
#include "bsp/bsp_board.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "display/xiaozhi_display.h"
#include "iot/speaker_thing.h"
#include "iot/light_thing.h"
#include "string.h"
typedef enum
{
    APP_STATE_STARTING,
    APP_STATE_ACTIVATING,
    APP_STATE_IDLE,
    APP_STATE_CONNECTING,
    APP_STATE_WAKEUP,
    APP_STATE_LISTENING,
    APP_STATE_SPEAKING,
} app_state_t;
typedef struct
{
    app_state_t state;
    protocol_t *protocol;
    audio_processor_t *audio_processor;
     things_t *things;
    // 唤醒超时定时器
    esp_timer_handle_t wakeup_timer;
    esp_timer_handle_t lvgl_timer;
} app_t;
extern app_t s_app;
void application_init(void);