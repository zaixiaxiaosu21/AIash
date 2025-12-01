#include "bsp_board.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

 

#define LCD_HOST SPI2_HOST//esp32s3 only support SPI2_HOST and SPI3_HOST for LCD
static void bsp_board_bk_init(bsp_board_t *board){
      gpio_config_t bk_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LCD_PIN_BK_LIGHT),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
 };
   ESP_ERROR_CHECK(gpio_config(&bk_config));
   ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_BK_LIGHT, 1));// 关闭背光
}
static void bsp_board_spi_init(bsp_board_t *board){
       spi_bus_config_t bus_config = {
        .data0_io_num = LCD_PIN_MOSI,
        .data1_io_num = -1,
        .data2_io_num = -1,
        .data3_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .sclk_io_num = LCD_PIN_PCLK,
       };
       ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));
 }
 static void bsp_board_lcd_io_init(bsp_board_t *board){
    
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = SPI_MASTER_FREQ_80M,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
 };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config,&board->lcd_io));

}
static void bsp_board_lcd_panel_init(bsp_board_t *board){
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .color_space = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(board->lcd_io, &panel_config, &board->lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(board->lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(board->lcd_panel));
}
void bsp_board_lcd_init(bsp_board_t *board)
{
    // 背光引脚初始化
    bsp_board_bk_init(board);

    // SPI总线初始化
    bsp_board_spi_init(board);

    // IO handle初始化
    bsp_board_lcd_io_init(board);

    // LCD 面板初始化
    bsp_board_lcd_panel_init(board);

    // 设置状态位
    xEventGroupSetBits(board->board_status, BSP_BOARD_LCD_BIT);

    // 开启LCD
    esp_lcd_panel_disp_on_off(board->lcd_panel, true);

}

 static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * label  = lv_event_get_user_data(e);

    int32_t v = lv_slider_get_value(slider);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)v);
    lv_label_set_text(label, buf);
}
static void btn_event_cb(lv_event_t * e)
{
    static bool toggled = false;
    lv_obj_t *btn   = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);

    if(!toggled) {
        lv_label_set_text(label, "Clicked!");
    } else {
        lv_label_set_text(label, "Click me");
    }
    toggled = !toggled;
}

 

void bsp_board_lcd_on(bsp_board_t *board)
{
    esp_lcd_panel_disp_on_off(board->lcd_panel, true);
    gpio_set_level(LCD_PIN_BK_LIGHT, 1);
}
