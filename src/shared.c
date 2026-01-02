#include "shared.h"

static Parser p;

Parser *parser(void) {
    static bool first_run = true;
    if (first_run) {
        parser_init(&p);
        first_run = false;
    }
    return &p;
}
