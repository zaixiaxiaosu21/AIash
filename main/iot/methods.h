#pragma once

#include "properties.h"

typedef  void(*method_call_back)(void *arg,properties_t *paramerters);   
typedef struct method{
    char *name;
    char *description;
    properties_t *parameters;
    method_call_back callback;
    void * arg;
}method_t;
#define methods_t my_list_t
method_t *method_create(void);

void method_set(method_t *method,
                const char *name,
                const char *description,
                properties_t *parameters,
                method_call_back callback,
                void *arg);
void method_destroy(method_t *method);
method_t *methods_get_by_name(methods_t *methods, const char *name);
cJSON *methods_get_descriptor_json(methods_t *methods);
void methods_invoke(methods_t *methods, cJSON *command);
