#include "object.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Object, Object*);
DEFINE_BUFFER(Char, char);

void object_destroy(Object* o) {
    switch (o->typ) {
        case o_Null:
        case o_Float:
        case o_Integer:
        case o_Boolean:
        case o_ReturnValue:
        case o_Function:
        case o_BuiltinFunction:
            break;
        case o_String:
            free(o->data.string->data);
            free(o->data.string);
            break;
        case o_Array:
            free(o->data.array->data);
            free(o->data.array);
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

static int
fprintf_string(ObjectData sl, FILE* fp) {
    if (sl.string->length == 0) {
        FPRINTF(fp, "\"\"");
    } else {
        FPRINTF(fp, "\"%.*s\"", sl.string->length, sl.string->data);
    }
    return 0;
}

static int
fprint_array(ObjectData o, FILE* fp) {
    ObjectBuffer* array = o.array;
    FPRINTF(fp, "[");
    for (int i = 0; i < array->length - 1; i++) {
        object_fprint(array->data[i], fp);
        FPRINTF(fp, ", ");
    }
    if (array->length >= 1)
        object_fprint(array->data[array->length - 1], fp);
    FPRINTF(fp, "]");
    return 0;
}

int object_fprint(Object* o, FILE* fp) {
    switch (o->typ) {
        case o_Integer:
            return fprintf_integer(o->data, fp);
        case o_Float:
            return fprintf_float(o->data, fp);
        case o_Boolean:
            return fprintf_boolean(o->data, fp);
        case o_Null:
            return fprintf_null(fp);
        case o_Error:
            return fprintf_error(o->data, fp);
        case o_BuiltinFunction:
            FPRINTF(fp, "<builtin function>");
            return 0;
        case o_Function:
        case o_Closure:
            return fprint_function(fp);
        case o_String:
            return fprintf_string(o->data, fp);
        case o_Array:
            return fprint_array(o->data, fp);
        default:
            fprintf(stderr, "object_fprint: object type not handled %d\n",
                    o->typ);
            exit(1);
    }
}

bool object_cmp(Object* left, Object* right) {
    if (left->typ != right->typ) return false;

    switch (left->typ) {
        case o_Float:
            return left->data.floating == right->data.floating;
        case o_Integer:
            return left->data.integer == right->data.integer;
        case o_Boolean:
            return left->data.boolean == right->data.boolean;
        case o_Null:
            return true;
        case o_Error:
            return !strcmp(left->data.error_msg, right->data.error_msg);
        case o_ReturnValue:
            return object_cmp(
                    from_return_value(left),
                    from_return_value(right));
        case o_String:
            return !strcmp(left->data.string->data, right->data.string->data);
        case o_Array:
            {
                ObjectBuffer* l_arr = left->data.array;
                ObjectBuffer* r_arr = right->data.array;
                if (l_arr->length != r_arr->length) return false;
                for (int i = 0; i < l_arr->length; i++)
                    if (!object_cmp(l_arr->data[i], r_arr->data[i]))
                        return false;
                return true;
            }
        // TODO: cmp function error
        default:
            fprintf(stderr, "object_cmp: object type not handled %s\n",
                    show_object_type(left->typ));
            exit(1);
            break;
    }
}

inline Object* to_return_value(Object* obj) {
    struct ReturnValue* return_val = (void*)obj;
    return_val->value_typ = return_val->typ;
    return_val->typ = o_ReturnValue;
    return obj;
}

inline Object* from_return_value(Object* obj) {
    if (obj->typ != o_ReturnValue) return obj;
    struct ReturnValue* return_val = (void*)obj;
    return_val->typ = return_val->value_typ;
    return obj;
}

const char* object_types[] = {
    "Null",
    "Integer",
    "Float",
    "Boolean",
    "ReturnValue",
    "Builtin",
    "Error",
    "Function",
    "Closure",
    "String",
    "Array",
};

const char* show_object_type(ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t >= len) {
        fprintf(stderr, "show_object_type: invalid object_type %d\n", t);
        exit(1);
    }
    return object_types[t];
}
