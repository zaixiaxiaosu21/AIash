#include "application.h"
#include "protocol/ota.h"
#include "protocol/protocol.h"
#include "audio/audio_process.h"
#include "bsp/bsp_board.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "display/xiaozhi_display.h"
#define TAG "Application"
#include "string.h"
static const char *state_str[] =
    {
        "STARTING",
        "ACTIVATING",
        "IDLE",
        "CONNECTING",
        "WAKEUP",
        "LISTENING",
        "SPEAKING",
};

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

    // 唤醒超时定时器
    esp_timer_handle_t wakeup_timer;
    esp_timer_handle_t lvgl_timer;
} app_t;

static app_t s_app;
static void application_update_display(app_t *app)
{
    switch (app->state)
    {
    case APP_STATE_STARTING:
        xiaozhi_display_tip("正在启动...");
        xiaozhi_display_emoji("neutral");
        xiaozhi_display_text("请稍候");
        break;

    case APP_STATE_ACTIVATING:
        xiaozhi_display_tip("激活中...");
        xiaozhi_display_emoji("thinking");
        xiaozhi_display_text("请在手机上完成激活");
        break;

    case APP_STATE_IDLE:
        xiaozhi_display_tip("已就绪");
        xiaozhi_display_emoji("neutral");
        xiaozhi_display_text("对我说：你好小智");
        break;

    case APP_STATE_CONNECTING:
        xiaozhi_display_tip("正在连接服务器...");
        xiaozhi_display_emoji("thinking");
        xiaozhi_display_text("请稍候");
        break;

    case APP_STATE_WAKEUP:
        xiaozhi_display_tip("唤醒成功");
        xiaozhi_display_emoji("happy");
        xiaozhi_display_text("请开始说话");
        break;

    case APP_STATE_LISTENING:
        xiaozhi_display_tip("正在聆听...");
        xiaozhi_display_emoji("thinking");
        // 具体听到什么，在 STT 事件里更新 text
        break;

    case APP_STATE_SPEAKING:
        xiaozhi_display_tip("正在回复...");
        // emoji 后面在 LLM 事件里根据 emotion 再改
        xiaozhi_display_emoji("cool");
        // 具体说什么，在 TTS_SENTENCE_START 里更新 text
        break;

    default:
        break;
    }
}
// Forward declaration
static void application_set_state(app_t *app, app_state_t state);

