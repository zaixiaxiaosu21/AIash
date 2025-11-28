#pragma once
#pragma once

#include "audio_sr.h"
#include "audio_encoder.h"
#include "audio_decoder.h"

typedef struct audio_processor audio_processor_t;

audio_processor_t *audio_processor_create(void);

void audio_processor_destroy(audio_processor_t *processor);

void audio_processor_start(audio_processor_t *processor);

void audio_processor_stop(audio_processor_t *processor);

/**
 * @brief 从processor中读取数据, opus编码后的语音录音
 *
 * @param processor 处理器
 * @param buffer 缓存区
 * @param size 缓存区大小
 * @return size_t 读取的字节数
 */
size_t audio_processor_read(audio_processor_t *processor, void *buffer, size_t size);

/**
 * @brief 向processor中写入数据, 音频数据
 *
 * @param processor 处理器
 * @param buffer 数据缓存区
 * @param size 数据大小
 */
void audio_processor_write(audio_processor_t *processor, const void *buffer, size_t size);

void audio_processor_register_callback(audio_processor_t *processor, audio_sr_event_t event, esp_event_handler_t callback, void *arg);
void audio_processor_set_vad_state(audio_processor_t *processor, bool state);