#pragma once
#include <stddef.h>
#include "esp_heap_caps.h"
#include "object.h"
typedef void (*data_free_func_t)(void *);
typedef struct my_list_node {
    struct my_list_node* next;
    void* data;
} my_list_node_t;
typedef struct my_list {
    my_list_node_t* head;
    my_list_node_t* tail;
    size_t size;
} my_list_t;
static inline my_list_t *my_list_create(void) {
   return   (my_list_t*) object_create(sizeof(my_list_t));
}
static inline void my_list_destroy(my_list_t* list,data_free_func_t data_free_func) {
      my_list_node_t *current = list->head;
      while (current ) {
          my_list_node_t *next = current->next;
          if (data_free_func)
          {
            data_free_func(current->data);
          }
          free(current);
          current = next;
      }
        free(list);//释放整个链表结构体
}
static inline void my_list_add(my_list_t *list, void *data) {
    my_list_node_t *node = (my_list_node_t *) malloc(sizeof(my_list_node_t));
    node->data = data;
    node->next = NULL; 
    if (list->head==NULL)
    {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->size++;
    
}
#define my_list_foreach(list, node) \
    for (node = list->head; node != NULL; node = node->next)
    