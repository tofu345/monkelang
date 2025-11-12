#include "constants.h"

DEFINE_BUFFER(Constant, Constant)

CompiledFunction *new_function() {
    CompiledFunction *fn = calloc(1, sizeof(CompiledFunction));
    if (fn == NULL) { die("malloc"); }
    return fn;
}

void free_function(CompiledFunction *fn) {
    free(fn->instructions.data);
    free(fn->mappings.data);
    free(fn);
}
