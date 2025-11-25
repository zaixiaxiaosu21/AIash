#include <stdio.h>
#include "bsp/bsp_board.h"
#include "audio/audio_encoder.h"
#include "audio/audio_decoder.h"
#include "audio/audio_sr.h"
#include "esp_log.h"
#define  TAG "main"
void audio_sr_callback(void* event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void* event_data){
    switch (event_id)
    {
        case AUDIO_SR_EVENT_WAKEUP:
            ESP_LOGI(TAG, "Wakeup event received: %s", (char *)event_data);
            break;
        case AUDIO_SR_EVENT_SPEECH:
            ESP_LOGI(TAG, "Speech detected");
            break;
        case AUDIO_SR_EVENT_SILENCE:
            ESP_LOGI(TAG, "Silence detected");
            break;
        default:
            ESP_LOGW(TAG, "Unknown event ID: %ld", event_id);
            break;
    }
    

}

void audio_output_task(void *arg){
    RingbufHandle_t output =(RingbufHandle_t )arg;
    bsp_board_t *board = bsp_board_get_instance();
    while(1){
         size_t size = 0;
        void*buffer=  xRingbufferReceiveUpTo(output,&size, portTICK_PERIOD_MS,2048);
        if(buffer==NULL){
            continue;
        }
        esp_codec_dev_write(board->codec_dev, buffer, size);
        vRingbufferReturnItem(output, buffer);
    }
}
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
    bsp_board_lcd_init(board);
    if (bsp_board_check_status(board, BSP_BOARD_BUTTON_BIT|BSP_BOARD_CODEC_BIT|BSP_BOARD_LED_BIT|BSP_BOARD_LCD_BIT, portMAX_DELAY))
    {
        ESP_LOGI(TAG, "Board initialized successfully.");
    }else{
        ESP_LOGE(TAG, "Board initialization failed.");
    }
    //注册按键回调
    iot_button_register_cb(board->front_button, BUTTON_SINGLE_CLICK,NULL,button_callback,NULL);
    iot_button_register_cb(board->front_button, BUTTON_DOUBLE_CLICK ,NULL,button_callback,NULL);
     // 打开音频设备
    esp_codec_dev_set_out_vol(board->codec_dev, 60);// 设置音量为60%
    esp_codec_dev_set_in_gain(board->codec_dev, 20);
    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = CODEC_BIT_WIDTH,
        .sample_rate = CODEC_SAMPLE_RATE,
        .channel = 2,
    };
    esp_codec_dev_open(board->codec_dev, &sample_info);

    //测试音频编解码，创建输入 中转 输出缓冲区
    RingbufHandle_t input = xRingbufferCreate(16384, RINGBUF_TYPE_BYTEBUF);
    RingbufHandle_t mid = xRingbufferCreate(2048, RINGBUF_TYPE_NOSPLIT);
    RingbufHandle_t output = xRingbufferCreate(16384*2, RINGBUF_TYPE_BYTEBUF);

     audio_sr_t *sr = audio_sr_create(input); //创建语音识别模块
    audio_sr_start(sr); //启动语音识别模块
    audio_sr_register_callback(sr, AUDIO_SR_EVENT_SPEECH, audio_sr_callback, sr);
    audio_sr_register_callback(sr, AUDIO_SR_EVENT_SILENCE, audio_sr_callback, sr);
    audio_sr_register_callback(sr, AUDIO_SR_EVENT_WAKEUP, audio_sr_callback, sr);
    //创建音频编解码任务
    audio_encoder_t *encoder = audio_encoder_create(input, mid, CODEC_SAMPLE_RATE, CODEC_BIT_WIDTH, 1);
    audio_decoder_t *decoder = audio_decoder_create(mid, output, CODEC_SAMPLE_RATE, 2);
    audio_encoder_start(encoder);
    audio_decoder_start(decoder);
    //创建输出任务
    
    xTaskCreate(audio_output_task, "audio_output_task", 4096, output, 6, NULL);
   
}