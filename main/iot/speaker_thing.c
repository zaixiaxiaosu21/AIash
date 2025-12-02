#include "speaker_thing.h"
#include "bsp/bsp_board.h"
#include "esp_log.h"

#define TAG "[IOT] Speaker"

static void speaker_thing_set_volume_callback(void *arg, properties_t *parameters)
{
    // 根据参数设置实际的音量
    property_t *volume_parameter = properties_get_by_name(parameters, "volume");
    if (!volume_parameter || volume_parameter->type != PROPERTY_TYPE_NUMBER)
    {
        ESP_LOGE(TAG, "Invalid volume parameter");
        return;
    }
    bsp_board_t *board = bsp_board_get_instance();
    ESP_LOGI(TAG, "Set volume to %d", volume_parameter->value.number);

    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(board->codec_dev, volume_parameter->value.number));

    // 更新speaker_thing状态
    thing_t *thing = (thing_t *)arg;
    property_t *volume_property = properties_get_by_name(thing->properties, "volume");
    volume_property->value = volume_parameter->value;
}

static void speaker_thing_set_mute_callback(void *arg, properties_t *parameters)
{
    // 根据参数设置实际的静音
    property_t *mute_parameter = properties_get_by_name(parameters, "mute");
    if (!mute_parameter || mute_parameter->type != PROPERTY_TYPE_BOOLEAN)
    {
        ESP_LOGE(TAG, "Invalid mute parameter");
        return;
    }
    bsp_board_t *board = bsp_board_get_instance();
    ESP_LOGI(TAG, "Set mute to %s", mute_parameter->value.boolean ? "true" : "false");
    ESP_ERROR_CHECK(esp_codec_dev_set_out_mute(board->codec_dev, mute_parameter->value.boolean));

    // 更新speaker_thing状态
    thing_t *thing = (thing_t *)arg;
    property_t *mute_property = properties_get_by_name(thing->properties, "mute");
    mute_property->value = mute_parameter->value;
}

static properties_t *speaker_properties_create(thing_t *speaker_thing)
{
    properties_t *properties = my_list_create();

    // 静音属性
    property_t *mute_property = property_create();
    property_set(mute_property, "mute", "Mute status", PROPERTY_TYPE_BOOLEAN, (property_value_t){.boolean = false});
    my_list_add(properties, mute_property);

    // 音量属性
    property_t *volume_property = property_create();
    property_set(volume_property, "volume", "Volume level[0-100]", PROPERTY_TYPE_NUMBER, (property_value_t){.number = 60});
    my_list_add(properties, volume_property);

    return properties;
}

static methods_t *speaker_methods_create(thing_t *speaker_thing)
{
    methods_t *methods = my_list_create();

    // 创建设置音量的method
    method_t *set_volume_method = method_create();

    // 创建参数列表
    properties_t *set_volume_parameters = my_list_create();
    property_t *volume_parameter = property_create();
    property_set(volume_parameter, "volume", "Volume level[0-100]", PROPERTY_TYPE_NUMBER, (property_value_t){.number = 60});
    my_list_add(set_volume_parameters, volume_parameter);

    method_set(set_volume_method,
               "SetVolume",
               "Set volume level[0-100]",
               set_volume_parameters,
               speaker_thing_set_volume_callback,
               speaker_thing);

    my_list_add(methods, set_volume_method);

    // 创建设置静音的method
    method_t *set_mute_method = method_create();

    // 创建参数列表
    properties_t *set_mute_parameters = my_list_create();
    property_t *mute_parameter = property_create();
    property_set(mute_parameter, "mute", "Mute status", PROPERTY_TYPE_BOOLEAN, (property_value_t){.boolean = false});
    my_list_add(set_mute_parameters, mute_parameter);

    method_set(set_mute_method,
               "SetMute",
               "Set mute status",
               set_mute_parameters,
               speaker_thing_set_mute_callback,
               speaker_thing);

    my_list_add(methods, set_mute_method);

    return methods;
}

thing_t *speaker_thing_create(void)
{
    thing_t *speaker_thing = thing_create();
    // 创建speaker属性列表
    properties_t *speaker_properties = speaker_properties_create(speaker_thing);

    // 创建speaker方法列表
    methods_t *speaker_methods = speaker_methods_create(speaker_thing);

    thing_set(speaker_thing, "Speaker", "Speaker", speaker_properties, speaker_methods);

    return speaker_thing;
}