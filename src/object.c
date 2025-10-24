#include "object.h"
#include "builtin.h" // provides type for "Builtin"
#include "utils.h"
#include "table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Object, Object);
DEFINE_BUFFER(Char, char);

static int
fprintf_integer(long i, FILE* fp) {
    FPRINTF(fp, "%ld", i);
    return 0;
}

static int
fprintf_float(double f, FILE* fp) {
    FPRINTF(fp, "%.3f", f);
    return 0;
}

static int
fprintf_boolean(bool b, FILE* fp) {
    FPRINTF(fp, "%s", b ? "true" : "false");
    return 0;
}

static int
fprintf_null(FILE* fp) {
    FPRINTF(fp, "null");
    return 0;
}

static int
fprintf_string(CharBuffer *str, FILE* fp) {
    if (str->length == 0) {
        FPRINTF(fp, "\"\"");
    } else {
        FPRINTF(fp, "\"%.*s\"", str->length, str->data);
    }
    return 0;
}

static int
fprint_array(ObjectBuffer *array, FILE* fp) {
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

static int
fprint_table(table *tbl, FILE* fp) {
    FPRINTF(fp, "{");
    tbl_it it;
    tbl_iterator(&it, tbl);
    size_t last = tbl->length - 1;
    for (size_t i = 0; tbl_next(&it); i++) {
        object_fprint(it.cur_key, fp);
        FPRINTF(fp, ": ");
        object_fprint(it.cur_val, fp);
        if (i != last) {
            FPRINTF(fp, ", ");
        }
    }
    FPRINTF(fp, "}");
    return 0;
}

static int
fprint_closure(Closure *cl, FILE *fp) {
    if (cl->func->name) {
        FPRINTF(fp, "<function: %s>", cl->func->name);
    } else {
        FPRINTF(fp, "<anonymous function>");
    }
    return 0;
}

static int
fprint_builtin_function(Builtin *builtin, FILE *fp) {
    FPRINTF(fp, "<builtin function: %s>", builtin->name);
    return 0;
}

int object_fprint(Object o, FILE* fp) {
    switch (o.type) {
        case o_Integer:
            return fprintf_integer(o.data.integer, fp);

        case o_Float:
            return fprintf_float(o.data.floating, fp);

        case o_Boolean:
            return fprintf_boolean(o.data.boolean, fp);

        case o_Null:
            return fprintf_null(fp);

        case o_String:
            return fprintf_string(o.data.string, fp);

        case o_Error:
            FPRINTF(fp, "error: %s", o.data.err);
            return 0;

        case o_Array:
            return fprint_array(o.data.array, fp);

        case o_Table:
            return fprint_table(o.data.table, fp);

        case o_BuiltinFunction:
            return fprint_builtin_function(o.data.ptr, fp);

        case o_Closure:
            return fprint_closure(o.data.closure, fp);

        default:
            fprintf(stderr, "object_fprint: object type not handled %d\n",
                    o.type);
            exit(1);
    }
}

Object object_eq(Object left, Object right) {
    if (left.type != right.type) return BOOL(false);

    switch (left.type) {
        case o_Float:
            return BOOL(left.data.floating == right.data.floating);

        case o_Integer:
            return BOOL(left.data.integer == right.data.integer);

        case o_Boolean:
            return BOOL(left.data.boolean == right.data.boolean);

        case o_Null:
            return BOOL(true);

        case o_String:
            return BOOL(strcmp(left.data.string->data, right.data.string->data) == 0);

        case o_Array:
            {
                ObjectBuffer* l_arr = left.data.array;
                ObjectBuffer* r_arr = right.data.array;
                if (l_arr->length != r_arr->length) {
                    return BOOL(false);
                }
                Object eq;
                for (int i = 0; i < l_arr->length; i++) {
                    eq = object_eq(l_arr->data[i], r_arr->data[i]);
                    if (IS_ERR(eq)) {
                        return eq;

                    } else if (!eq.data.boolean) {
                        return eq;
                    }
                }
                return BOOL(true);
            }

        default:
            return ERR("cannot compare %s", show_object_type(left.type));
    }
}

const char* object_types[] = {
    "null",
    "integer",
    "float",
    "boolean",
    "builtin",
    "error",
    "string",
    "array",
    "table",
    "function",
};

const char* show_object_type(ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t >= len) {
        fprintf(stderr, "show_object_type: invalid object_type %d\n", t);
        exit(1);
    }
    return object_types[t];
}
