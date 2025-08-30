#include "object.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>

void object_destroy(Object* o) {
    switch (o->typ) {
        case o_Float:
        case o_Integer:
        case o_Boolean:
        case o_Null:
            break;
        case o_ReturnValue:
            {
                // TODO: replace with garbage collector
                Object* val = o->data.ptr;
                object_destroy(val);
                free(val);
            }
        default:
            // TODO: panic if type not handled
            break;
    }
}

static int
fprintf_integer(ObjectData i, FILE* fp) {
    FPRINTF(fp, "%ld", i.integer);
    return 0;
}

static int
fprintf_float(ObjectData f, FILE* fp) {
    FPRINTF(fp, "%.3f", f.floating);
    return 0;
}

static int
fprintf_boolean(ObjectData b, FILE* fp) {
    FPRINTF(fp, "%s", b.boolean ? "true" : "false");
    return 0;
}

static int
fprintf_null(FILE* fp) {
    FPRINTF(fp, "null");
    return 0;
}

int object_fprint(Object o, FILE* fp) {
    switch (o.typ) {
        case o_Integer:
            return fprintf_integer(o.data, fp);
        case o_Float:
            return fprintf_float(o.data, fp);
        case o_Boolean:
            return fprintf_boolean(o.data, fp);
        case o_Null:
            return fprintf_null(fp);
        default:
            fprintf(stderr, "object_fprint: object type not handled %d\n",
                    o.typ);
            exit(1);
    }
}

Object to_return_value(Object obj) {
    struct ReturnValue* return_val = (void*)&obj;
    return_val->value_typ = return_val->typ;
    return_val->typ = o_ReturnValue;
    return obj;
}

Object from_return_value(Object obj) {
    struct ReturnValue* return_val = (void*)&obj;
    return_val->typ = return_val->value_typ;
    return obj;
}

const char* object_types[] = {
    "Null",
    "Integer",
    "Float",
    "Boolean",
};

const char* show_object_type(ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t >= len) {
        fprintf(stderr, "show_object_type: invalid object_type %d\n", t);
        exit(1);
    }
    return object_types[t];
}
