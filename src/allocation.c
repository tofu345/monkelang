#include "allocation.h"
#include "object.h"
#include "utils.h"
#include "vm.h"
#include "table.h"

#include <assert.h>
#include <stdio.h>

CharBuffer *create_string(VM *vm, char *text, int length) {
#ifdef DEBUG_PRINT
    printf("creating %s", show_object_type(o_String));
    if (text) {
        printf(": '%s'", text);
    }
    putc('\n', stdout);
#endif

    char *data = vm_allocate(vm, length + 1);
    if (text) {
        memcpy(data, text, length);
        data[length] = '\0';
    }

    CharBuffer *buf = new_allocation(vm, o_String, sizeof(CharBuffer));
    *buf = (CharBuffer) {
        .data = data,
        .length = length,
        .capacity = length,
    };
    return buf;
}

ObjectBuffer *create_array(VM *vm, Object *data, int length) {
#ifdef DEBUG_PRINT
    printf("creating %s with %d elems\n", show_object_type(o_Array), length);
#endif

    int capacity = length;
    Object *objs = NULL;

    if (length > 0) {
        capacity = power_of_2_ceil(length);
        objs = vm_allocate(vm, capacity * sizeof(Object));
        if (data) { memcpy(objs, data, length * sizeof(Object)); }
    }

    ObjectBuffer *buf = new_allocation(vm, o_Array, sizeof(ObjectBuffer));
    *buf = (ObjectBuffer) {
        .data = objs,
        .length = length,
        .capacity = capacity,
    };
    return buf;
}

table *create_table(VM *vm) {
#ifdef DEBUG_PRINT
    printf("creating %s\n", show_object_type(o_Table));
#endif

    table *tbl = new_allocation(vm, o_Table, sizeof(table));
    void *err = table_init(tbl);
    if (err == NULL) { die("create_table"); }
    return tbl;
}

Closure *create_closure(VM *vm, Function *func, Object *free,
        int num_free) {

    size_t free_size = num_free * sizeof(Object),
           size = sizeof(Closure) + free_size;

    Closure *cl = new_allocation(vm, o_Closure, size);
    *cl = (Closure){
        .num_free = num_free,
        .func = func,
    };

    if (num_free > 0) {
        memcpy(cl->free, free, free_size);
    }
    return cl;
}

Object object_copy(VM* vm, Object obj) {
    switch (obj.type) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_BuiltinFunction:
            return obj;

        case o_String:
            {
                CharBuffer* new_str =
                    create_string(vm, NULL, obj.data.string->length);
                return OBJ(o_String, .string = new_str);
            }

        case o_Array:
            {
                ObjectBuffer *old = obj.data.array;
                ObjectBuffer *new_arr =
                    create_array(vm, old->data, old->length);
                return OBJ(o_Array, .array = new_arr);
            }

        case o_Table:
            {
                table *new_tbl = create_table(vm);
                tbl_it it;
                tbl_iterator(&it, obj.data.table);
                while (tbl_next(&it)) {
                    Object res = table_set(new_tbl, it.cur_key, it.cur_val);
                    if (IS_NULL(res)) {
                        return ERR("could not set table value: %s");
                    }
                }
                return OBJ(o_Table, .table = new_tbl);
            }

        default:
            die("object_copy: object type not handled %s (%d)\n",
                    show_object_type(obj.type), obj.type);
            return NULL_OBJ;
    }
}

static void
mark(Object obj) {
    assert(obj.type >= o_String);

#ifdef DEBUG_PRINT
    printf("mark: ");
    object_fprint(obj, stdout);
    putc('\n', stdout);
#endif

    // access [Alloc] prepended to ptr with [vm_allocate]
    Allocation *alloc = ((Allocation *)obj.data.ptr) - 1;
    alloc->is_marked = true;
}

