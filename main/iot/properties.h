#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "mylist.h"
#include "cJSON.h"

typedef enum{
    PROPERTY_TYPE_BOOLEAN,
    PROPERTY_TYPE_NUMBER,
    PROPERTY_TYPE_STRING,
    
}property_type_t;
typedef union{
    bool boolean;
    int number;
    char * string;
}property_value_t;

typedef struct{
    char *name;
    char *description;
    property_type_t type;
    property_value_t value;
}property_t;
#define properties_t my_list_t
property_t *property_create(void);
void property_set(property_t *property, char *name,
    char *desdescription,
    property_type_t type,
    property_value_t value);
void property_destroy(property_t *property);

property_t* properties_get_by_name(properties_t *properties, const char *name);

cJSON *properties_get_descriptor_json(properties_t *properties);
cJSON *properties_get_state_json(properties_t *properties);
