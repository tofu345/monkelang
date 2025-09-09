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
    void name##BufferPush(name##Buffer* buf, typ data);                       \
    typ name##BufferPop(name##Buffer* buf);                                   \
    name##Buffer name##BufferCopy(name##Buffer* buf);                         \

// From: wren DEFINE_BUFFER
#define DEFINE_BUFFER(name, typ)                                              \
    void name##Buffer##Init(name##Buffer* buf) {                              \
        buf->length = 0;                                                      \
        buf->data = NULL;                                                     \
        buf->capacity = 0;                                                    \
    }                                                                         \
                                                                              \
    void name##BufferPush(name##Buffer* buf, typ val) {                       \
        if (buf->length >= buf->capacity) {                                   \
            int capacity;                                                     \
            if (buf->data == NULL)                                            \
                capacity = DEFAULT_CAPACITY;                                  \
            else                                                              \
                capacity = power_of_2_ceil(buf->capacity * 2);                \
            buf->data = reallocate(buf->data, capacity * sizeof(typ));        \
            buf->capacity = capacity;                                         \
        }                                                                     \
        buf->data[(buf->length)++] = val;                                     \
    }                                                                         \
                                                                              \
    typ name##BufferPop(name##Buffer* buf) {                                  \
        return buf->data[--(buf->length)];                                    \
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

