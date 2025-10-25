#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPRINTF(fp, ...) \
    if (fprintf(fp, __VA_ARGS__) <= 0) return -1;

// From: wren DECLARE_BUFFER
#define BUFFER(name, typ)                                                     \
    typedef struct {                                                          \
        typ* data;                                                            \
        int length;                                                           \
        int capacity;                                                         \
    } name##Buffer;                                                           \
                                                                              \
    void name##BufferInit(name##Buffer* buf);                                 \
    void name##BufferAllocate(name##Buffer* buf, int length);                 \
    void name##BufferPush(name##Buffer* buf, typ val);                        \

// From: wren DEFINE_BUFFER
#define DEFINE_BUFFER(name, typ)                                              \
    void name##BufferInit(name##Buffer* buf) {                                \
        memset(buf, 0, sizeof(name##Buffer));                                 \
    }                                                                         \
                                                                              \
    void name##BufferAllocate(name##Buffer* buf, int length) {                \
        buf->length += length;                                                \
        if (buf->length > buf->capacity) {                                    \
            int capacity = power_of_2_ceil(buf->length);                      \
            buf->data = realloc(buf->data, capacity * sizeof(typ));           \
            if (buf->data == NULL) {                                          \
                die("realloc %sBuffer", #name);                               \
            }                                                                 \
            buf->capacity = capacity;                                         \
        }                                                                     \
    }                                                                         \
                                                                              \
    void name##BufferPush(name##Buffer* buf, typ val) {                       \
        int i = buf->length;                                                  \
        name##BufferAllocate(buf, 1);                                         \
        buf->data[i] = val;                                                   \
    }                                                                         \

// from dwm :p
void die(const char *fmt, ...);

uint64_t hash_string_fnv1a(const char *string, int length);

typedef char *error;

error new_error(const char* format, ...);
error error_num_args(const char *name, int expected, int actual);

// From: wrenPowerOf2Ceil: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
int power_of_2_ceil(int n);
