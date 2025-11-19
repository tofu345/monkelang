#pragma once

#include "object.h"
#include "vm.h"

// This module contains builtin functions.

typedef Object BuiltinFn(VM *vm, Object *args, int num_args);

struct Builtin {
    const char* name;
    BuiltinFn *fn;
};

// set len if not NULL, return pointer to array of Builtins.
const Builtin *get_builtins(int *len);
