#include "constants.h"

DEFINE_BUFFER(Constant, Constant)
DEFINE_BUFFER(SourceMapping, SourceMapping)

void free_function(CompiledFunction *fn) {
    free(fn->instructions.data);
    free(fn->mappings.data);
    free(fn);
}

// void free_constant(Constant c) {
//     switch (c.type) {
//         case c_Function:
//             return;
//         default:
//             return;
//     }
// }
