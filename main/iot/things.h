#pragma once

#include "methods.h"

typedef struct thing
{
    char *name;
    char *description;
    methods_t *methods;
    properties_t *properties;
} thing_t;

#define things_t my_list_t

thing_t *thing_create(void);
void thing_set(thing_t *thing,
               const char *name,
               const char *description,
               properties_t *properties,
               methods_t *methods);
void thing_destroy(thing_t *thing);

thing_t *things_get_by_name(things_t *things, const char *name);
cJSON *things_get_descriptor_json(things_t *things);
cJSON *things_get_state_json(things_t *things);
void things_invoke(things_t *things, cJSON *commands);