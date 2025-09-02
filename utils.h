#pragma once

#include <stddef.h>

#define DEFAULT_CAPACITY 8

#define FPRINTF(fp, ...) \
    if (fprintf(fp, __VA_ARGS__) <= 0) \
        return -1;
