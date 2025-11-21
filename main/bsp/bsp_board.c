#include "bsp_board.h"
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
bsp_board_t *s_bsp_board_instance = NULL;
bsp_board_t *bsp_board_get_instance()
{
     if (s_bsp_board_instance == NULL)
     {   //初始化
         s_bsp_board_instance = (bsp_board_t *)malloc(sizeof(bsp_board_t));
         memset(s_bsp_board_instance, 0, sizeof(bsp_board_t));
         // 创建事件组
         s_bsp_board_instance->board_status = xEventGroupCreate();
     }
     
     return s_bsp_board_instance;
     
    
   
     
}
void bsp_board_nvs_init(bsp_board_t *board)
{
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS分区有问题，进行擦除并重新初始化
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // 设置NVS初始化完成标志
    xEventGroupSetBits(board->board_status, BSP_BOARD_NVS_BIT);

}
bool bsp_board_check_status(bsp_board_t *board, EventBits_t status_bit, uint32_t timeout_ms)
{
    EventBits_t ret = xEventGroupWaitBits(board->board_status, status_bit, pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return (ret & status_bit) == status_bit;
}