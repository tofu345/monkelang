#pragma once

// This file contains sub modules, other source files which are "required" 
// i.e loaded, compiled and executed at runtime by the VM.

#include "ast.h"
#include "compiler.h"

typedef struct Module {
    const char *name;
    const char *source;
    Program program;

    Object *globals; // Global variables defined in [program].
    int num_globals;
    CompiledFunction *main_function; // Entry point.

    // The `Module` that "required" this one.
    struct Module *parent;
} Module;

void module_bytecode(Module *, Bytecode);
