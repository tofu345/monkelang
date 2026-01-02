#pragma once

// This module contains the Stack Virtual Machine.

#include "compiler.h"
#include "sub_modules.h"
#include "object.h"
#include "utils.h"

#include <stdint.h>

// When defined:
// - print allocations, function calls, returns and garbage collection.
// - perform checks on stack pointer in vm_pop()
// #define DEBUG

static const int StackSize = 2048;
static const int GlobalsSize = 2048;
static const int MaxFraxes = 1024;

// from wren: Number of bytes allocated before triggering GC.
static const int NextGC = 1024;

// A Function call.
typedef struct {
    Object function;

    int ip; // instruction pointer to bytecode

    // Points to bottom of stack for current function.
    //
    // When returning to the previous Frame, the stack pointer is set to the
    // this value, removing all arguments, local variables and intermediate
    // objects from scope.
    short base_pointer;
} Frame;

typedef struct VM {
    // The stack, contains all non-global Objects in scope.
    Object *stack;
    int sp; // Stack pointer, points to right after the top of stack.
            // The top of stack is `stack[sp-1]`.

    // The Function call stack.
    Frame *frames;
    int frames_index; // 0-based index of current Frame

    // The current number of bytes to allocate till before GC is run.
    int bytesTillGC;
    struct Allocation *last; // Linked list of all allocated objects.

    // Globals, contains global variables used in `Bytecode`.
    Object *globals;
    int num_globals;

    Module *cur_module;
    ht *sub_modules; // "required" sub Modules.

    Compiler *compiler; // to access Constants
    Closure *closure; // for main function
} VM;

void vm_init(VM *, Compiler *);
void vm_free(VM *);

error vm_run(VM *vm, Bytecode);

// Last object popped of the stack.
Object vm_last_popped(VM *);

// int vm_num_globals(VM *vm);

// Print the state of the function call stack where vm_run() exited, using
// instruction pointers and SourceMapping of `Frames`.
void print_vm_stack_trace(VM *vm, FILE *stream);
