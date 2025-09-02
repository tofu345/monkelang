#include "buffer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* buffer_init(buffer* buf, size_t size) {
    buf->data = malloc(DEFAULT_CAPACITY * size);
    if (buf->data == NULL) return NULL;
    buf->len = 0;
    buf->cap = DEFAULT_CAPACITY;
    buf->_size = size;
    return buf;
}

buffer* buffer_new(size_t size) {
    buffer* buf = malloc(sizeof(buffer));
    if (buf == NULL) return NULL;
    return buffer_init(buf, size);
}

void buffer_destroy(buffer* buf) {
    free(buf->data);
}

void* buffer_push(buffer* buf, void** val) {
    if (buf->len >= buf->cap) {
        size_t new_cap  = buf->cap * 2;
        void** new_data =
            realloc(buf->data, new_cap * buf->_size);
        if (new_data == NULL)
            return NULL;

        buf->cap = new_cap;
        buf->data = new_data;
    }
    // copy [val] of length [size] into (buf->data + buf->len - 1) byte.
    // cast to (char*) to increment pointer by 1 byte and manually define offset.
    // https://stackoverflow.com/questions/16578054/how-to-make-a-pointer-increment-by-1-byte-not-1-unit.
    void* ptr = (char*)buf->data + (buf->len++) * buf->_size;
    memcpy(ptr, val, buf->_size);
    return val;
}

void* buffer_pop(buffer* buf) {
    if (buf->len == 0) return NULL;
    // retrieve value at buf->data[buf->len - 1]
    return (char*)buf->data + --(buf->len) * buf->_size;
}

void* buffer_nth(buffer* buf, size_t nth) {
    if (nth > buf->len) return NULL;
    return (char*)buf->data + nth * buf->_size;
}
