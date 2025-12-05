#include "light_thing.h"
#include "bsp/bsp_board.h"
#include "esp_log.h"
#include "led_strip.h"
#include "protocol/protocol.h"
#include "application.h"

#define TAG "[IOT] Light"


static void light_thing_set_toggle_callback(void *arg, properties_t *parameters)
{
    // 根据参数设置实际的开关
    property_t *toggle_parameter = properties_get_by_name(parameters, "toggle");
    if (!toggle_parameter || toggle_parameter->type != PROPERTY_TYPE_BOOLEAN)
    {
        ESP_LOGE(TAG, "Invalid toggle parameter");
        return;
    }
    bsp_board_t *board = bsp_board_get_instance();
    ESP_LOGI(TAG, "Set toggle to %s", toggle_parameter->value.boolean ? "true" : "false");
    // 控制灯带
    if(toggle_parameter->value.boolean){
        // 打开灯
        led_strip_set_pixel(board->led_strip, 0, 255, 255, 255); // 白色
        led_strip_set_pixel(board->led_strip, 1, 0, 255, 0); // 绿色
        led_strip_refresh(board->led_strip );
    } else {
        // 关闭灯
        led_strip_clear(board->led_strip);
    }
    // 更新light_thing状态
    thing_t *thing = (thing_t *)arg;
    property_t *toggle_property = properties_get_by_name(thing->properties, "toggle");
    toggle_property->value = toggle_parameter->value;
    // 通知协议层更新状态
      if (s_app.protocol && protocol_is_connected(s_app.protocol))
    {
        cJSON *state_json = things_get_state_json(s_app.things);
        if (state_json)
        {
            protocol_send_iot(s_app.protocol, PROTOCOL_IOT_TYPE_STATE, state_json);
            cJSON_Delete(state_json);
            ESP_LOGI(TAG, "Light state synchronized to cloud");
        }
    }
    
}

static properties_t *light_properties_create(thing_t *light_thing)
{
    properties_t *properties = my_list_create();

    // 开关属性
    property_t *toggle_property = property_create();
    property_set(toggle_property, "toggle", "Toggle status", PROPERTY_TYPE_BOOLEAN, (property_value_t){.boolean = false});
    my_list_add(properties, toggle_property);

   
    return properties;
}

static methods_t *light_methods_create(thing_t *light_thing)
{
    methods_t *methods = my_list_create();

    // 创建设置toggle的method
    method_t *set_toggle_method = method_create();

    // 创建参数列表
    properties_t *set_toggle_parameters = my_list_create();
    property_t *toggle_parameter = property_create();
    property_set(toggle_parameter, "toggle", "Toggle status", PROPERTY_TYPE_BOOLEAN, (property_value_t){.boolean = false});
    my_list_add(set_toggle_parameters, toggle_parameter);

    method_set(set_toggle_method,
               "SetToggle",
               "Set toggle status",
               set_toggle_parameters,
               light_thing_set_toggle_callback,
               light_thing);

    my_list_add(methods, set_toggle_method);

    

    return methods;
}

thing_t *light_thing_create(void)
{
    thing_t *light_thing = thing_create();
    // 创建light属性列表
    properties_t *light_properties = light_properties_create(light_thing);

    // 创建light方法列表
    methods_t *light_methods = light_methods_create(light_thing);

    thing_set(light_thing, "Light", "Light", light_properties, light_methods);

    return light_thing;
}