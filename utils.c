#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

void grow_array(void** arr, size_t* cap, size_t size) {
    *cap = *cap * 2;
    *arr = realloc(*arr, (*cap) * size);
    if (*arr == NULL) {
        fprintf(stderr, "no memory");
        exit(1);
    }
}
