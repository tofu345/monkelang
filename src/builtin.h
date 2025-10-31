#pragma once

#include "object.h"
#include "vm.h"

// returns 0 on success
typedef Object BuiltinFn(VM *vm, Object *args, int num_args);

struct Builtin {
    const char* name;
    BuiltinFn *fn;
};

typedef struct {
    const Builtin *data;
    int length;
} Builtins;

Builtins get_builtins();
