#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "iot_button.h"
#include "button_adc.h"

static const char *TAG = "ADC_BUTTON_TEST";

static led_strip_handle_t led_strip = NULL;

// 按键事件回调
static void button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
    int btn_id = (int)data;

    if (event == BUTTON_PRESS_DOWN) {
        ESP_LOGI(TAG, "Button[%d] Press Down", btn_id);
        
        if (btn_id == 0) {
            // 按键1：点亮灯
            led_strip_set_pixel(led_strip, 0, 0, 0, 255);  // 蓝灯
            led_strip_set_pixel(led_strip, 1, 0, 255, 0);  // 绿灯
            led_strip_refresh(led_strip);
        } else if (btn_id == 1) {
            // 按键2：熄灭灯
            led_strip_clear(led_strip);
            led_strip_refresh(led_strip);
        }
    }

    if (event == BUTTON_PRESS_UP) {
        ESP_LOGI(TAG, "Button[%d] Release", btn_id);
    }
}

void app_main(void)
{
    /** 初始化 WS2812 灯条 **/
    led_strip_config_t strip_config = {
        .strip_gpio_num = 46,          // 控制引脚
        .max_leds = 2,                 // 两颗灯
        .led_model = LED_MODEL_WS2812, // WS2812 灯型
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .flags.with_dma = true,
    };
    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);

    /** 配置 ADC 按键（GPIO8 → ADC1_CH7） **/
    const button_config_t btn_cfg = {0};
    button_adc_config_t btn_adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .adc_channel = ADC_CHANNEL_7, // GPIO8 ✅
    };

    const uint16_t vol[2] = {0, 1650}; // 按键1=0V, 按键2=1.65V
    button_handle_t btns[2] = {NULL};

    for (size_t i = 0; i < 2; i++) {
        btn_adc_cfg.button_index = i;
         if (i == 0) {
            btn_adc_cfg.min = 0;
            btn_adc_cfg.max =(vol[0] + vol[1]) / 2;;   // ~825mV
        }else {
            btn_adc_cfg.min = (vol[0] + vol[1]) / 2; // ~825mV
            btn_adc_cfg.max = (vol[1] + 3000) / 2;   // ~2325mV
        }

        ESP_ERROR_CHECK(iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &btns[i]));
        iot_button_register_cb(btns[i], BUTTON_PRESS_DOWN, NULL, button_event_cb, (void *)i);
        iot_button_register_cb(btns[i], BUTTON_PRESS_UP, NULL, button_event_cb, (void *)i);
    }

    ESP_LOGI(TAG, "ADC button and LED initialized successfully.");
   
    // 主任务空转
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
