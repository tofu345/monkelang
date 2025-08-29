#include "object.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>

void object_destroy(Object* o) {
    switch (o->typ) {
        case o_Boolean:
        case o_Null:
            break;
        default:
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

const char* object_types[] = {
    "Integer",
    "Float",
    "Boolean",
    "Null",
};

const char* show_object_type(ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t == 0 || t > len) {
        fprintf(stderr, "show_object_type: invalid object_type %d", t);
        exit(1);
    }
    return object_types[t - 1];
}
