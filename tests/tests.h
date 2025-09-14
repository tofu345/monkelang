#pragma once

#include <stdbool.h>

typedef enum {
    l_Integer,
    l_Float,
    l_String,
    l_Boolean,
} L_Type;

typedef union {
    long integer;
    double floating;
    const char* string;
    bool boolean;
} L_Test;

#define L_TEST(t) (L_Test){t}