void audio_sr_callback(void* event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void* event_data){
app_t *self = (app_t *)event_handler_arg;
    switch (event_id)
    {
        case AUDIO_SR_EVENT_WAKEUP:
            // 进入唤醒状态
            if (self->state==APP_STATE_IDLE)
            {   
                protocol_connect(self->protocol);
                application_set_state(self, APP_STATE_CONNECTING);
            }
            if (self->state==APP_STATE_SPEAKING)
            {
                // 唤醒后停止说话
                 protocol_send_abort_speaking(self->protocol, ABORT_REASON_WAKE_WORD);
                 protocol_send_wake_word(self->protocol, "你好小智");
                 application_set_state(self, APP_STATE_WAKEUP);
            }
            break;
        case AUDIO_SR_EVENT_SPEECH:
            // 进入说话状态
            if (self->state == APP_STATE_WAKEUP)
            {
               // audio_processor_set_vad_state(self->audio_processor, true);
                application_set_state(self, APP_STATE_LISTENING);
                protocol_send_start_listening(self->protocol,LISTENING_TYPE_MANUAL); //发送开始监听指令
            }
            
            break;
        case AUDIO_SR_EVENT_SILENCE:
            //实现静音
            if (self->state == APP_STATE_LISTENING)
            {
                audio_processor_set_vad_state(self->audio_processor, false);
                application_set_state(self, APP_STATE_WAKEUP);
                protocol_send_stop_listening(self->protocol); //发送停止监听指令
            }
            
            break;
        default:
            ESP_LOGW(TAG, "Unknown event ID: %ld", event_id);
            break;
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
static void application_set_state(app_t *app, app_state_t state){
    if (state==app->state)
    {
        return;
    }
    ESP_LOGI(TAG, "State changed from %s to %s", state_str[app->state], state_str[state]);
    app->state = state;
    //如果处于唤醒状态启动定时器 定时器的生命周期仅存在唤醒没有发生其他时间时候
    if (app->state==APP_STATE_WAKEUP)
    {
        esp_timer_start_once(app->wakeup_timer, 10000 * 1000); //10秒后超时
    }else{
        esp_timer_stop(app->wakeup_timer);
    }
    application_update_display(app); //更新显示
}
static void application_upload_task(void *arg){
    app_t *s_app = (app_t *)arg;
    void *buffer=malloc(300);
    while (1)
    {
      size_t size=  audio_processor_read(s_app->audio_processor, buffer, 300);//读取编码器output数据
      if (s_app->state==APP_STATE_LISTENING && size>0)
      {
            binary_data_t audio_data = {
                .data = buffer,
                .size = size
            };
            protocol_send_audio(s_app->protocol, &audio_data);
    }
      }
      
      
}
static void application_protocol_callback(void* event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void* event_data){
    app_t *self = (app_t *)event_handler_arg;
    switch (event_id)
    {
    case PROTOCOL_EVENT_CONNECTED:
         if (self->state==APP_STATE_CONNECTING)
         {
            protocol_send_hello(self->protocol);
         }
         break;
    case PROTOCOL_EVENT_DISCONNECTED: 
           audio_processor_set_vad_state(self->audio_processor, false); // 停止VAD
           application_set_state(self, APP_STATE_IDLE);
        break;
    case PROTOCOL_EVENT_HELLO:
         if (self->state==APP_STATE_CONNECTING)
         {
            protocol_send_wake_word(self->protocol, "你好小智");
            application_set_state(self, APP_STATE_WAKEUP);
         }
        break;
    case PROTOCOL_EVENT_STT:
        ESP_LOGI(TAG, "User: %s", (char *)event_data);
        xiaozhi_display_text((char *)event_data); //显示用户说的话
        break;
    case PROTOCOL_EVENT_LLM:
        ESP_LOGI(TAG, "Emotion: %s", (char *)event_data);
        xiaozhi_display_emoji((char *)event_data);
        break;
    case PROTOCOL_EVENT_TTS_START:
        if (self->state == APP_STATE_WAKEUP)
        {
            application_set_state(self, APP_STATE_SPEAKING);
        }
        break;
    case PROTOCOL_EVENT_TTS_SENTENCE_START:
        if (self->state==APP_STATE_SPEAKING)
        {
             ESP_LOGI(TAG, "Assistant: %s", (char *)event_data);
              xiaozhi_display_text((char *)event_data);
        }
        break;
    case PROTOCOL_EVENT_TTS_STOP:
        if (self->state==APP_STATE_SPEAKING)
        {
            application_set_state(self, APP_STATE_WAKEUP);
            audio_processor_set_vad_state(self->audio_processor, true); // 开启VAD
        }
        break;
    case PROTOCOL_EVENT_AUDIO:
       if (self->state == APP_STATE_SPEAKING)
        {
            binary_data_t *audio_data = (binary_data_t *)event_data;
            audio_processor_write(self->audio_processor, audio_data->data, audio_data->size);
        }
        break;
    default:   
        break;
    }
}
static void application_lvgl_timer_cb(void *arg)
{
    //app_t *app = (app_t *)arg;
    bsp_board_t *board = bsp_board_get_instance();
    int wifi_rssi = bsp_board_wifi_get_rssi(board);
    int bateery_soc = 100; //假设电量为100%
    xiaozhi_display_update_status(bateery_soc, wifi_rssi); //更新显示
}
static void application_check_ota(app_t *app, ota_t *ota){
    while (1)
    {
         ota_process(ota);
         if (!ota->activation_code)//如果服务器没有下发激活码 则说明已经激活
         {
            return;
         }
          application_set_state(app,APP_STATE_ACTIVATING);
          ESP_LOGI(TAG, "Activation code: %s", ota->activation_code); // 打印激活码
          char buffer[100];
          sprintf(buffer, "请在手机上使用激活码完成激活:\n%s", ota->activation_code);
          xiaozhi_display_text(buffer);
          vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
void application_init(void)
{
    s_app.state = APP_STATE_STARTING;
    bsp_board_t *board = bsp_board_get_instance();
    bsp_board_nvs_init(board);
    bsp_board_led_init(board);
    bsp_board_button_init(board);
     bsp_board_lcd_init(board);
    xiaozhi_display_init(); //初始化显示
     char payload[150] = {0};
    bsp_board_wifi_init(board,payload, sizeof(payload));
    if (payload[0])
    {
         xiaozhi_display_show_qrcode(payload, strlen(payload));
    }
    
    bsp_board_codec_init(board);
   
    if (bsp_board_check_status(board, BSP_BOARD_BUTTON_BIT|BSP_BOARD_CODEC_BIT|BSP_BOARD_LED_BIT|BSP_BOARD_LCD_BIT|BSP_BOARD_WIFI_BIT, portMAX_DELAY))
    {
        ESP_LOGI(TAG, "Board initialized successfully.");
    }else{
        ESP_LOGE(TAG, "Board initialization failed.");
    }
    //删除二维码
    xiaozhi_display_delete_qrcode();
    //注册按键回调
    iot_button_register_cb(board->front_button, BUTTON_SINGLE_CLICK,NULL,button_callback,NULL);
    iot_button_register_cb(board->front_button, BUTTON_DOUBLE_CLICK ,NULL,button_callback,NULL);
     // 打开音频设备
    esp_codec_dev_set_out_vol(board->codec_dev, 60);// 设置音量为60%
    esp_codec_dev_set_in_gain(board->codec_dev, 5);// 设置增益为5
    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = CODEC_BIT_WIDTH,
        .sample_rate = CODEC_SAMPLE_RATE,
        .channel = 2,
    };
    esp_codec_dev_open(board->codec_dev, &sample_info);
    
   
    //发送ota请求
    ota_t *ota = ota_create();
    application_check_ota(&s_app,ota);
    application_set_state(&s_app, APP_STATE_STARTING);
    
    s_app.protocol = protocol_create(ota->websocket_url, ota->websocket_token);
    ota_destroy(ota);
    
    protocol_register_callback(s_app.protocol, application_protocol_callback, &s_app);
     s_app.audio_processor = audio_processor_create();
    // 注册语音识别回调
    audio_processor_register_callback(s_app.audio_processor, AUDIO_SR_EVENT_WAKEUP, audio_sr_callback, &s_app);
    audio_processor_register_callback(s_app.audio_processor, AUDIO_SR_EVENT_SPEECH, audio_sr_callback, &s_app);
    audio_processor_register_callback(s_app.audio_processor, AUDIO_SR_EVENT_SILENCE, audio_sr_callback, &s_app);
    // 启动音频处理器
    audio_processor_start(s_app.audio_processor);
    // 启动后台上传任务
    xTaskCreatePinnedToCoreWithCaps(application_upload_task, "upload_task", 4096, &s_app, 6, NULL, 0, MALLOC_CAP_SPIRAM);
    //初始关闭vad
    audio_processor_set_vad_state(s_app.audio_processor, false);
    //初始化定时器
    esp_timer_create_args_t wakeup_timer_args = {
        .arg = s_app.protocol,
        .callback=(esp_timer_cb_t)protocol_disconnect,
        .name = "wakeup_timer",
        .skip_unhandled_events = true,
    };
    esp_timer_create(&wakeup_timer_args, &s_app.wakeup_timer);
    //初始化lvgltimer
     esp_timer_create_args_t update_timer_cfg = {
        .callback = application_lvgl_timer_cb,
        .arg = &s_app,
        .skip_unhandled_events = true,
        .name = "visual_timer",
    };
    esp_timer_create(&update_timer_cfg, &s_app.lvgl_timer);
    esp_timer_start_periodic(s_app.lvgl_timer, 1000 * 1000);//每秒更新一次显示状态
    application_set_state(&s_app, APP_STATE_IDLE);
}
