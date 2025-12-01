#include "things.h"
#include "esp_log.h"

#define TAG "[IOT] Things"

thing_t *thing_create(void)
{
    return (thing_t *)object_create(sizeof(thing_t));
}

void thing_set(thing_t *thing, const char *name, const char *description, properties_t *properties, methods_t *methods)
{
    thing->name = object_create(strlen(name) + 1);
    strcpy(thing->name, name);
    thing->description = object_create(strlen(description) + 1);
    strcpy(thing->description, description);
    thing->properties = properties;
    thing->methods = methods;
}

void thing_destroy(thing_t *thing)
{
    free(thing->name);
    free(thing->description);
    my_list_destroy(thing->properties, (data_free_func_t)property_destroy);
    my_list_destroy(thing->methods, (data_free_func_t)method_destroy);
    free(thing);
}

thing_t *things_get_by_name(things_t *things, const char *name)
{
    my_list_node_t *current = NULL;
    my_list_foreach(things, current)
    {
        thing_t *thing = (thing_t *)current->data;
        if (strcmp(thing->name, name) == 0)
        {
            return thing;
        }
    }
    return NULL;
}

cJSON *things_get_descriptor_json(things_t *things)
{
    cJSON *root = cJSON_CreateArray();
    my_list_node_t *current = NULL;
    my_list_foreach(things, current)
    {
        thing_t *thing = (thing_t *)current->data;
        cJSON *thing_json = cJSON_CreateObject();
        cJSON_AddStringToObject(thing_json, "name", thing->name);
        cJSON_AddStringToObject(thing_json, "description", thing->description);
        cJSON_AddItemToObject(thing_json, "properties", properties_get_descriptor_json(thing->properties));
        cJSON_AddItemToObject(thing_json, "methods", methods_get_descriptor_json(thing->methods));

        cJSON_AddItemToArray(root, thing_json);
    }
    return root;
}

cJSON *things_get_state_json(things_t *things)
{
    cJSON *root = cJSON_CreateArray();
    my_list_node_t *current = NULL;
    my_list_foreach(things, current)
    {
        thing_t *thing = (thing_t *)current->data;
        cJSON *thing_json = cJSON_CreateObject();
        cJSON_AddStringToObject(thing_json, "name", thing->name);
        cJSON_AddItemToObject(thing_json, "state", properties_get_state_json(thing->properties));

        cJSON_AddItemToArray(root, thing_json);
    }
    return root;
}

void things_invoke(things_t *things, cJSON *commands)
{
    cJSON *command = NULL;
    cJSON_ArrayForEach(command, commands)
    {
        cJSON *thing_name_json = cJSON_GetObjectItem(command, "name");
        if (!cJSON_IsString(thing_name_json))
        {
            ESP_LOGE(TAG, "No thing name");
            return;
        }
        thing_t *thing = things_get_by_name(things, thing_name_json->valuestring);
        if (!thing)
        {
            ESP_LOGE(TAG, "No thing name %s found", thing_name_json->valuestring);
            return;
        }

        methods_invoke(thing->methods, command);
    }
}
