#include "utils.h"
#include "errors.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

DEFINE_BUFFER(, void *)

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

uint64_t hash_string_fnv1a(const char *string, int length) {
    uint64_t hash = FNV_OFFSET;
    int i = 0;
    for (const char *p = string; i < length; ++p, ++i) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

_Noreturn void die(const char *fmt, ...) {
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
    if (n < 4) { n = 4; }
    return n;
}

error load_file(const char *filename, char **source) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return errorf("could not open file %s - %s\n", filename,
                      strerror(errno));
    }

    fseek(fp, 0, SEEK_END);
    long src_size = ftell(fp);
    rewind(fp);

    char* buf = malloc((src_size + 1) * sizeof(char));
    int errno_ = errno;
    if (buf == NULL) {
        fclose(fp);
        return errorf("could not allocate memory for file %s - %s\n",
                      filename, strerror(errno_));
    }

    size_t src_len = fread(buf, sizeof(char), src_size, fp);
    errno_ = ferror(fp);
    if (errno_ != 0) {
        free(buf);
        fclose(fp);
        return errorf("error while reading %s - %s\n", filename, strerror(errno_));
    } else {
        buf[src_len] = '\0';
    }
    fclose(fp);
    *source = buf;
    return 0;
}
