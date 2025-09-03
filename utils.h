#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEFAULT_CAPACITY 8

#define FPRINTF(fp, ...) \
    if (fprintf(fp, __VA_ARGS__) <= 0) \
        return -1;

#define ALLOC_FAIL() \
    do { \
        fprintf(stderr, "allocation failed: %s\n", strerror(errno)); \
        exit(1); \
    } while(0);

// for use in evaluator.c
// TODO? gc?
#define _ALLOC_FAIL() \
    do { \
        fprintf(stderr, "allocation failed: %s\n", strerror(errno)); \
        exit(1); \
    } while(0);
