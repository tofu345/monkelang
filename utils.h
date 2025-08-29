#pragma once

#include <stddef.h>

#define START_CAPACITY 8

#define FPRINTF(fp, ...) \
    if (fprintf(fp, __VA_ARGS__) <= 0) \
        return -1;

// realloc `arr` to (`cap` * 2) * `size` and return new pointer
//
// one could argue that this method is unnecessary
void grow_array(void** arr, size_t* cap, size_t size);
