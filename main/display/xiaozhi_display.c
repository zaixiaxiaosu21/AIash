#include "display/xiaozhi_display.h"
#include "esp_lvgl_port.h"
#include "bsp/bsp_board.h"
#include "lvgl.h"
#include "esp_log.h"
#include "font_emoji.h"
#include "font_awesome.h"
#include  "string.h"

#define TAG "xiaozhi_display"
#define BSP_LCD_H_RES   240
#define BSP_LCD_V_RES   320
#define BSP_LCD_DRAW_BUF_HEIGHT  40 // 每次刷新40行
/* 直接用 BSP 里的分辨率 */
            // 每次刷 40 行
typedef struct
{
    char *name; // 显示的文本
    char *emoji;
} xiaozhi_emoji_t;
static lv_display_t *lvgl_disp = NULL;

lv_obj_t *qr;
lv_obj_t *text_label;
lv_obj_t *emoji_label;
lv_obj_t *tip_label;
lv_obj_t *battery_label;
lv_obj_t *wifi_label;

extern const lv_font_t font_puhui_20_4;
extern const lv_font_t font_awesome_16_4;

const xiaozhi_emoji_t emoji_list[21] = {
    {.name = "neutral",    .emoji = "😶"},
    {.name = "happy",      .emoji = "🙂"},
    {.name = "laughing",   .emoji = "😆"},
    {.name = "funny",      .emoji = "😂"},
    {.name = "sad",        .emoji = "😔"},
    {.name = "angry",      .emoji = "😠"},
    {.name = "crying",     .emoji = "😭"},
    {.name = "loving",     .emoji = "😍"},
    {.name = "embarrassed",.emoji = "😳"},
    {.name = "surprised",  .emoji = "😲"},
    {.name = "shocked",    .emoji = "😱"},
    {.name = "thinking",   .emoji = "🤔"},
    {.name = "winking",    .emoji = "😉"},
    {.name = "cool",       .emoji = "😎"},
    {.name = "relaxed",    .emoji = "😌"},
    {.name = "delicious",  .emoji = "🤤"},
    {.name = "kissy",      .emoji = "😘"},
    {.name = "confident",  .emoji = "😏"},
    {.name = "sleepy",     .emoji = "😴"},
    {.name = "silly",      .emoji = "😜"},
    {.name = "confused",   .emoji = "🙄"},
};

void xiaozhi_display_init(void)
{
    /* 1. 先保证 LCD 本身已经通过 bsp 初始化：
       通常在 app_main 里先调用 bsp_board_lcd_init(board) */
    bsp_board_t *board = bsp_board_get_instance();

    /* 2. 初始化 LVGL 任务 */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority     = 4,
        .task_stack        = 6144,
        .task_affinity     = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms   = 5,
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    uint32_t buff_size = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT;   // 240 * 40

    ESP_LOGI(TAG, "Add LCD screen via SPI panel from bsp_board");

    /* 3. 通过 BSP 里的 lcd_io / lcd_panel 注册到 LVGL */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = board->lcd_io,
        .panel_handle  = board->lcd_panel,
        .buffer_size   = buff_size,
        .double_buffer = true,

        .hres          = BSP_LCD_H_RES,  // 240
        .vres          = BSP_LCD_V_RES,  // 320
        .monochrome    = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy  = false,
            .mirror_x = true,
            .mirror_y = true,
        },

        .flags = {
            .buff_dma    = true,
            .buff_spiram = true,   // 如果是 SPIRAM，则设置为 true
        },
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    /* 4. 创建默认 UI（欢迎光临 + 表情 + 文本） */
    lvgl_port_lock(0);

#if LVGL_VERSION_MAJOR >= 9
    lv_obj_t *screen = lv_screen_active();
#else
    lv_obj_t *screen = lv_scr_act();
