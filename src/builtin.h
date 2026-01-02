#pragma once

// This module contains builtin functions.

#include "object.h"
#include "symbol_table.h"
#include "vm.h"

#include <stdint.h>

typedef Object BuiltinFn(VM *vm, Object *args, int num_args);

typedef struct Builtin {
    const char* name;
    int name_len;
    BuiltinFn *fn;
} Builtin;

const Builtin *get_builtins(int *len);
