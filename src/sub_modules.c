#include "sub_modules.h"
#include "compiler.h"

static void module_resize_globals(Module *m, int num_globals) {
    if (num_globals > m->num_globals) {
        m->globals = realloc(m->globals, num_globals * sizeof(Object));
        if (m->globals == NULL) { die("module_resize_globals:"); }
    }

    int added = num_globals - m->num_globals;
    if (added > 0) {
        Object *globals = m->globals + m->num_globals;
        memset(globals, 0, added * sizeof(Object));
    }
    m->num_globals = num_globals;
}

void module_bytecode(Module *m, Bytecode bytecode) {
    module_resize_globals(m, bytecode.num_globals);
    m->main_function = bytecode.main_function;
}
