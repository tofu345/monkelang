#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// #define TRACING_ENABLED

int traceLevel = 0; // i know

static char*
indent_level(void) {
    int len = 1;
    if (traceLevel > 0)
        len = traceLevel + 1;
    char* indent = malloc(len * sizeof(char));
    int i;
    for (i = 0; i < len; i++) {
        indent[i] = '\t';
    }
    indent[i] = '\0';
    return indent;
}

// not so easy in C, without macro function wrappers, i think

void trace(const char* msg) {
#ifndef TRACING_ENABLED
    return;
#endif
    traceLevel++;
    char* indent = indent_level();
    printf("%sBEGIN %s\n", indent, msg);
    free(indent);
}

void untrace(const char* msg) {
#ifndef TRACING_ENABLED
    return;
#endif
    char* indent = indent_level();
    printf("%sEND %s\n", indent, msg);
    free(indent);
    traceLevel--;
}
