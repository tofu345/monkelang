#pragma once

#include "utils.h"

// similar to wren DECLARE_BUFFER
#define BUFFER(name, typ)                                                     \
    typedef struct {                                                          \
        typ* data;                                                            \
        int length;                                                           \
        int capacity;                                                         \
    } name##Buffer;                                                           \
                                                                              \
    void name##Buffer##Init(name##Buffer* buf);                               \
    void name##BufferPush(name##Buffer* buf, typ val);                        \
    typ name##BufferPop(name##Buffer* buf);                                   \

// similar to wren DEFINE_BUFFER
#define DEFINE_BUFFER(name, typ)                                              \
    void name##Buffer##Init(name##Buffer* buf) {                              \
        buf->capacity = DEFAULT_CAPACITY;                                     \
        buf->data = calloc(DEFAULT_CAPACITY, sizeof(typ));                    \
        if (buf->data == NULL) {                                              \
            printf("buffer malloc failed\n");                                 \
            exit(1);                                                          \
        }                                                                     \
        buf->length = 0;                                                      \
    }                                                                         \
                                                                              \
    void name##BufferPush(name##Buffer* buf, typ val) {                       \
        if (buf->length >= buf->capacity) {                                   \
            buf->capacity *= 2;                                               \
            buf->data = realloc(buf->data, buf->capacity * sizeof(typ));      \
            if (buf->data == NULL) {                                          \
                printf("buffer alloc failed\n");                              \
                exit(1);                                                      \
            }                                                                 \
        }                                                                     \
        buf->data[(buf->length)++] = val;                                     \
    }                                                                         \
                                                                              \
    typ name##BufferPop(name##Buffer* buf) {                                  \
        return buf->data[--(buf->length)];                                    \
    }                                                                         \

