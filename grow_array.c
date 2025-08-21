#include "grow_array.h"

#include <stdio.h>
#include <stdlib.h>

void* grow_array(void*** arr, size_t* len, size_t* cap, size_t size) {
    if (*len < *cap)
        return arr;

    size_t new_cap = *cap * 2;
    void* new_arr = realloc(*arr, new_cap * size);
    if (new_arr == NULL)
        return NULL;

    *cap = new_cap;
    *arr = new_arr;
    return new_arr;
}
