#include "properties.h"

property_t *property_create(void)
{
    return (property_t *)object_create(sizeof(property_t));
}

void property_set(property_t *property, char *name, char *description, property_type_t type, property_value_t value)
{
    property->name = object_create(strlen(name) + 1);
    strcpy(property->name, name);
    property->description=object_create(strlen(description)+1);
    strcpy(property->description,description);
    property->type=type;
    property->value=value;
}

void property_destroy(property_t *property)
{
    free(property->name);
    free(property->description);
    free(property);
}

property_t *properties_get_by_name(properties_t *properties, const char *name)
{
     my_list_node_t *current = NULL;
     my_list_foreach(properties,current){
        property_t *property = (property_t *)current->data;
        if (strcmp(property->name,name)==0)
        {
           return property;
        } 
     }
     return NULL;
}

cJSON *properties_get_descriptor_json(properties_t *properties)
{
     cJSON *root = cJSON_CreateObject();
     static const char *type_strs[] = {"boolean", "number", "string"};
      my_list_node_t *current = NULL;
      my_list_foreach(properties,current){
         property_t *property = (property_t *)current->data;
         cJSON *item= cJSON_CreateObject();
         cJSON_AddStringToObject(item,"description",property->description);
         cJSON_AddStringToObject(item,"type", type_strs[property->type]);
         cJSON_AddItemToObject(root,property->name,item);
      }
      return root;
}

cJSON *properties_get_state_json(properties_t *properties)
{
    
     cJSON *root = cJSON_CreateObject();
     
      my_list_node_t *current = NULL;
      my_list_foreach(properties,current){
         property_t *property = (property_t *)current->data;
        switch (property->type)
        {
        case  PROPERTY_TYPE_BOOLEAN:
           cJSON_AddBoolToObject(root,property->name,property->value.boolean);
            break;
        case    PROPERTY_TYPE_STRING:
             cJSON_AddStringToObject(root,property->name,property->value.string);
             break;
        case   PROPERTY_TYPE_NUMBER:
             cJSON_AddNumberToObject(root,property->name,property->value.number);
             break;
        default:
            break;
        }
      }
      return root;
}
