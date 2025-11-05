#pragma once

// This module manages the allocation and freeing of Compound Data
// Types.
//
// Every compound data type when created has an [Allocation] prepended
// to its allocated memory, this stores the type of the [ObjectType],
// for the use in the garbage collector.
//
// It also contains the next allocation because all allocations are
// stored in a linked list.

#include "object.h"
#include "vm.h"

#include <stdint.h>

// [malloc(size)] and call GC if necessary.
void *vm_allocate(VM *vm, size_t size);

void free_allocation(Allocation *alloc);

CharBuffer *create_string(VM *vm, const char *text, int length);
ObjectBuffer *create_array(VM *vm, Object *data, int length);
table *create_table(VM *vm);
Closure *create_closure(VM *vm, CompiledFunction *func, Object *free,
                        int num_free);

// Create a deep copy of [obj].
Object object_copy(VM* vm, Object obj);

void mark_and_sweep(VM *vm);
