#pragma once

#include <stddef.h>
#include <errno.h>

#define DEFAULT_CAPACITY 8

#define FPRINTF(fp, ...) \
    if (fprintf(fp, __VA_ARGS__) <= 0) \
        return -1;

// perform gc?
#define ALLOC_FAIL() \
    do { \
        fprintf(stderr, "allocation failed: %s\n", strerror(errno)); \
        exit(1); \
    } while(false);
