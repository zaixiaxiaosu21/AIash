#include "audio_process.h"
#include "bsp/bsp_board.h"
#include "object.h"
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"

#define TAG "Audio Processor"

#define PLAY_TASK_STACK_SIZE 4096
#define PLAY_TASK_CORE_ID 0
#define PLAY_TASK_PRIORITY 5
struct audio_processor
{
    bool is_running;

    audio_encoder_t *encoder;
    audio_decoder_t *decoder;
    audio_sr_t *sr;

    RingbufHandle_t enc_input;
    RingbufHandle_t enc_output;
    RingbufHandle_t dec_input;
    RingbufHandle_t dec_output;
};
void audio_output_task(void *arg){
    audio_processor_t *processor = (audio_processor_t *)arg;
    bsp_board_t *board = bsp_board_get_instance();
    while(processor->is_running){
         size_t size = 0;
        void*buffer=  xRingbufferReceiveUpTo(processor->dec_output,&size, pdMS_TO_TICKS(10),2048);
        if(buffer==NULL){
            continue;
        }
        esp_codec_dev_write(board->codec_dev, buffer, size);
        vRingbufferReturnItem(processor->dec_output, buffer);
    }
}
audio_processor_t *audio_processor_create(void){
    audio_processor_t *processor = (audio_processor_t *)object_create(sizeof(audio_processor_t));

    processor->enc_input = xRingbufferCreateWithCaps(16384, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
    processor->enc_output = xRingbufferCreateWithCaps(4096, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    processor->dec_input = xRingbufferCreateWithCaps(4096, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    processor->dec_output = xRingbufferCreateWithCaps(16384*2.5, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);

    processor->encoder = audio_encoder_create(processor->enc_input, processor->enc_output, CODEC_SAMPLE_RATE, CODEC_BIT_WIDTH, 1);
    processor->decoder = audio_decoder_create(processor->dec_input, processor->dec_output, CODEC_SAMPLE_RATE, 2);
    processor->sr = audio_sr_create(processor->enc_input);

    return processor;
}
void audio_processor_destroy(audio_processor_t *processor){
    audio_encoder_destroy(processor->encoder);
    audio_decoder_destroy(processor->decoder);
    audio_sr_destroy(processor->sr);

    vRingbufferDelete(processor->enc_input);
    vRingbufferDelete(processor->enc_output);
    vRingbufferDelete(processor->dec_input);
    vRingbufferDelete(processor->dec_output);

    free(processor);
}
void audio_processor_start(audio_processor_t *processor){
     processor->is_running = true;
     audio_encoder_start(processor->encoder);
     audio_decoder_start(processor->decoder);
     audio_sr_start(processor->sr);
     xTaskCreatePinnedToCoreWithCaps(audio_output_task, "play_task",
                                    PLAY_TASK_STACK_SIZE, processor,
                                    6, NULL,
                                    PLAY_TASK_CORE_ID, MALLOC_CAP_SPIRAM);
}
void audio_processor_stop(audio_processor_t *processor){
    processor->is_running = false;
    audio_encoder_stop(processor->encoder);
    audio_decoder_stop(processor->decoder);
    audio_sr_stop(processor->sr);
}
size_t audio_processor_read(audio_processor_t *processor, void *buffer, size_t size){
    size_t read_size = 0;
    void* data = xRingbufferReceive(processor->enc_output, &read_size, portMAX_DELAY);
    memcpy(buffer, data, read_size);
    vRingbufferReturnItem(processor->enc_output, data);
    return read_size;
}
void audio_processor_write(audio_processor_t *processor, void *buffer, size_t size){
    xRingbufferSend(processor->dec_input, buffer, size, portMAX_DELAY);
}
void audio_processor_register_callback(audio_processor_t *processor, audio_sr_event_t event, esp_event_handler_t callback, void *arg){
    audio_sr_register_callback(processor->sr, event, callback, arg);
}
    
   