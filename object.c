#include "object.h"
#include "ast.h"
#include "environment.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

DEFINE_BUFFER(Object, Object);

void object_destroy(Object* o) {
    switch (o->typ) {
        case o_Float:
        case o_Integer:
        case o_Boolean:
        case o_Null:
        case o_ReturnValue:
            break;
        case o_Error:
            free(o->data.error_msg);
            break;
        case o_Function:
            {
                Function* fn = o->data.func;
                for (int i = 0; i < fn->params.length; i++) {
                    free(fn->params.data[i]->value);
                    free(fn->params.data[i]);
                }
                free(fn->params.data);
                destroy_block_statement(fn->body);
                // env_destroy(fn->env);
                free(fn);
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

static int
fprintf_error(ObjectData e, FILE* fp) {
    FPRINTF(fp, "error: %s", e.error_msg);
    return 0;
}

static int
fprint_function(ObjectData d, FILE* fp) {
    Function* fn = d.func;
    FPRINTF(fp, "fn(");
    for (int i = 0; i < fn->params.length - 1; i++) {
        FPRINTF(fp, "%s, ", fn->params.data[i]->value);
    }
    if (fn->params.length >= 1)
        FPRINTF(fp, "%s",
                fn->params.data[fn->params.length - 1]->value);
    FPRINTF(fp, ") {\n");
    fprint_block_statement(fn->body, fp);
    FPRINTF(fp, "\n}\n");
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
            return fprint_function(o.data, fp);
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
    "ReturnValue",
    "Error",
    "Function",
};

const char* show_object_type(ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t >= len) {
        fprintf(stderr, "show_object_type: invalid object_type %d\n", t);
        exit(1);
    }
    return object_types[t];
}
