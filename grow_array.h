#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <stddef.h>

#define START_CAPACITY 8

// realloc `arr` to (`cap` * 2) * `size` and return new pointer
//
// one could argue that this method is unnecessary
void grow_array(void** arr, size_t* cap, size_t size);

#endif
