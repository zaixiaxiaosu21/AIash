#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_event.h"

typedef struct protocol protocol_t;

typedef enum
{
    PROTOCOL_EVENT_CONNECTED,          // event_data为NULL
    PROTOCOL_EVENT_DISCONNECTED,       // event_data为NULL
    PROTOCOL_EVENT_HELLO,              // event_data为NULL
    PROTOCOL_EVENT_STT,                // event_data为char*
    PROTOCOL_EVENT_LLM,                // event_data为char*
    PROTOCOL_EVENT_TTS_START,          // event_data为NULL
    PROTOCOL_EVENT_TTS_SENTENCE_START, // event_data为char*
    PROTOCOL_EVENT_TTS_STOP,           // event_data为NULL
    PROTOCOL_EVENT_AUDIO,              // event_data为binary_data_t*
} protocol_event_t;

typedef enum
{
    LISTENING_TYPE_AUTO,
    LISTENING_TYPE_MANUAL,
    LISTENING_TYPE_REALTIME,
} protocol_listening_type_t;

typedef enum
{
    ABORT_REASON_NONE,
    ABORT_REASON_WAKE_WORD,
} protocol_abort_reason_t;

typedef struct
{
    const void *data;
    size_t size;
} binary_data_t;

// protocol的构建和清理
protocol_t *protocol_create(const char *url, const char *token);
void protocol_destroy(protocol_t *protocol);

// protocol底层的websocket连接与断开
void protocol_connect(protocol_t *protocol);
void protocol_disconnect(protocol_t *protocol);
bool protocol_is_connected(protocol_t *protocol);

// protocol可以向服务器发送的协议
void protocol_send_hello(protocol_t *protocol);
void protocol_send_wake_word(protocol_t *protocol, const char *wake_word);
void protocol_send_start_listening(protocol_t *protocol, protocol_listening_type_t type);
void protocol_send_stop_listening(protocol_t *protocol);
void protocol_send_audio(protocol_t *protocol, binary_data_t *audio_data);
void protocol_send_abort_speaking(protocol_t *protocol, protocol_abort_reason_t reason);

// 注册服务器回复处理函数
void protocol_register_callback(protocol_t *protocol, esp_event_handler_t callback, void *arg);
