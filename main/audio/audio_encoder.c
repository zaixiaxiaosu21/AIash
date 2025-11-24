#include "audio_encoder.h"
#include "esp_audio_enc.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_enc_reg.h"
#include "object.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#define TAG "[AUDIO] Encoder"
#define AUDIO_ENCODER_TASK_STACK_SIZE 32 * 1024
#define AUDIO_ENCODER_TASK_PRIORITY 5
#define AUDIO_ENCODER_TASK_CORE_ID 0
struct audio_encoder
{
    RingbufHandle_t input_buffer;
    RingbufHandle_t output_buffer;

    esp_audio_enc_handle_t enc;

    bool is_running;
};
void audio_encoder_task(void *arg){
    audio_encoder_t * encoder = (audio_encoder_t *)arg;
     // 获取输入的帧大小
    int in_frame_size = 0, out_frame_size = 0;
    esp_audio_enc_get_frame_size(encoder->enc, &in_frame_size, &out_frame_size);
    // 创建输入和输出缓冲区
    void *in_buf = object_create(in_frame_size);
    void *out_buf = object_create(out_frame_size);
    esp_audio_enc_in_frame_t in_frame = {
        .buffer = in_buf,
        .len = in_frame_size,
    };
    esp_audio_enc_out_frame_t out_frame = {
        .buffer = out_buf,
        .len = out_frame_size,
    };
    while(encoder->is_running){
        size_t size_read = 0;
      void * buf_read  = xRingbufferReceiveUpTo(encoder->input_buffer, &size_read, portMAX_DELAY, in_frame_size);
        if(!buf_read){
            continue;
        }
        memcpy(in_buf, buf_read, size_read);
        vRingbufferReturnItem(encoder->input_buffer, buf_read);
        //指针移动
        in_frame_size -= size_read;
        in_buf += size_read;
        if (in_frame_size>0)
        {
            continue;
        }
        // 重置
        in_frame_size = in_frame.len;
        in_buf = in_frame.buffer;
        // 编码
        ESP_ERROR_CHECK(esp_audio_enc_process(encoder->enc, &in_frame, &out_frame));
        // 写入输出缓冲区
       
         if (xRingbufferSend(encoder->output_buffer, out_frame.buffer, out_frame.encoded_bytes, 0) != pdTRUE)
        {
            ESP_LOGW(TAG, "Output buffer is full");
        };
        }
        free(in_buf);
        free(out_buf);
        vTaskDelete(NULL);
}

audio_encoder_t *audio_encoder_create(RingbufHandle_t input_buffer,
                                      RingbufHandle_t output_buffer,
                                      int sample_rate,
                                      int bit_per_sample,
                                      int channels)
{
    audio_encoder_t * encoder= object_create(sizeof(audio_encoder_t));
    encoder->input_buffer = input_buffer;
    encoder->output_buffer = output_buffer;
     ESP_ERROR_CHECK(esp_opus_enc_register()); // Register encoder

     esp_opus_enc_config_t opus_cfg = {
        .sample_rate = sample_rate,
        .bits_per_sample = bit_per_sample,
        .channel = channels,
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS,
        .bitrate = 32000,
        .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,
        .complexity = 0,
        .enable_fec = false,
        .enable_dtx = false,
        .enable_vbr = false,
     };
     esp_audio_enc_config_t cfg={
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_cfg,
        .cfg_sz = sizeof(opus_cfg),
     };

    ESP_ERROR_CHECK(esp_audio_enc_open(&cfg, &encoder->enc));

    return encoder;
}
void audio_encoder_destroy(audio_encoder_t *encoder){
    esp_audio_enc_close(encoder->enc);
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_OPUS);
    free(encoder);
}
void audio_encoder_start(audio_encoder_t *encoder){
    encoder->is_running = true;
    
    xTaskCreatePinnedToCoreWithCaps(audio_encoder_task,
                                    "encoder_task", AUDIO_ENCODER_TASK_STACK_SIZE, encoder, AUDIO_ENCODER_TASK_PRIORITY, NULL, AUDIO_ENCODER_TASK_CORE_ID,MALLOC_CAP_SPIRAM);
    
}   
void audio_encoder_stop(audio_encoder_t *encoder){
    encoder->is_running = false;
}
