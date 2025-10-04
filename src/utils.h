#pragma once

#include "buffer.h"

#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPRINTF(fp, ...) \
    if (fprintf(fp, __VA_ARGS__) <= 0) \
        return -1;

#define ALLOC_FAIL() \
    do { \
        fprintf(stderr, "%s:%d:%s() allocation failed: %s\n", \
                __FILE__, __LINE__, __func__, \
                strerror(errno)); \
        exit(1); \
    } while(0);

BUFFER(String, char*);

void error(StringBuffer* buf, char* format, ...);

// from dwm :p
void die(const char *fmt, ...);

void* allocate(size_t size);
void* reallocate(void* ptr, size_t size);

// From: wrenPowerOf2Ceil: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
int power_of_2_ceil(int n);
