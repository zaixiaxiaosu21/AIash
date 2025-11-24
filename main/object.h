#pragma once
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "esp_heap_caps.h"

static inline void *object_create(size_t size)
{
    void *obj = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!obj)
    {
        return NULL;
    }

    memset(obj, 0, size);
    return obj;
}