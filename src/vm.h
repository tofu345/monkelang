#pragma once

#include "compiler.h"
#include "object.h"
#include "utils.h"
#include <stdint.h>

// #define DEBUG_PRINT

// TODO:
// - check for integer overflow, underflow.

static const int StackSize = 2048;
static const int GlobalsSize = 2048;
static const int MaxFraxes = 1024;

// from wren: Number of bytes allocated before triggering GC.
static const int NextGC = 1; // FIXME change back to 1024.

// Prepended to all instances of Compound Data Types.
typedef struct Allocation {
    ObjectType type;
    bool is_marked;
    struct Allocation *next;

    void *object_data[]; // used to access data after struct.
} Allocation;

typedef struct {
    Closure *cl;

    int ip; // instruction pointer

    // points to bottom of stack for current function
    int base_pointer;
} Frame;

typedef struct {
    ConstantBuffer constants;

    // contains all local variables and intermediate objects in scope
    Object *stack;
    int sp; // Stack pointer,
            // Top of stack is `stack[sp-1]`

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

    Function *main_fn;
    Closure *main_cl;
} VM;

// if [stack], [globals] or [frames] are NULL, allocate.
void vm_init(VM *vm, Object *stack, Object *globals, Frame *frames);

void vm_free(VM *vm);

error vm_run(VM *vm, Bytecode bytecode);

Object vm_last_popped(VM *vm);

void print_vm_stack_trace(VM *vm);
