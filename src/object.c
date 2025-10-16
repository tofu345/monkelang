#include "object.h"
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
fprint_hash(table *tbl, FILE* fp) {
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
fprint_compiled_function(CompiledFunction *func, FILE *fp) {
    FPRINTF(fp, "CompiledFunction[%p]", func);
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

        case o_Array:
            return fprint_array(o.data.array, fp);

        case o_Hash:
            return fprint_hash(o.data.hash, fp);

        case o_CompiledFunction:
            return fprint_compiled_function(o.data.func, fp);

        default:
            fprintf(stderr, "object_fprint: object type not handled %d\n",
                    o.type);
            exit(1);
    }
}

bool object_eq(Object left, Object right) {
    if (left.type != right.type) return false;

    switch (left.type) {
        case o_Float:
            return left.data.floating == right.data.floating;

        case o_Integer:
            return left.data.integer == right.data.integer;

        case o_Boolean:
            return left.data.boolean == right.data.boolean;

        case o_Null:
            return true;

        case o_String:
            return !strcmp(left.data.string->data, right.data.string->data);

        case o_Array:
            {
                ObjectBuffer* l_arr = left.data.array;
                ObjectBuffer* r_arr = right.data.array;
                if (l_arr->length != r_arr->length) return false;
                for (int i = 0; i < l_arr->length; i++)
                    if (!object_eq(l_arr->data[i], r_arr->data[i]))
                        return false;
                return true;
            }

        // TODO: cmp function error
        default:
            fprintf(stderr, "object_eq: object type not handled %s\n",
                    show_object_type(left.type));
            exit(1);
            break;
    }
}

const char* object_types[] = {
    "Null",
    "Integer",
    "Float",
    "Boolean",
    "CompiledFunction",
    "BuiltinFunction",
    "Error",
    "String",
    "Array",
    "Hash",
};

const char* show_object_type(ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t >= len) {
        fprintf(stderr, "show_object_type: invalid object_type %d\n", t);
        exit(1);
    }
    return object_types[t];
}
