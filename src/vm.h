#pragma once

#include "compiler.h"
#include "frame.h"
#include "object.h"
#include "utils.h"

static const int StackSize = 2048;
static const int GlobalsSize = 2048;
static const int MaxFraxes = 1024;

// Number of bytes allocated before GC is run.
static const int NextGC = 2048;

typedef struct {
    ObjectType type;
    bool is_marked;
} Alloc;

BUFFER(Alloc, Alloc *);
BUFFER(Frame, Frame);

typedef struct {
    ConstantBuffer constants;

    Object *stack;
    int sp;

    Object *globals;

    Frame *frames;
    int frames_index;

    // The current number to allocate till before GC is run.
    int bytesTillGC;
    AllocBuffer allocs;
} VM;

// if [stack], [globals] or [frames] are NULL, allocate.
void vm_init(VM *vm, Object *stack, Object *globals, Frame *frames);

// prepare to run [bytecode]
void vm_with(VM *vm, Bytecode bytecode);
void vm_free(VM *vm);

error vm_run(VM *vm);

Object vm_last_popped(VM *vm);
error vm_push(VM *vm, Object obj);
Object vm_pop(VM *vm);

// perform deep copy of [obj], can return error
Object object_copy(VM* vm, Object obj);
