#pragma once

// This module contains utility functions.

#include "errors.h"

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
    void name##BufferFill(name##Buffer* buf, typ val, int length);            \
    void name##BufferPush(name##Buffer* buf, typ val);                        \

// From: wren DEFINE_BUFFER
#define DEFINE_BUFFER(name, typ)                                              \
    void name##BufferInit(name##Buffer* buf) {                                \
        buf->data = NULL;                                                     \
        buf->length = 0;                                                      \
        buf->capacity = 0;                                                    \
    }                                                                         \
                                                                              \
    void name##BufferFill(name##Buffer* buf, typ val, int length) {           \
        if (buf->length + length > buf->capacity) {                           \
            int capacity = power_of_2_ceil(buf->length + length);             \
            buf->data = realloc(buf->data, capacity * sizeof(typ));           \
            if (buf->data == NULL) {                                          \
                die("realloc %sBuffer", #name);                               \
            }                                                                 \
            buf->capacity = capacity;                                         \
        }                                                                     \
        for (int i = 0; i < length; i++) {                                    \
            buf->data[buf->length++] = val;                                   \
        }                                                                     \
    }                                                                         \
                                                                              \
    void name##BufferPush(name##Buffer* buf, typ val) {                       \
        name##BufferFill(buf, val, 1);                                        \
    }                                                                         \

// `void *` Buffer
BUFFER(, void *)
BUFFER(Int, int)

// from dwm :p
_Noreturn void die(const char *fmt, ...);

// return fnv1a hash of `string[0:length)` exclusive.
uint64_t hash_string_fnv1a(const char *string, int length);

// From: wrenPowerOf2Ceil: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
int power_of_2_ceil(int n);

static inline int fprintf_float(double f, FILE* fp) {
    FPRINTF(fp, "%.16g", f);
    // %g specifier removes trailing '.' in float if it is followed by only zeros
    if (f == (long) f) { FPRINTF(fp, ".") }
    return 0;
}

error load_file(const char *filename, char **source);
