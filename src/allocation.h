#pragma once

// This module manages the allocation and garbage collection of Compound Data
// Types.  Garbage collection based on the Boot.dev memory management in C
// course https://youtu.be/rJrd2QMVbGM and wren https://github.com/wren-lang/wren.

#include "object.h"
#include "vm.h"
#include "table.h"

#include <stdint.h>

// Create a deep copy of [obj].
Object object_copy(VM* vm, Object obj);

// Every compound data type when created has an [Allocation] prepended to its
// allocated memory, this stores information like the [ObjectType], for the use
// in the garbage collector.
typedef struct Allocation {
    ObjectType type;
    bool is_marked;

    // All Allocations are stored in a linked list.
    struct Allocation *next;

    void *object_data[]; // used to access data after struct.
} Allocation;

// `malloc(size)` and garbage collect if necessary.
void *vm_allocate(VM *vm, size_t size);

void free_allocation(Allocation *alloc);
void mark_and_sweep(VM *vm);

CharBuffer *create_string(VM *vm, const char *text, int length);
ObjectBuffer *create_array(VM *vm, Object *data, int length);
Table *create_table(VM *vm);
Closure *create_closure(VM *vm, CompiledFunction *func, Object *free,
                        int num_free);