static void
trace_mark_object(Object obj) {
    switch (obj.type) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_BuiltinFunction:
        case o_Error:
            return;

        case o_String:
            mark(obj);
            return;

        case o_Closure:
            mark(obj);
            for (int i = 0; i < obj.data.closure->num_free; i++)
                trace_mark_object(obj.data.closure->free[i]);
            return;

        case o_Array:
            mark(obj);
            for (int i = 0; i < obj.data.array->length; i++)
                trace_mark_object(obj.data.array->data[i]);
            return;

        case o_Table:
            mark(obj);
            tbl_it it;
            tbl_iterator(&it, obj.data.table);
            while (tbl_next(&it)) {
                trace_mark_object(it.cur_key);
                trace_mark_object(it.cur_val);
            }
            return;

        default:
            die("trace_mark_object: type %s (%d) not handled",
                    show_object_type(obj.type), obj.type);
    }
}

void free_allocation(Allocation *alloc) {
    assert(alloc->type >= o_String);

    void *obj_data = alloc->object_data;

#ifdef DEBUG_PRINT
    printf("free: ");
    object_fprint(OBJ(alloc->type, .ptr = alloc + 1), stdout);
    putc('\n', stdout);
#endif

    switch (alloc->type) {
        case o_String:
            free(((CharBuffer *)obj_data)->data);
            break;

        case o_Array:
            free(((ObjectBuffer *)obj_data)->data);
            break;

        case o_Table:
            table_free(obj_data);
            break;

        case o_Closure:
            break;

        default:
            die("alloc_free: %d typ not handled\n", alloc->type);
            return;
    }
    free(alloc);
}

void mark_objs(Object *objs, int len) {
    for (int i = 0; i < len; i++) {
        if (objs[i].type >= o_String) {
            trace_mark_object(objs[i]);
        }

#ifdef DEBUG_PRINT
        if (objs[i].type < o_String) {
            printf("skip: ");
            object_fprint(objs[i], stdout);
            putc('\n', stdout);
        }
#endif
    }
}

void mark_and_sweep(VM *vm) {
#ifdef DEBUG_PRINT
    puts("begin mark_and_sweep");
    puts("stack:");
#endif
    mark_objs(vm->stack, vm->sp);

#ifdef DEBUG_PRINT
    puts("\nglobals:");
#endif
    mark_objs(vm->globals, vm->num_globals);

#ifdef DEBUG_PRINT
    puts("\nsweep:");
#endif
    // sweep and rebuild Linked list of [Allocations].
    Allocation *cur = vm->last,
               *prev_marked = NULL;
    while (cur) {
        Allocation *next = cur->next;

        if (cur->is_marked) {
            cur->is_marked = false;
            cur->next = prev_marked;
            prev_marked = cur;
            cur = next;
            continue;
        }

        free_allocation(cur);
        cur = next;
    }
    vm->last = prev_marked;
}

void *vm_allocate(VM *vm, size_t size) {
    vm->bytesTillGC -= size;
    if (vm->bytesTillGC <= 0) {
#ifdef DEBUG_PRINT
        if (vm->last) {
            putc('\n', stdout);
            printf("starting mark_and_sweep from:\n");
            for (int i = 0; i <= vm->frames_index; i++) {
                printf("-> function: %s\n", vm->frames[i].cl->func->name);
            }
            putc('\n', stdout);
        }
#endif

        mark_and_sweep(vm);
        vm->bytesTillGC = NextGC;
    }

    void *ptr = malloc(size);
    if (ptr == NULL) { die("malloc"); }
    return ptr;
}

void *new_allocation(VM *vm, ObjectType type, size_t size) {
    // prepend [Alloc] object.
    Allocation *ptr = vm_allocate(vm, size + sizeof(Allocation));
    *ptr = (Allocation){
        .is_marked = false,
        .type = type,
        .next = vm->last,
    };
    vm->last = ptr;

    void *obj_data = ptr->object_data;
    memset(obj_data, 0, size);
    return obj_data;
}
