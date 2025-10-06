#include "unity/unity.h"
#include "helpers.h"

void print_errors(StringBuffer* errs) {
    for (int i = 0; i < errs->length; i++) {
        printf("%s\n", errs->data[i]);
    }
    TEST_FAIL();
}

void check_parser_errors(Parser* p) {
    if (p->errors.length == 0) { return; }
    printf("parser had %d errors\n", p->errors.length);
    print_errors(&p->errors);
}

void check_compiler_errors(Compiler* c) {
    if (c->errors.length == 0) { return; }
    printf("compiler had %d errors\n", c->errors.length);
    print_errors(&c->errors);
}

Program parse(char *input) {
    Lexer l = lexer_new(input);
    Parser p;
    parser_init(&p, &l);
    Program prog = parse_program(&p);

    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    parser_destroy(&p);

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
    Constant *constants =
        allocate(8 * sizeof(Constant));
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
    return (Constants){constants, length};
}

int test_integer_object(long expected, Object actual) {
    if (o_Integer != actual.typ) {
        printf("object is not Integer. got=%s (",
                show_object_type(actual.typ));
        object_fprint(&actual, stdout);
        puts(")");
        return -1;
    }

    if (expected != actual.data.integer) {
        printf("object has wrong value. want=%ld got=%ld\n",
                expected, actual.data.integer);
        return -1;
    }
    return 0;
}

int test_boolean_object(bool expected, Object actual) {
    if (actual.typ != o_Boolean) {
        printf("object is not Boolean. got=%s (",
                show_object_type(actual.typ));
        object_fprint(&actual, stdout);
        puts(")");
        return -1;
    }

    if (actual.data.boolean != expected) {
        printf("object has wrong value. got=%d, want=%d\n",
                actual.data.boolean, expected);
        return -1;
    }

    return 0;
}
