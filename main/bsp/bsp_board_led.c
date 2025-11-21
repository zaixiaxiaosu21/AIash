#include "bsp_board.h"
#include  "sdkconfig.h"

void bsp_board_led_init(bsp_board_t *board){
     led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,          // 控制引脚
        .max_leds = LED_NUM,                 // 两颗灯
        .led_model = LED_MODEL_WS2812, // WS2812 灯型
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .flags.with_dma = true,
    };
  esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &board->led_strip);
    ESP_ERROR_CHECK(ret);
    // LED 初始化结束，设置状态位
    xEventGroupSetBits(board->board_status, BSP_BOARD_LED_BIT);
}