#pragma once

#include "object.h"
#include "vm.h"

// returns 0 on success
typedef int Builtin(VM *vm, Object *args, int num_args);

typedef struct {
    const char* name;
    Builtin *fn;
} Builtins;

// NULL-terminated array of builtin functions
const Builtins *get_builtins();
