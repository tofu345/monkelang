#pragma once

#include "compiler.h"
#include "object.h"
#include "utils.h"

static const int StackSize = 2048;
static const int GlobalsSize = 65536;

// Number of bytes allocated before GC is run.
static const int NextGC = 2048;

typedef struct {
    ObjectType type;
    bool is_marked;
} Alloc;
BUFFER(Alloc, Alloc *);

typedef struct {
    ConstantBuffer constants;
    Instructions instructions;

    ErrorBuffer errors;

    Object *globals;
    Object *stack;
    int sp; // Always points to the next value.
            // Top of stack is `stack[sp-1]`

    // The current number to allocate till before GC is run.
    int bytesTillGC;

    AllocBuffer allocs;
} VM;

// call [vm_with()], if [globals] or [stack] are NULL, allocate.
void vm_init(VM *vm, Bytecode *bytecode, Object *globals, Object *stack);

// prepare to run [bytecode] if not NULL.
void vm_with(VM *vm, Bytecode *bytecode);

int vm_run(VM *vm);

Object vm_last_popped(VM *vm);

void vm_free(VM *vm);
