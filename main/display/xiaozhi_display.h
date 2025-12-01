#pragma once

#include <stdint.h>

void xiaozhi_display_init(void);

void xiaozhi_display_show_qrcode(char *data, uint32_t data_len);
void xiaozhi_display_delete_qrcode(void);

void xiaozhi_display_text(char *text);
void xiaozhi_display_emoji(char *emoji_name);
void xiaozhi_display_tip(char *tip);
void xiaozhi_display_update_status(int battery_soc, int wifi_rssi);