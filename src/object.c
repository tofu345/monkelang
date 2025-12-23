#include "object.h"
#include "ast.h"
#include "builtin.h"
#include "errors.h"
#include "utils.h"
#include "table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Object, Object)
DEFINE_BUFFER(Char, char)

static int _object_fprint(Object o, Buffer *print_stack, FILE* fp);

static int
fprintf_integer(long i, FILE* fp) {
    FPRINTF(fp, "%ld", i);
    return 0;
}

static int
fprintf_boolean(bool b, FILE* fp) {
    FPRINTF(fp, "%s", b ? "true" : "false");
    return 0;
}

static int
fprintf_nothing(FILE* fp) {
    FPRINTF(fp, "nothing");
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

inline static int
in_seen(void *cur, Buffer *seen) {
    for (int i = seen->length - 1; i >= 0; --i) {
        if (cur == seen->data[i]) {
            return 1;
        }
    }
    return 0;
}

static int
fprint_array(ObjectBuffer *array, Buffer *seen, FILE* fp) {
    if (in_seen(array, seen)) {
        FPRINTF(fp, "[...]");
        return 0;
    }
    BufferPush(seen, array);

    FPRINTF(fp, "[");

    int last = array->length - 1;
    for (int i = 0; i < last; i++) {
        _object_fprint(array->data[i], seen, fp);
        FPRINTF(fp, ", ");
    }
    if (last >= 0) {
        _object_fprint(array->data[last], seen, fp);
    }

    FPRINTF(fp, "]");

    --seen->length;
    return 0;
}

static int
fprint_table(Table *tbl, Buffer *seen, FILE* fp) {
    if (in_seen(tbl, seen)) {
        FPRINTF(fp, "{...}");
        return 0;
    }
    BufferPush(seen, tbl);

    FPRINTF(fp, "{");
    tbl_it it = tbl_iterator(tbl);
    size_t last = tbl->length - 1;
    for (size_t i = 0; tbl_next(&it); i++) {
        _object_fprint(it.cur_key, seen, fp);
        FPRINTF(fp, ": ");
        _object_fprint(it.cur_val, seen, fp);
        if (i != last) {
            FPRINTF(fp, ", ");
        }
    }
    FPRINTF(fp, "}");

    --seen->length;
    return 0;
}

static int
fprint_closure(Closure *cl, FILE *fp) {
    FunctionLiteral *lit = cl->func->literal;
    if (lit->name) {
        Identifier *id = lit->name;
        FPRINTF(fp, "<function: %.*s>", id->tok.length, id->tok.start);
    } else {
        FPRINTF(fp, "<anonymous function>");
    }
    return 0;
}

static int
fprint_builtin_function(const Builtin *builtin, FILE *fp) {
    FPRINTF(fp, "<builtin: %s>", builtin->name);
    return 0;
}

static int
fprint_error(error err, FILE *fp) {
    FPRINTF(fp, "<error: %s>", err->message);
    return 0;
}

static int
_object_fprint(Object o, Buffer *seen, FILE* fp) {
    switch (o.type) {
        case o_Integer:
            return fprintf_integer(o.data.integer, fp);

        case o_Float:
            return fprintf_float(o.data.floating, fp);

        case o_Boolean:
            return fprintf_boolean(o.data.boolean, fp);

        case o_Nothing:
            return fprintf_nothing(fp);

        case o_String:
            return fprintf_string(o.data.string, fp);

        case o_Error:
            return fprint_error(o.data.err, fp);

        case o_Array:
            return fprint_array(o.data.array, seen, fp);

        case o_Table:
            return fprint_table(o.data.table, seen, fp);

        case o_BuiltinFunction:
            return fprint_builtin_function(o.data.builtin, fp);

        case o_Closure:
            return fprint_closure(o.data.closure, fp);

        default:
            fprintf(stderr, "object_fprint: object type not handled %d\n",
                    o.type);
            exit(1);
    }
}

int object_fprint(Object o, FILE* fp) {
    Buffer seen = {0};
    int res = _object_fprint(o, &seen, fp);
    free(seen.data);
    return res;
}

bool is_truthy(Object obj) {
    switch (obj.type) {
        case o_Boolean:
            return obj.data.boolean;
        case o_Nothing:
            return false;

        case o_Array:
            return obj.data.array->length > 0;
        case o_Table:
            return obj.data.table->length > 0;

        case o_Integer:
            return obj.data.integer != 0;
        case o_Float:
            return obj.data.floating != 0;

        default:
            return true;
    }
}

Object object_eq(Object left, Object right) {
    if (left.type != right.type) { return OBJ_BOOL(false); }

    switch (left.type) {
        case o_Float:
            return OBJ_BOOL(left.data.floating == right.data.floating);

        case o_Integer:
            return OBJ_BOOL(left.data.integer == right.data.integer);

        case o_Boolean:
            return OBJ_BOOL(left.data.boolean == right.data.boolean);

        case o_Nothing:
            return OBJ_BOOL(true);

        case o_String:
            return OBJ_BOOL(strcmp(left.data.string->data, right.data.string->data) == 0);

        case o_Array:
            {
                ObjectBuffer* l_arr = left.data.array;
                ObjectBuffer* r_arr = right.data.array;
                if (l_arr->length != r_arr->length) {
                    return OBJ_BOOL(false);
                }

                Object eq;
                for (int i = 0; i < l_arr->length; i++) {
                    eq = object_eq(l_arr->data[i], r_arr->data[i]);
                    if (eq.type == o_Error) {
                        return eq;

                    } else if (!eq.data.boolean) {
                        return eq;
                    }
                }
                return OBJ_BOOL(true);
            }

        // NOTE: must only be for types that use up the entirety of
        // `ObjectData`
        default:
            return OBJ_BOOL(memcmp(&left.data, &right.data, sizeof(ObjectData)) == 0);
    }
}

const char* object_types[] = {
    "nothing",
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
