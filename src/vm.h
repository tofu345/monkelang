#pragma once

// This module contains the Stack Virtual Machine.

#include "compiler.h"
#include "object.h"
#include "utils.h"

#include <stdint.h>

// TODO: check for integer overflow, underflow.

// - Perform garbage collection before every allocation to ensure all objects
//   needed are kept in scope.
// - Print allocations, function calls and garbage collection.
#define DEBUG

static const int StackSize = 2048;
static const int GlobalsSize = 2048;
static const int MaxFraxes = 1024;

// from wren: Number of bytes allocated before triggering GC.
#ifndef DEBUG
static const int NextGC = 1024;
#else
static const int NextGC = 1;
#endif

// see [allocation.h]
typedef struct Allocation {
    ObjectType type;
    bool is_marked;
    struct Allocation *next;

    void *object_data[]; // used to access data after struct.
} Allocation;

// A Function call.
typedef struct {
    Closure *cl;

    int ip; // instruction pointer

    // Points to bottom of stack for current function.
    //
    // When returning to the previous Frame, the stack pointer is set to the
    // this value, removing all arguments, local variables and intermediate
    // objects from scope.
    int base_pointer;
} Frame;

typedef struct {
    // A copy of the Bytecode Constants for (hopefully) faster retrieval.
    ConstantBuffer constants;

    // The stack, contains all Objects in scope.
    Object *stack;
    int sp; // Stack pointer, points to right after the top of stack.
            // The top of stack is `stack[sp-1]`.

    // Global variables, where Objects of global variables are stored.
    Object *globals;
    int num_globals;

    // The Function call stack.
    Frame *frames;
    int frames_index; // 0-based index of current Frame

    // The current number of bytes to allocate till before GC is run.
    int bytesTillGC;
    // most recent [Allocation] in linked list of all allocated objects.
    Allocation *last;

    // The Closure of the main function.
    Closure *main_cl;
} VM;

// if [stack], [globals] or [frames] are NULL, allocate.
void vm_init(VM *vm, Object *stack, Object *globals, Frame *frames);

error vm_run(VM *vm, Bytecode bytecode);

void vm_free(VM *vm);

// Last object popped of the stack.
Object vm_last_popped(VM *vm);

// Print where an error occured when [vm_run] exited, using instruction
// pointers and SourceMapping of Frames in the Function call stack.
void print_vm_stack_trace(VM *vm);

// Truthy:
// - boolean true
// - numbers not 0 (C-like)
// - arrays and tables with more than 1 element
// - everything else except null.
bool is_truthy(Object obj);
