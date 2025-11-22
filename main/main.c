#include <stdio.h>
#include "bsp/bsp_board.h"
#include "esp_log.h"
#define  TAG "main"
static void button_callback(void *button_handle, void *usr_data){
            bsp_board_t *board = bsp_board_get_instance();
            if (button_handle!=board->front_button)
            { 
                ESP_LOGW(TAG, "Button handle does not match front button, ignoring callback");
                 return;
            }
            
            button_event_t event = iot_button_get_event(button_handle); 
            switch (event)
            {
                case BUTTON_SINGLE_CLICK:
                     led_strip_set_pixel(board->led_strip, 0, 255, 255, 255);
                     led_strip_set_pixel(board->led_strip, 1, 255, 255, 255);
                    led_strip_refresh(board->led_strip);
                    ESP_LOGI(TAG, "Front button single clicked");
                    break;
                case BUTTON_DOUBLE_CLICK:
                    led_strip_clear(board->led_strip);
                    ESP_LOGI(TAG, "Front button double clicked");
                    break;
                default:
                    break;
            }

    
}
#define TAG "main"
void app_main(void)
{
    bsp_board_t *board = bsp_board_get_instance();
    bsp_board_nvs_init(board);
    bsp_board_led_init(board);
    bsp_board_button_init(board);
   // bsp_board_wifi_init(board);
    bsp_board_codec_init(board);
    if (bsp_board_check_status(board, BSP_BOARD_BUTTON_BIT|BSP_BOARD_CODEC_BIT|BSP_BOARD_LED_BIT|BSP_BOARD_CODEC_BIT, portMAX_DELAY))
    {
        ESP_LOGI(TAG, "Board initialized successfully.");
    }else{
        ESP_LOGE(TAG, "Board initialization failed.");
    }
    //注册按键回调
    iot_button_register_cb(board->front_button, BUTTON_SINGLE_CLICK,NULL,button_callback,NULL);
    iot_button_register_cb(board->front_button, BUTTON_DOUBLE_CLICK ,NULL,button_callback,NULL);
     // 打开音频设备
    esp_codec_dev_set_out_vol(board->codec_dev, 60);
    esp_codec_dev_set_in_gain(board->codec_dev, 20);
    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = CODEC_BIT_WIDTH,
        .sample_rate = CODEC_SAMPLE_RATE,
        .channel = 1,
    };
    esp_codec_dev_open(board->codec_dev, &sample_info);

    uint8_t *buf = malloc(1024);
    while (1)
    {
        esp_codec_dev_read(board->codec_dev, buf, 1024);
        esp_codec_dev_write(board->codec_dev, buf, 1024);
    }
}