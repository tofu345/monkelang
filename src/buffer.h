#pragma once

#include "utils.h"

// From: wren DECLARE_BUFFER
#define BUFFER(name, typ)                                                     \
    typedef struct {                                                          \
        typ* data;                                                            \
        int length;                                                           \
        int capacity;                                                         \
    } name##Buffer;                                                           \
                                                                              \
    void name##Buffer##Init(name##Buffer* buf);                               \
    void name##BufferFill(name##Buffer* buf, typ val, int length);            \
    void name##BufferPush(name##Buffer* buf, typ val);                        \
    name##Buffer name##BufferCopy(name##Buffer* buf);                         \

// From: wren DEFINE_BUFFER
#define DEFINE_BUFFER(name, typ)                                              \
    void name##Buffer##Init(name##Buffer* buf) {                              \
        buf->data = NULL;                                                     \
        buf->length = 0;                                                      \
        buf->capacity = 0;                                                    \
    }                                                                         \
                                                                              \
    void name##BufferFill(name##Buffer* buf, typ val, int length) {           \
        if (buf->length + length >= buf->capacity) {                          \
            int capacity = power_of_2_ceil(buf->length + length);             \
            buf->data = reallocate(buf->data, capacity * sizeof(typ));        \
            buf->capacity = capacity;                                         \
        }                                                                     \
        for (int i = 0; i < length; i++) {                                    \
            buf->data[(buf->length)++] = val;                                 \
        }                                                                     \
    }                                                                         \
                                                                              \
    void name##BufferPush(name##Buffer* buf, typ val) {                       \
        name##BufferFill(buf, val, 1);                                        \
    }                                                                         \
                                                                              \
    name##Buffer name##BufferCopy(name##Buffer* buf) {                        \
         name##Buffer new_buf;                                                \
         if (buf->length == 0) {                                              \
            name##BufferInit(&new_buf);                                       \
            return new_buf;                                                   \
         }                                                                    \
         new_buf.data = allocate(buf->capacity * sizeof(typ));                \
         memcpy(new_buf.data, buf->data, buf->length * sizeof(typ));          \
         new_buf.length = buf->length;                                        \
         new_buf.capacity = buf->capacity;                                    \
         return new_buf;                                                      \
    }                                                                         \

