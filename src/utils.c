#include "utils.h"

#include <stddef.h>
#include <stdlib.h>

inline void* allocate(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) ALLOC_FAIL();
    return ptr;
}

inline void* reallocate(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) ALLOC_FAIL();
    return new_ptr;
}

int power_of_2_ceil(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}
