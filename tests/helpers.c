#include "unity/unity.h"
#include "helpers.h"

void print_errors(ErrorBuffer* errs) {
    for (int i = 0; i < errs->length; i++) {
        printf("%s\n", errs->data[i]);
        free(errs->data[i]);
    }
    errs->length = 0;
}

void check_parser_errors(Parser* p) {
    if (p->errors.length == 0) { return; }
    printf("parser had %d errors\n", p->errors.length);
    print_errors(&p->errors);
    parser_free(p);
    TEST_FAIL();
}

Program test_parse(char *input) {
    Parser p;
    parser_init(&p);
    Program prog = parse(&p, input);
    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    parser_free(&p);
    return prog;
}

Instructions
concat(Instructions ins, ...) {
    va_list ap;
    Instructions cur;
    int cur_start;
    va_start(ap, ins);
    while (1) {
        cur = va_arg(ap, Instructions);
        if (cur.data == NULL) break;

        cur_start = ins.length;
        instructions_allocate(&ins, cur.length);
        memcpy(ins.data + cur_start, cur.data, cur.length * sizeof(uint8_t));
        free(cur.data);
    }
    va_end(ap);
    return ins;
}

Constants
constants(Constant c, ...) {
    va_list ap;
    int length = 0, capacity = 8;
    Constant *constants = malloc(capacity * sizeof(Constant));
    if (constants == NULL) { die("constants: malloc"); }
    va_start(ap, c);
    do {
        if (length >= capacity) {
            constants = realloc(constants, capacity *= 2);
            if (constants == NULL) die("realloc");
        }
        constants[length++] = c;
        c = va_arg(ap, Constant);
    } while (c.type != 0);
    va_end(ap);
    return (Constants){ constants, length };
}
