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
   ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_BK_LIGHT, 0));// 关闭背光
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
    esp_lcd_panel_disp_on_off(board->lcd_panel, false);

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

 void lvgl_init_and_demo(bsp_board_t *board)
{
    // 1. 初始化 LVGL port（你之前已经有这段）
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = board->lcd_io,
        .panel_handle  = board->lcd_panel,
        .buffer_size   = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT,  // 240 * 40
        .double_buffer = true,

        .hres          = BSP_LCD_H_RES,   // 240
        .vres          = BSP_LCD_V_RES,   // 320
        .monochrome    = false,

        .rotation = {
            .swap_xy  = false,
            .mirror_x = false,
            .mirror_y = false,
        },

        .flags = {
            .buff_dma    = true,
            .buff_spiram = false,
        },
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    (void)disp;

    // 2. 取当前屏幕
#if LVGL_VERSION_MAJOR >= 9
    lv_obj_t *scr = lv_screen_active();
#else
    lv_obj_t *scr = lv_scr_act();
#endif

    // 可以让整个屏幕用列布局，看起来更整齐
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
                          LV_FLEX_ALIGN_CENTER,   // 主轴居中
                          LV_FLEX_ALIGN_CENTER,   // 交叉轴居中
                          LV_FLEX_ALIGN_CENTER);  // 行内对齐

    // 3. 顶部标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32-S3 + LVGL");
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);

    // 4. 一个按钮 + 按钮上的文字
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click me");
    lv_obj_center(btn_label);

    // 给按钮加个简单事件：点击变文字
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    

    // 5. 一个滑条 + 数值显示
    lv_obj_t *slider = lv_slider_create(scr);
    lv_obj_set_width(slider, 180);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 30, LV_ANIM_OFF);

    lv_obj_t *slider_label = lv_label_create(scr);
    lv_label_set_text(slider_label, "30");

    // 滑动时更新数字
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, slider_label);

    // 6. 一个彩色条块，当“装饰”
    lv_obj_t *color_bar = lv_obj_create(scr);
    lv_obj_set_size(color_bar, 200, 40);
    lv_obj_set_style_bg_color(color_bar, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_radius(color_bar, 8, 0);
    lv_obj_set_style_border_width(color_bar, 0, 0);
}