#endif

    /* 背景色 */
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xF0F0F0), LV_PART_MAIN);//浅灰

    /* 4.1 顶部提示标签 */
    tip_label = lv_label_create(screen);
    lv_label_set_text(tip_label, "欢迎光临");
    lv_obj_set_style_width(tip_label, BSP_LCD_H_RES, LV_PART_MAIN);  // 240 宽
    lv_obj_align(tip_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(tip_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_align(tip_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(tip_label, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tip_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tip_label, LV_OPA_COVER, LV_PART_MAIN);
    
    /* 4.2 表情标签 */
    emoji_label = lv_label_create(screen);
    lv_label_set_text(emoji_label, "🙂");
    lv_obj_align(emoji_label, LV_ALIGN_CENTER, 0, -70);
    lv_obj_set_style_text_align(emoji_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(emoji_label, font_emoji_64_init(), LV_PART_MAIN);

    /* 4.3 中间文本标签 */
    text_label = lv_label_create(screen);
    lv_label_set_text(text_label, "STEFANIE");
    lv_obj_set_style_width(text_label, BSP_LCD_H_RES, LV_PART_MAIN);
    lv_obj_align_to(text_label, emoji_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_text_color(text_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(text_label, &font_puhui_20_4, LV_PART_MAIN);

    /* 4.4 电量图标 */
    battery_label = lv_label_create(tip_label);
    lv_label_set_text(battery_label, FONT_AWESOME_BATTERY_FULL);                      // 选择合适的电量图标（这里初始为满电）
    lv_obj_align(battery_label, LV_ALIGN_LEFT_MID, 10, 0);                            // 左侧对齐
    lv_obj_set_style_text_font(battery_label, &font_awesome_16_4, LV_PART_MAIN);      // 使用字体图标字体
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0x000000), LV_PART_MAIN); // 设置图标颜色

    /* 4.5 Wi-Fi图标 */
    wifi_label = lv_label_create(tip_label);
    lv_label_set_text(wifi_label, FONT_AWESOME_WIFI);                              // 初始Wi-Fi状态，显示强信号图标
    lv_obj_align(wifi_label, LV_ALIGN_RIGHT_MID, -10, 0);                          // 右侧对齐
    lv_obj_set_style_text_font(wifi_label, &font_awesome_16_4, LV_PART_MAIN);      // 使用字体图标字体
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0x000000), LV_PART_MAIN); // 设置图标颜色

    lvgl_port_unlock();
}

/* 修改顶部提示文字 */
void xiaozhi_display_tip(char *tip)
{
    if (!tip_label) return;
    lvgl_port_lock(0);
    lv_label_set_text(tip_label, tip);
    lvgl_port_unlock();
}

/* 修改中间说明文字 */
void xiaozhi_display_text(char *text)
{
    if (!text_label) return;
    lvgl_port_lock(0);
    lv_label_set_text(text_label, text);
    lvgl_port_unlock();
}

/* 修改表情 */
void xiaozhi_display_emoji(char *emoji_name)
{
    if (!emoji_label) return;

    lvgl_port_lock(0);
    bool found = false;
    for (uint8_t i = 0; i < 21; i++) {
        if (strcmp(emoji_list[i].name, emoji_name) == 0) {
            lv_label_set_text(emoji_label, emoji_list[i].emoji);
            found = true;
            break;
        }
    }
    if (!found) {
        lv_label_set_text(emoji_label, "😏");
    }
    lvgl_port_unlock();
}

/* 显示二维码 */
void xiaozhi_display_show_qrcode(char *data, uint32_t data_len)
{
    lvgl_port_lock(0);

    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);

    qr = lv_qrcode_create(lv_screen_active());
    lv_qrcode_set_size(qr, 220);
    lv_qrcode_set_dark_color(qr, fg_color);
    lv_qrcode_set_light_color(qr, bg_color);

    lv_qrcode_update(qr, data, data_len);
    lv_obj_center(qr);

    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);

    lvgl_port_unlock();
}

/* 删除二维码 */
void xiaozhi_display_delete_qrcode(void)
{
    if (qr == NULL) {
        return;
    }
    lvgl_port_lock(0);
    lv_obj_del(qr);
    qr = NULL;
    lvgl_port_unlock();
}
void xiaozhi_display_update_status(int battery_soc, int wifi_rssi)
{
    // 确保电量在0到100之间
    if (battery_soc > 100)
    {
        battery_soc = 100;
    }
    if (battery_soc < 0)
    {
        battery_soc = 0;
    }

    // 根据电量百分比选择电池图标
    static const char *battery_socs[] = {
        FONT_AWESOME_BATTERY_EMPTY,      // 0% ~ 19%
        FONT_AWESOME_BATTERY_QUARTER,    // 20% ~ 39%
        FONT_AWESOME_BATTERY_HALF,       // 40% ~ 59%
        FONT_AWESOME_BATTERY_THREE_QUARTERS, // 60% ~ 79%
        FONT_AWESOME_BATTERY_FULL,       // 80% ~ 100%
    };
    int index = battery_soc / 20; // 计算电池图标索引
    if (index>4)
    {
        index = 4;
    }
    
    const char *battery_icon = battery_socs[index];

    // 根据Wi-Fi信号强度选择Wi-Fi图标
    const char *wifi_icon;
    if (wifi_rssi >= 0)
    {
        wifi_icon = FONT_AWESOME_WIFI_SLASH;  // 无信号
    }
    else if (wifi_rssi < -70)
    {
        wifi_icon = FONT_AWESOME_WIFI_WEAK;   // 弱信号
    }
    else if (wifi_rssi < -50)
    {
        wifi_icon = FONT_AWESOME_WIFI_FAIR;   // 中等信号
    }
    else
    {
        wifi_icon = FONT_AWESOME_WIFI;        // 强信号
    }

    // 更新电池和Wi-Fi标签
    lvgl_port_lock(0);
    lv_label_set_text(battery_label, battery_icon);
    lv_label_set_text(wifi_label, wifi_icon);
    lvgl_port_unlock();
}
