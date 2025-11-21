#include "bsp_board.h"
#include "esp_log.h"
#include "esp_wifi.h"

#define TAG "[BSP] WiFi"
static void event_handler(void *event_handler_arg,
                                         esp_event_base_t event_base,
                                         int32_t event_id,
                                         void *event_data)
{
     bsp_board_t *board = (bsp_board_t *)event_handler_arg;
    if (event_base == WIFI_EVENT  && event_id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi started, connecting to AP...");     
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi disconnected, reconnecting to AP...");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        //设置WiFi连接成功标志位
        xEventGroupSetBits(board->board_status, BSP_BOARD_WIFI_BIT);
}
}

void bsp_board_wifi_init(bsp_board_t *board){
  bool res = bsp_board_check_status(board, BSP_BOARD_NVS_BIT, portMAX_DELAY);
    if (!res)
    {
        ESP_LOGE("bsp_board_wifi_init", "NVS not initialized, cannot init WiFi");
        return;
    }
    //tcpip
    ESP_ERROR_CHECK(esp_netif_init());
    //创建eventtask
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    //注册wifi和ip事件回调
     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        event_handler,
                                                        board,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        event_handler,
                                                        board,
                                                        NULL)) ;

    //创建wifitask
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //设置wifi工作模式
     wifi_config_t wifi_config = {
        .sta = {
            .ssid = "iphone",
            .password = "stefanie1.",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    //启动wifi
    ESP_ERROR_CHECK(esp_wifi_start() );





}