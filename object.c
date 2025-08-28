#include "object.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>

void object_destroy(Object* o) {
    if (o->obj == NULL) return;
    switch (o->typ) {
        case o_Boolean:
        case o_Null:
            break;
        default:
            free(o->obj);
            break;
    }
}

static int
fprintf_integer(Integer* i, FILE* fp) {
    FPRINTF(fp, "%ld", i->value);
    return 0;
}

static int
fprintf_boolean(Boolean* b, FILE* fp) {
    FPRINTF(fp, "%s", b->value ? "true" : "false");
    return 0;
}

static int
fprintf_null(FILE* fp) {
    FPRINTF(fp, "null");
    return 0;
}

int object_fprint(Object o, FILE* fp) {
    if (o.obj == NULL) return 0;

    switch (o.typ) {
        case o_Integer:
            return fprintf_integer(o.obj, fp);
        case o_Boolean:
            return fprintf_boolean(o.obj, fp);
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
    "Boolean",
    "Null",
};

const char* show_object_type(enum ObjectType t) {
    static size_t len = sizeof(object_types) / sizeof(object_types[0]);
    if (t == 0 || t > len) {
        fprintf(stderr, "invalid object_type to show_object_type");
        exit(1);
    }
    return object_types[t - 1];
}
