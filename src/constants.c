#include "constants.h"

DEFINE_BUFFER(Constant, Constant)

void free_constant(Constant c) {
    switch (c.type) {
        case c_Function:
            {
                Function *ins = c.data.function;
                free(ins->instructions.data);
                free(ins->name);
                free(ins);
                break;
            }

        case c_String:
            free(c.data.string);
            break;

        default:
            return;
    }
}
