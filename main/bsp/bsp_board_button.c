#include "bsp_board.h"
#include "sdkconfig.h"
#include "iot_button.h"
#include  "button_adc.h"

void bsp_board_button_init(bsp_board_t *board){
    
    const button_config_t btn_cfg = {0};
    button_adc_config_t btn_adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .adc_channel = ADC_CHANNEL_7, // GPIO8 ✅
        .min= 0,
        .max=20,
    };
   ESP_ERROR_CHECK(iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &board->front_button));
   
   btn_adc_cfg.button_index = 1;
   btn_adc_cfg.min = 1600;   
   btn_adc_cfg.max =1800;
   ESP_ERROR_CHECK(iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &board->back_button));
   //设置标志位
    xEventGroupSetBits(board->board_status, BSP_BOARD_BUTTON_BIT);

}
