#include "audio_sr.h"
#include "esp_event.h"
#include "object.h"
#include "esp_afe_sr_models.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/ringbuf.h>
#include "bsp/bsp_board.h"
#define TAG "[AUDIO] SR"
#define FEED_TASK_STACK_SIZE 4096
#define FEED_TASK_CORE 0
#define FEED_TASK_PRIORITY 5

#define FETCH_TASK_STACK_SIZE 4096
#define FETCH_TASK_CORE 0
#define FETCH_TASK_PRIORITY 5
ESP_EVENT_DEFINE_BASE(AUDIO_SR_BASE);
struct audio_sr
{
    bool is_running;

    // sr模型句柄
    const esp_afe_sr_iface_t *afe_handle;
    esp_afe_sr_data_t *afe_data ;
    //输出结果缓存
    RingbufHandle_t output;
    esp_event_loop_handle_t event_loop; //事件循环句柄
    vad_state_t last_vad_state; 
    
};
void feedtask(void *arg){
    audio_sr_t *sr = (audio_sr_t *)arg;
    const esp_afe_sr_iface_t *afe_handle = sr->afe_handle;
    esp_afe_sr_data_t *afe_data = sr->afe_data;
    
    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);
    size_t feed_size = feed_chunksize * feed_nch * sizeof(int16_t);
    int16_t *feed_buff = (int16_t *)object_create(feed_size);
    bsp_board_t *board=  bsp_board_get_instance();
    while (sr->is_running) {
        esp_codec_dev_read(board->codec_dev, feed_buff, feed_size);
        afe_handle->feed(afe_data, feed_buff); //将数据喂给模型
    }   
    free(feed_buff);
    vTaskDelete(NULL);
    
}
void fetchtask(void *arg){
    audio_sr_t *sr = (audio_sr_t *)arg;
    const esp_afe_sr_iface_t *afe_handle = sr->afe_handle;
    esp_afe_sr_data_t *afe_data = sr->afe_data;
    while (sr->is_running)
    {
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);
        vad_state_t vad_state = result->vad_state;
        wakenet_state_t wakeup_state = result->wakeup_state;
        if (wakeup_state == WAKENET_DETECTED)
        {
           esp_event_post_to(sr->event_loop,AUDIO_SR_BASE,AUDIO_SR_EVENT_WAKEUP,"我是theshy",13,0);
        }
            vad_state = result->vad_state; //获取vad状态
        if (vad_state!=sr->last_vad_state)
        {   
            sr->last_vad_state = vad_state;
            esp_event_post_to(sr->event_loop, AUDIO_SR_BASE,
                              vad_state == VAD_SPEECH ? AUDIO_SR_EVENT_SPEECH : AUDIO_SR_EVENT_SILENCE,
                              NULL, 0, 0);
        }
        if (sr->last_vad_state == VAD_SPEECH)
        {
             //将处理后的音频数据发送到输出缓冲区(优先将缓存放入环形缓冲区)
        int16_t *processed_audio = result->data;
        if (result->vad_cache_size>0)
        {  
              int16_t *vad_cache = result->vad_cache;
            xRingbufferSend(sr->output, vad_cache, result->vad_cache_size, 0); //将vad缓存放入环形缓冲区
        }
        
           xRingbufferSend(sr->output, processed_audio, result->data_size, 0); //将处理后的音频数据放入环形缓冲区
        }
        
    }
    //释放资源
    vTaskDelete(NULL);
}
audio_sr_t *audio_sr_create(RingbufHandle_t output){
    audio_sr_t *sr = (audio_sr_t *)malloc(sizeof(audio_sr_t));
    sr->output = output;
    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init("MR", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    //打开回声消除
    afe_config->aec_init = true;
    sr->afe_handle= esp_afe_handle_from_config(afe_config);
    sr->afe_data= sr->afe_handle->create_from_config(afe_config);
    free(afe_config);//释放配置
    esp_event_loop_args_t args = {
        .queue_size = 10,
        .task_name = "audio_sr_event_loop",
        .task_priority = 5,
        .task_stack_size =4096,
        .task_core_id = 0,
    };
   ESP_ERROR_CHECK(esp_event_loop_create(&args,&sr->event_loop));
   return sr;

}
void audio_sr_destroy(audio_sr_t *sr){
    sr->afe_handle->destroy(sr->afe_data);
    esp_event_loop_delete(sr->event_loop);
    free(sr);
}
void audio_sr_start(audio_sr_t *sr){
    sr->is_running = true;
    xTaskCreatePinnedToCoreWithCaps(feedtask,"feedtask",FEED_TASK_STACK_SIZE,sr,
                                            FEED_TASK_PRIORITY,NULL,
                                            FEED_TASK_CORE,MALLOC_CAP_SPIRAM);
    xTaskCreatePinnedToCoreWithCaps(fetchtask,"fetchtask",FETCH_TASK_STACK_SIZE,sr,
                                            FETCH_TASK_PRIORITY,NULL,
                                            FETCH_TASK_CORE,MALLOC_CAP_SPIRAM);
}
void audio_sr_stop(audio_sr_t *sr){
    sr->is_running = false;
}
void audio_sr_register_callback(audio_sr_t *sr,audio_sr_event_t event, esp_event_handler_t callback, void *arg){
    esp_event_handler_register_with(sr->event_loop,AUDIO_SR_BASE,event,callback,arg);
}