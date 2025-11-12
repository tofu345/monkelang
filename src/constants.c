#include "constants.h"

DEFINE_BUFFER(Constant, Constant)

void free_function(CompiledFunction *fn) {
    free(fn->instructions.data);
    free(fn->mappings.data);
    free(fn);
}
