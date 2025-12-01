#include "bsp_board.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "qrcode.h"
#define TAG "[BSP] WiFi"
#define SECURITY_KEY "abcd1234"

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
   
}  else if(event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_CRED_FAIL){
        ESP_LOGE(TAG, "WiFi provisioning failed");
        wifi_prov_mgr_reset_sm_state_on_failure();
}  else if(event_base == WIFI_PROV_EVENT && event_id ==WIFI_PROV_END ){
        wifi_prov_mgr_deinit();
       
}
}

void bsp_board_wifi_init(bsp_board_t *board,char *payload, size_t payload_size){
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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_PROV_EVENT,
         ESP_EVENT_ANY_ID, event_handler, board, NULL));
    //创建wifitask
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    uint8_t eth_mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, eth_mac);
    sprintf(board->mac, "%02x:%02x:%02x:%02x:%02x:%02x", eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
      // 初始化配网manager
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
     // 检查是否已经配网
    bool provisioned = false;
    //wifi_prov_mgr_reset_provisioning();
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned)
    {
         // 没有配网，启动配网
        // 通过mac地址获取默认的服务名称
        uint8_t eth_mac[6];
        char service_name[15];
        esp_wifi_get_mac(ESP_IF_WIFI_STA, eth_mac);
        snprintf(service_name, sizeof(service_name), "PROV_%02X%02X%02X", eth_mac[3], eth_mac[4], eth_mac[5]); // 将mac地址的最后三个字节作为服务名称
        wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, SECURITY_KEY, service_name, NULL);
        //获取二维码字符串
        
        snprintf(payload, payload_size, "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}", service_name, SECURITY_KEY);

        ESP_LOGI(TAG, "https://espressif.github.io/esp-jumpstart/qrcode.html?data=%s", payload);
        //生成qrcode
        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
        //控制台显示二维码
        esp_qrcode_generate(&cfg, payload);
        

        }  else{
            ESP_LOGI(TAG, "Device already provisioned, starting WiFi STA");
            // 配置WiFi
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

            // 启动wifi
            ESP_ERROR_CHECK(esp_wifi_start());
        }

}
int bsp_board_wifi_get_rssi(bsp_board_t *board){
    //获取wifi信号强度
    int rssi =0;
   esp_err_t res= esp_wifi_sta_get_rssi(&rssi);
    if (res != ESP_OK){
         ESP_LOGE(TAG, "Failed to get RSSI: %s", esp_err_to_name(res));
         return 0;
    }
   return rssi;
}

