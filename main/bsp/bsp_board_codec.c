#include "bsp_board.h"
#include "esp_codec_dev_defaults.h"
#include "esp_codec_dev.h"
#include "driver/i2s_std.h"
#include "driver/i2c_master.h"
#include "driver/i2c.h"
static void bsp_board_codec_i2s_init(bsp_board_t *board,
                                     i2s_chan_handle_t *rx_handle_p,
                                     i2s_chan_handle_t *tx_handle_p)
{
   i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
   chan_cfg.auto_clear_after_cb = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, tx_handle_p, rx_handle_p));
   i2s_std_config_t std_cfg={
    .clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(CODEC_SAMPLE_RATE),
    .slot_cfg=I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(CODEC_BIT_WIDTH, I2S_SLOT_MODE_MONO),
    .gpio_cfg={
         .mclk = CODEC_MCK_PIN,
            .bclk = CODEC_BCK_PIN,
            .ws = CODEC_WS_PIN,
            .dout = CODEC_DOUT_PIN,
            .din = CODEC_DIN_PIN,
    }
   };
   ESP_ERROR_CHECK(i2s_channel_init_std_mode(*tx_handle_p, &std_cfg));
   ESP_ERROR_CHECK(i2s_channel_init_std_mode(*rx_handle_p, &std_cfg));
   ESP_ERROR_CHECK(i2s_channel_enable(*tx_handle_p));
   ESP_ERROR_CHECK(i2s_channel_enable(*rx_handle_p));

}
static void bsp_board_codec_i2c_init(bsp_board_t *board, i2c_master_bus_handle_t *bus_handle_p){
    i2c_master_bus_config_t i2c_cfg = {
    .i2c_port = I2C_NUM_0,
    .scl_io_num = CODEC_SCL_PIN,
    .sda_io_num = CODEC_SDA_PIN,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt=7,
    .flags.enable_internal_pullup = true,
};  
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_cfg, bus_handle_p));
}
void bsp_board_codec_init(bsp_board_t *board){
     i2c_master_bus_handle_t bus_handle = NULL;
    bsp_board_codec_i2c_init(board, &bus_handle);
     audio_codec_i2c_cfg_t i2c_cfg = {
        .bus_handle = bus_handle,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    es8311_codec_cfg_t es8311_cfg ={
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .pa_pin = CODEC_PA_PIN, 
        .use_mclk = true,
    };
    audio_codec_if_t *codec_if =es8311_codec_new(&es8311_cfg);
    i2s_chan_handle_t rx_handle = NULL;
    i2s_chan_handle_t tx_handle = NULL;
    bsp_board_codec_i2s_init(board, &rx_handle, &tx_handle);
    audio_codec_i2s_cfg_t i2s_cfg={
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
    };
    const audio_codec_data_if_t *data_if=audio_codec_new_i2s_data(&i2s_cfg);
    esp_codec_dev_cfg_t codec_cfg ={
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    board->codec_dev = esp_codec_dev_new(&codec_cfg);
    assert(board->codec_dev);
    xEventGroupSetBits(board->board_status, BSP_BOARD_CODEC_BIT);

}   
