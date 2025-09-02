#pragma once

#include <stddef.h>
#include <stdbool.h>

// dynamic array to store [data] of size [size]
typedef struct {
    void* data;
    size_t len;
    size_t cap;
    size_t _size; // size of an element. avoid changing
} buffer;

// allocate new. returns NULL on err.
buffer* buffer_new(size_t size);

// initialise. returns NULL on err.
void* buffer_init(buffer* buf, size_t size);

// free [buffer] and [data].
// NOTE: elements in [data] are not freed.
void buffer_destroy(buffer* buf);

// push [val] to end of [buf]. returns NULL on err.
void* buffer_push(buffer* buf, void** val);
// remove [val] frome end of [buf]. returns NULL on err.
void* buffer_pop(buffer* buf);

// returns pointer to [nth] elemnt of [buf]. if [nth] > [buf.len] returns NULL.
void* buffer_nth(buffer* buf, size_t nth);
