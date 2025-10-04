#include "utils.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

inline void* allocate(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) die("allocate");
    return ptr;
}

inline void* reallocate(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) die("reallocate");
    return new_ptr;
}

void die(const char *fmt, ...) {
    va_list ap;
    int saved_errno;

    saved_errno = errno;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fmt[0] && fmt[strlen(fmt)-1] == ':')
        fprintf(stderr, " %s", strerror(saved_errno));
    fputc('\n', stderr);

    exit(1);
}

int power_of_2_ceil(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}
