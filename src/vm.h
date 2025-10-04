#pragma once

#include "code.h"
#include "compiler.h"
#include "compiler.h"
#include "parser.h"
#include "object.h"

static const int StackSize = 2048;

typedef struct {
    ConstantBuffer constants;
    Instructions instructions;

    StringBuffer errors;

    Object *stack;
    int sp; // Always points to the next value.
            // Top of stack is `stack[sp-1]`
} VM;

void vm_init(VM *vm, Bytecode* code);
void vm_destroy(VM *vm);

int vm_run(VM *vm);
Object vm_last_popped(VM *vm);
