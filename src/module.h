#pragma once

// This file contains sub modules.

#include "ast.h"
#include "compiler.h"
#include "vm.h"

#include <time.h>

// A source file which is "required" i.e loaded, compiled and executed at
// runtime by the VM.
typedef struct Module {
    const char *name;
    const char *source;

    // When the file at [name] was last modified.
    time_t mtime;

    Program program;

    Object *globals; // Global variables defined in [program].
    int num_globals;
    CompiledFunction *main_function; // Entry point.

    // The `Module` that "required" this one.
    struct Module *parent;
} Module;

// create Module with String `Constant`, then load, compile and add to `vm.modules`.
error require_module(VM *vm, Constant filename);
void  module_free(Module *m);
