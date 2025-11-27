#include "bsp_board.h"
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_random.h"
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
    
    // 获取UUID
    nvs_handle_t nvs_handle = 0;
    ESP_ERROR_CHECK(nvs_open("settings", NVS_READWRITE, &nvs_handle));

    // 尝试读取uuid
    size_t uuid_len = sizeof(board->uuid);
    ret = nvs_get_str(nvs_handle, "uuid", board->uuid, &uuid_len);
    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
        // 生成新的uuid
        unsigned char bytes[16];

        // 生成16个随机字节
        esp_fill_random(bytes, 16);

        // 设置UUID版本 (第6-7位为0100)
        bytes[6] = (bytes[6] & 0x0F) | 0x40;

        // 设置变体 (第8-9位为10xx)
        bytes[8] = (bytes[8] & 0x3F) | 0x80;

        // 格式化为UUID字符串
        snprintf(board->uuid, 37,
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 bytes[0], bytes[1], bytes[2], bytes[3],
                 bytes[4], bytes[5], bytes[6], bytes[7],
                 bytes[8], bytes[9], bytes[10], bytes[11],
                 bytes[12], bytes[13], bytes[14], bytes[15]);

        // 将uuid保存到nvs
        ret = nvs_set_str(nvs_handle, "uuid", board->uuid);
    }
    ESP_ERROR_CHECK(ret);
    nvs_close(nvs_handle);

}
bool bsp_board_check_status(bsp_board_t *board, EventBits_t status_bit, uint32_t timeout_ms)
{
    EventBits_t ret = xEventGroupWaitBits(board->board_status, status_bit, pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return (ret & status_bit) == status_bit;
}