#include "object.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Object, Object);

void object_destroy(Object* o) {
    switch (o->typ) {
        case o_Null:
        case o_Float:
        case o_Integer:
        case o_Boolean:
        case o_ReturnValue:
        case o_Function:
            break;
        case o_Closure:
            free(o->data.closure);
            break;
        case o_Error:
            free(o->data.error_msg);
            return;
        default:
            printf("object_destroy: %d typ not handled\n", o->typ);
            exit(1);
            return;
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

static int
fprintf_error(ObjectData e, FILE* fp) {
    FPRINTF(fp, "error: %s", e.error_msg);
    return 0;
}

static int
fprint_function(FILE* fp) {
    // [Object] with type [o_Function] points to [FunctionLiteral] which
    // is managed by [Program] i.e freed with [Program]. For simplicity,
    // printing functions is disabled.
    FPRINTF(fp, "<function>");
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
        case o_Error:
            return fprintf_error(o.data, fp);
        case o_Function:
        case o_Closure:
            return fprint_function(fp);
        default:
            fprintf(stderr, "object_fprint: object type not handled %d\n",
                    o.typ);
            exit(1);
    }
}

bool object_cmp(Object left, Object right) {
    if (left.typ != right.typ) return false;

    switch (left.typ) {
        case o_Float:
            return left.data.floating == right.data.floating;
        case o_Integer:
            return left.data.integer == right.data.integer;
        case o_Boolean:
            return left.data.boolean == right.data.boolean;
        case o_Null:
            return true;
        case o_Error:
            return !strcmp(left.data.error_msg, right.data.error_msg);
        case o_ReturnValue:
            return object_cmp(
                    from_return_value(left),
                    from_return_value(right));
        // TODO: cmp function error
        default:
            fprintf(stderr, "object_cmp: object type not handled %s\n",
                    show_object_type(left.typ));
            exit(1);
            break;
    }
}

Object object_copy(Object obj) {
    switch (obj.typ) {
        case o_Null:
        case o_Integer:
        case o_Float:
        case o_Boolean:
        case o_Error:
        case o_Function:
        case o_Closure:
            return obj;
        case o_ReturnValue:
            return to_return_value(object_copy(from_return_value(obj)));
        default:
            fprintf(stderr, "object_copy: object type not handled %d\n",
                    obj.typ);
            exit(1);
    }
}

inline Object to_return_value(Object obj) {
    struct ReturnValue* return_val = (void*)&obj;
    return_val->value_typ = return_val->typ;
    return_val->typ = o_ReturnValue;
    return obj;
}

inline Object from_return_value(Object obj) {
    if (obj.typ != o_ReturnValue) return obj;
    struct ReturnValue* return_val = (void*)&obj;
    return_val->typ = return_val->value_typ;
    return obj;
}

const char* object_types[] = {
    "Null",
    "Integer",
    "Float",
    "Boolean",
    "ReturnValue",
    "Error",
    "Function",
    "Closure",
};

const char* show_object_type(ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t >= len) {
        fprintf(stderr, "show_object_type: invalid object_type %d\n", t);
        exit(1);
    }
    return object_types[t];
}
