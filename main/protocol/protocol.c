#include "protocol.h"
#include "esp_websocket_client.h"
#include "object.h"
#include "esp_crt_bundle.h"
#include "bsp/bsp_board.h"
#include "esp_log.h"
#include <stdio.h>
#include "cJSON.h"       // 新增：用于解析服务器下行 JSON
#define TAG "Protocol"
#define protocol_send_text(protocol, fmtstr, ...)                                                 \
    do                                                                                            \
    {                                                                                             \
        if (!esp_websocket_client_is_connected(protocol->ws_client))                              \
        {                                                                                         \
            ESP_LOGE(TAG, "Websocket not connected");                                             \
            return;                                                                               \
        }                                                                                         \
        char *_text = NULL;                                                                       \
        asprintf(&_text, fmtstr, ##__VA_ARGS__);                                                  \
        esp_websocket_client_send_text(protocol->ws_client, _text, strlen(_text), portMAX_DELAY); \
        free(_text);                                                                              \
    } while (0)
 struct protocol
{
    esp_websocket_client_handle_t ws_client;
    char *session_id;

    esp_event_handler_t callback;
    void *callback_arg;
} ;
/* 小工具：统一分发事件给上层 */
static void protocol_dispatch_event(protocol_t *protocol,
                                    protocol_event_t event,
                                    void *event_data)
{
    if (protocol->callback) {
        // base 这里用 NULL，event_id 用我们自己的枚举
        protocol->callback(protocol->callback_arg, NULL, event, event_data);
    }
}
/* 处理文本帧(JSON)下行 */
static void protocol_handle_text_frame(protocol_t *protocol,
                                       const char *payload,
                                       size_t len)
{
    cJSON *root = cJSON_ParseWithLength(payload, len);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse websocket json");
        return;
    }

    cJSON *type_json = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type_json)) {
        cJSON_Delete(root);
        return;
    }

    const char *type = type_json->valuestring;

    // 1) hello: 解析 session_id
    if (strcmp(type, "hello") == 0) {
        cJSON *session_json = cJSON_GetObjectItem(root, "session_id");
        if (cJSON_IsString(session_json)) {
            free(protocol->session_id);
            protocol->session_id = strdup(session_json->valuestring);
            ESP_LOGI(TAG, "Session id set to: %s", protocol->session_id);
        }
        protocol_dispatch_event(protocol, PROTOCOL_EVENT_HELLO, NULL);
    }
    // 2) stt: 语音识别结果
    else if (strcmp(type, "stt") == 0) {
        cJSON *text_json = cJSON_GetObjectItem(root, "text");
        if (cJSON_IsString(text_json)) {
            char *text = strdup(text_json->valuestring);
            // 上层用完需要 free(text)
            protocol_dispatch_event(protocol, PROTOCOL_EVENT_STT, text);
        }
    }
    // 3) llm: 大模型文字回复
    else if (strcmp(type, "llm") == 0) {
        cJSON *text_json = cJSON_GetObjectItem(root, "text");
        if (cJSON_IsString(text_json)) {
            char *text = strdup(text_json->valuestring);
            // 上层用完需要 free(text)
            protocol_dispatch_event(protocol, PROTOCOL_EVENT_LLM, text);
        }
    }
    // 4) tts: 文本转语音控制
    else if (strcmp(type, "tts") == 0) {
        cJSON *state_json = cJSON_GetObjectItem(root, "state");
        if (cJSON_IsString(state_json)) {
            const char *state = state_json->valuestring;
            if (strcmp(state, "start") == 0) {
                protocol_dispatch_event(protocol, PROTOCOL_EVENT_TTS_START, NULL);
            } else if (strcmp(state, "sentence_start") == 0) {
                cJSON *text_json = cJSON_GetObjectItem(root, "text");
                if (cJSON_IsString(text_json)) {
                    char *text = strdup(text_json->valuestring);
                    // 上层用完需要 free(text)
                    protocol_dispatch_event(protocol, PROTOCOL_EVENT_TTS_SENTENCE_START, text);
                }
            } else if (strcmp(state, "stop") == 0) {
                protocol_dispatch_event(protocol, PROTOCOL_EVENT_TTS_STOP, NULL);
            }
        }
    }

    cJSON_Delete(root);
}
/* 处理二进制帧(音频)下行 */
static void protocol_handle_binary_frame(protocol_t *protocol,
                                         const void *payload,
                                         size_t len)
{
    binary_data_t *audio = malloc(sizeof(binary_data_t));
    if (!audio) {
        ESP_LOGE(TAG, "No memory for binary_data_t");
        return;
    }

    audio->data = malloc(len);
    if (!audio->data) {
        ESP_LOGE(TAG, "No memory for audio buffer");
        free(audio);
        return;
    }

    memcpy(audio->data, payload, len);
    audio->size = len;

    // 上层用完需要 free(audio->data) + free(audio)
    protocol_dispatch_event(protocol, PROTOCOL_EVENT_AUDIO, audio);
}
/* Websocket 事件回调：上下行核心 */
static void protocol_ws_event_handler(void *handler_args,
                                      esp_event_base_t base,
                                      int32_t event_id,
                                      void *event_data)
{
    protocol_t *protocol = (protocol_t *)handler_args;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Websocket connected");
        protocol_dispatch_event(protocol, PROTOCOL_EVENT_CONNECTED, NULL);
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Websocket disconnected");
        protocol_dispatch_event(protocol, PROTOCOL_EVENT_DISCONNECTED, NULL);
        break;

    case WEBSOCKET_EVENT_DATA: {
        esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
        // 文本帧 (JSON)
        if (data->op_code == WS_TRANSPORT_OPCODES_TEXT) {
            protocol_handle_text_frame(protocol, data->data_ptr, data->data_len);
        }
        // 二进制帧 (音频)
        else if (data->op_code == WS_TRANSPORT_OPCODES_BINARY) {
            protocol_handle_binary_frame(protocol, data->data_ptr, data->data_len);
        }
        break;
    }

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "Websocket error");
        // 看需要要不要通知上层，可以映射成 DISCONNECTED 之类
        break;

    default:
        break;
    }
}
protocol_t *protocol_create(const char *url, const char *token){
    // 创建protocol
    protocol_t *protocol = (protocol_t *)object_create(sizeof(protocol_t));
    bsp_board_t *board = bsp_board_get_instance();
      char *headers = NULL;
    asprintf(&headers, "Device-Id: %s\r\nClient-Id: %s\r\nAuthorization: Bearer %s\r\nProtocol-Version: 1\r\n",
             board->mac, board->uuid, token);
    // 配置websocket客户端
    esp_websocket_client_config_t ws_config = {
        .uri = url,
        .headers = headers,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    protocol->ws_client = esp_websocket_client_init(&ws_config);
    esp_websocket_register_events(protocol->ws_client, ESP_EVENT_ANY_ID, protocol_ws_event_handler, protocol);
    free(headers);
    return protocol;
}
void protocol_destroy(protocol_t *protocol)
{
    esp_websocket_client_destroy(protocol->ws_client);
    free(protocol->session_id);
    free(protocol);
}
void protocol_connect(protocol_t *protocol)
{
    if (esp_websocket_client_is_connected(protocol->ws_client))
    {
        ESP_LOGW(TAG, "Already connected");
        return;
    }
    esp_err_t ret = esp_websocket_client_start(protocol->ws_client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_websocket_client_start failed: %s", esp_err_to_name(ret));
    }
}
void protocol_disconnect(protocol_t *protocol)
{
    if (!esp_websocket_client_is_connected(protocol->ws_client))
    {
        ESP_LOGW(TAG, "Not connected");
        return;
    }
    esp_err_t ret = esp_websocket_client_stop(protocol->ws_client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_websocket_client_stop failed: %s", esp_err_to_name(ret));
    }
}
bool protocol_is_connected(protocol_t *protocol)
{
    return esp_websocket_client_is_connected(protocol->ws_client);
}
void protocol_send_hello(protocol_t *protocol)
{
    protocol_send_text(protocol, "{\"audio_params\":{\"channels\":%d,\"format\":\"opus\",\"frame_duration\":60,\"sample_rate\":%d},\"transport\":\"websocket\",\"type\":\"hello\",\"version\":1}", 1, CODEC_SAMPLE_RATE);
}
void protocol_send_wake_word(protocol_t *protocol, const char *wake_word)
{
    protocol_send_text(protocol, "{\"session_id\":\"%s\",\"state\":\"detect\",\"text\":\"%s\",\"type\":\"listen\"}", protocol->session_id, wake_word);
}
void protocol_send_start_listening(protocol_t *protocol, protocol_listening_type_t type)
{
    static const char *mode_str[] = {"auto", "manual", "realtime"};
    protocol_send_text(protocol, "{\"mode\":\"%s\",\"session_id\":\"%s\",\"state\":\"start\",\"type\":\"listen\"}", mode_str[type], protocol->session_id);
}
void protocol_send_stop_listening(protocol_t *protocol)
{
    protocol_send_text(protocol, "{\"session_id\":\"%s\",\"state\":\"stop\",\"type\":\"listen\"}", protocol->session_id);
}

void protocol_send_audio(protocol_t *protocol, binary_data_t *audio_data)
{
    if (!esp_websocket_client_is_connected(protocol->ws_client))
    {
        ESP_LOGE(TAG, "Websocket not connected");
        return;
    }
    esp_websocket_client_send_bin(protocol->ws_client, audio_data->data, audio_data->size, portMAX_DELAY);
}

void protocol_send_abort_speaking(protocol_t *protocol, protocol_abort_reason_t reason)
{
    static const char *reason_str[] = {"none", "wake_word_detected"};
    protocol_send_text(protocol, "{\"reason\":\"%s\",\"session_id\":\"%s\",\"type\":\"abort\"}", reason_str[reason], protocol->session_id);
}

void protocol_register_callback(protocol_t *protocol, esp_event_handler_t callback, void *arg)
{
    protocol->callback = callback;
    protocol->callback_arg = arg;
}