#include "constants.h"

DEFINE_BUFFER(Constant, Constant)

void free_function(Constant c) {
    Function *ins = c.data.function;
    free(ins->instructions.data);
    free(ins);
}

void free_constant(Constant c) {
    switch (c.type) {
        case c_Function:
            free_function(c);
            return;

        default:
            return;
    }
}
