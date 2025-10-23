#pragma once

#include "object.h"
#include "vm.h"

// returns 0 on success
typedef Object BuiltinFn(VM *vm, Object *args, int num_args);

struct Builtin {
    const char* name;
    BuiltinFn *fn;
};

// NULL-terminated array of builtin functions
const Builtin *get_builtins();
