#pragma once
#include "bsp_board_config.h"
#include "led_strip.h"
#include "iot_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_codec_dev.h"

#define BSP_BOARD_LED_BIT (1 << 0)
#define BSP_BOARD_BUTTON_BIT (1 << 1)
#define BSP_BOARD_NVS_BIT (1 << 2)
#define BSP_BOARD_WIFI_BIT (1 << 3)
#define BSP_BOARD_CODEC_BIT (1 << 4)

typedef struct
{
    EventGroupHandle_t board_status;
    // 灯带
    led_strip_handle_t led_strip;
    // 按键
    button_handle_t front_button;
    button_handle_t back_button;
    // 编解码器设备
    // 音频
    esp_codec_dev_handle_t codec_dev; // 音频

} bsp_board_t;

bsp_board_t *bsp_board_get_instance();

void bsp_board_led_init(bsp_board_t *board);

void bsp_board_button_init(bsp_board_t *board);

void bsp_board_nvs_init(bsp_board_t *board);

void bsp_board_wifi_init(bsp_board_t *board);

void bsp_board_codec_init(bsp_board_t *board);

bool bsp_board_check_status(bsp_board_t *board, EventBits_t status_bit, uint32_t timeout_ms);
