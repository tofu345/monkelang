#pragma once

#include <stddef.h>
#include <errno.h>
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
    void name##BufferFill(name##Buffer* buf, typ val, int length);            \
    void name##BufferPush(name##Buffer* buf, typ val);                        \

// From: wren DEFINE_BUFFER
#define DEFINE_BUFFER(name, typ)                                              \
    void name##BufferInit(name##Buffer* buf) {                                \
        *buf = (name##Buffer){};                                              \
    }                                                                         \
                                                                              \
    void name##BufferFill(name##Buffer* buf, typ val, int length) {           \
        if (buf->length + length >= buf->capacity) {                          \
            int capacity = power_of_2_ceil(buf->length + length);             \
            buf->data = realloc(buf->data, capacity * sizeof(typ));           \
            if (buf->data == NULL) {                                          \
                die("realloc %sBufferFill", #name);                           \
            }                                                                 \
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

// from dwm :p
void die(const char *fmt, ...);

BUFFER(Error, char*);
void error(ErrorBuffer* buf, char* format, ...);

// From: wrenPowerOf2Ceil: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
int power_of_2_ceil(int n);
