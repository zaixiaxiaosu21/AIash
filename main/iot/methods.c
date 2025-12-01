#include "methods.h"
#include "esp_log.h"

#define TAG "[IOT] Methods"
method_t *method_create(void)
{
     return (method_t *)object_create(sizeof(method_t));
}

void method_set(method_t *method, const char *name, const char *description, properties_t *parameters, method_call_back callback, void *arg)
{
    method->name = object_create(strlen(name) + 1);
    strcpy(method->name, name);
    method->description=object_create(strlen(description)+1);
    strcpy(method->description,description);
    method->parameters = parameters;
    method->callback = callback;
    method->arg = arg;
}

void method_destroy(method_t *method)
{
    free(method->name);
    free(method->description);
    my_list_destroy(method->parameters, (data_free_func_t)property_destroy);
    free(method);
}

method_t *methods_get_by_name(methods_t *methods, const char *name)
{
    my_list_node_t *current = NULL;
     my_list_foreach(methods,current){
        method_t *method = (method_t *)current->data;
        if (strcmp(method->name,name)==0)
        {
           return method;
        } 
     }
     return NULL;
}

cJSON *methods_get_descriptor_json(methods_t *methods)
{
   cJSON *root = cJSON_CreateObject();
      my_list_node_t *current = NULL;
      my_list_foreach(methods,current){
         method_t *method = (method_t *)current->data;
         cJSON *item= cJSON_CreateObject();
         cJSON_AddStringToObject(item,"description",method->description);
         cJSON_AddItemToObject(item,"parameters", properties_get_descriptor_json(method->parameters));
         cJSON_AddItemToObject(root,method->name,item);
      }
      return root;
}

void methods_invoke(methods_t *methods, cJSON *command)
{      cJSON *method_name_json = cJSON_GetObjectItem(command, "method");
    if (!cJSON_IsString(method_name_json))
    {
        ESP_LOGE(TAG, "method name is not a string");
        return;
    }

    cJSON *parameters_json = cJSON_GetObjectItem(command, "parameters");
    if (!parameters_json)
    {
        ESP_LOGE(TAG, "parameters is not a object");
        return;
    }
    //get method by name
    method_t *method = methods_get_by_name(methods, method_name_json->valuestring);
    my_list_node_t *current = NULL;
    my_list_foreach(method->parameters, current)
    {
        property_t *parameter = (property_t *)current->data;
        cJSON *parameter_json = cJSON_GetObjectItem(parameters_json, parameter->name);
        if (!parameter_json)
        {
            ESP_LOGE(TAG, "parameter %s is not found", parameter->name);
            return;
        }

        switch (parameter->type)
        {
        case PROPERTY_TYPE_BOOLEAN:
            parameter->value.boolean = cJSON_IsTrue(parameter_json);
            break;
        case PROPERTY_TYPE_NUMBER:
            parameter->value.number = parameter_json->valueint;
            break;
        case PROPERTY_TYPE_STRING:
            parameter->value.string = parameter_json->valuestring;
            break;
        default:
            break;
        }
    }

    // 调用回调函数
    method->callback(method->arg, method->parameters);
       
}
