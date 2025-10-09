#pragma once

#include "compiler.h"

static const int StackSize = 2048;
static const int GlobalsSize = 65536;

// from wren: Number of bytes allocated before triggering GC.
static const int NextGC = 2048;

// See [object.h]
typedef struct {
    void *ptr;
    bool is_marked;
    ObjectType type;
} HeapObject;
BUFFER(HeapObject, HeapObject *);

typedef struct {
    ConstantBuffer constants;
    Instructions instructions;

    ErrorBuffer errors;

    Object *globals;
    Object *stack;
    int sp; // Always points to the next value.
            // Top of stack is `stack[sp-1]`

    // The current number of bytes before GC is run.
    int nextGC;
    HeapObjectBuffer allocs;
} VM;

// call [vm_with()], if [globals] or [stack] are NULL, allocate.
void vm_init(VM *vm, Bytecode *bytecode, Object *globals, Object *stack);

// prepare to run [bytecode] if not NULL.
void vm_with(VM *vm, Bytecode *bytecode);

int vm_run(VM *vm);
Object vm_last_popped(VM *vm);

// free [vm.errors] and [vm.allocs]
void vm_free(VM *vm);
