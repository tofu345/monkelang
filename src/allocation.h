#pragma once

// This module manages allocation and freeing of Compound Data Types.

#include "object.h"
#include "vm.h"

#include <stdint.h>

// [malloc(size)] and call GC if necessary.
void *vm_allocate(VM *vm, size_t size);

// [allocate()] and prepend [Allocation] with [type].
void *new_allocation(VM *vm, ObjectType type, size_t size);
void free_allocation(Allocation *alloc);

void mark_and_sweep(VM *vm);
void mark(Object obj);

Object object_copy(VM* vm, Object obj);

CharBuffer *create_string(VM *vm, const char *text, int length);
ObjectBuffer *create_array(VM *vm, Object *data, int length);
table *create_table(VM *vm);
Closure *create_closure(VM *vm, Function *func, Object *free, int num_free);
