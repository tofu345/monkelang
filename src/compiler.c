#include "compiler.h"
#include "parser.h"

#include <stdarg.h>

DEFINE_BUFFER(Constant, Object);

void compiler_init(Compiler *c) {
    *c = (Compiler){}; // equiv to memset(0)?
}

void compiler_destroy(Compiler *c) {
    for (int i = 0; i < c->errors.length; i++) {
        free(c->errors.data[i]);
    }
    free(c->errors.data);
}

static void
compiler_error(Compiler* c, char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) ALLOC_FAIL();
    StringBufferPush(&c->errors, msg);
    va_end(args);
}

int compile(Compiler *c, Program *prog) {
    return -1;
}

Bytecode *bytecode(Compiler *c) {
    return (Bytecode *)c;
}
