#include "bsp_board.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#define BSP_LCD_H_RES   240
#define BSP_LCD_V_RES   320
#define BSP_LCD_DRAW_BUF_HEIGHT  40 // 每次刷新40行
 

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
   ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_BK_LIGHT, 1));// 打开背光
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
        .color_space = LCD_RGB_ELEMENT_ORDER_BGR,
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

 void lvgl_init_and_demo(bsp_board_t *board)
{
    // 1. 初始化 LVGL 端口（创建 LVGL 任务、互斥锁等）
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 2. 把你已经有的 lcd_io / lcd_panel 挂给 LVGL
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = board->lcd_io,
        .panel_handle  = board->lcd_panel,
        .buffer_size   = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT,  // 240 * 40
        .double_buffer = true,         // 有内存就开，刷新更顺滑

        .hres          = BSP_LCD_H_RES,   // 240
        .vres          = BSP_LCD_V_RES,   // 320
        .monochrome    = false,

        .rotation = {
            .swap_xy  = false,
            .mirror_x = false,
            .mirror_y = false,
        },

        .flags = {
            .buff_dma    = true,    // 显存放片内 RAM + DMA
            .buff_spiram = false,   // 暂时先不用 PSRAM
        },
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    (void)disp; // 暂时不用，防止未使用警告

    // 3. 创建一个简单的 LVGL 标签
#if LVGL_VERSION_MAJOR >= 9
    lv_obj_t *label = lv_label_create(lv_screen_active());
#else
    lv_obj_t *label = lv_label_create(lv_scr_act());
#endif
    lv_label_set_text(label, "Hello LVGL 240x320!");
    lv_obj_center(label);
}
