#pragma once

#include "compiler.h"
#include "frame.h"
#include "object.h"
#include "utils.h"

#include <stdint.h>

static const int StackSize = 2048;
static const int GlobalsSize = 2048;
static const int MaxFraxes = 1024;

// Number of bytes allocated before GC is run.
static const int NextGC = 1; // FIXME change back to 1024.

// Prepended to all instances of Compound Data Types, see [object.h].
typedef struct Allocation {
    ObjectType type;
    bool is_marked;
    struct Allocation *next;
    void *object_data[]; // not initialiazed, points to data after struct.
} Allocation;

BUFFER(Frame, Frame);

typedef struct {
    ConstantBuffer constants;

    // contains all local variables and intermediate objects in scope
    Object *stack;
    int sp; // Top of stack is `stack[sp-1]`

    // contains global variables
    Object *globals;
    int num_globals;

    Frame *frames; // function call stack
    int frames_index; // index of current frame

    // The current number of bytes to allocate till before GC is run.
    int bytesTillGC;
    // most recent [Allocation] in linked list of all allocated
    // `Compound Data Type` objects.
    Allocation *last;
} VM;

// if [stack], [globals] or [frames] are NULL, allocate.
void vm_init(VM *vm, Object *stack, Object *globals, Frame *frames);
void vm_free(VM *vm);

error vm_run(VM *vm, Bytecode bytecode);

// push [obj] onto the stack, returns error if [vm.sp] >= [StackSize].
error vm_push(VM *vm, Object obj);

// retrieve `vm.stack[vm.sp - 1]` and decrement [vm.sp].
// NOTE: does not check if [vm.sp] >= 0.
Object vm_pop(VM *vm);
Object vm_last_popped(VM *vm);

// perform deep copy of [obj]
Object object_copy(VM* vm, Object obj);

// when is too much?
#define COMPOUND_OBJ(typ, data, ...) \
    compound_obj(vm, typ, sizeof(data), &(data)__VA_ARGS__ );

// create `Compound Data Type` of [size] and initialiaze with [data] or 0;
void *compound_obj(VM *vm, ObjectType type, size_t size, void *data);

// allocate [size] and decrement [vm.bytesTillGC]
void *allocate(VM *vm, size_t size);
