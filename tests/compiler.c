#include "../src/ast.h"
#include "../src/parser.h"
#include "../src/compiler.h"

#include "unity/unity.h"
#include "compiler.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void test_integer_arithmetic(void) {
    run_compiler_test(
        "1 + 2",
        _I(
            make(OpConstant, 0),
            make(OpConstant, 1)
        ),
        _C( INT(1), INT(2) )
    );
}

static Program parse(char *input);
static void check_compiler_errors(Compiler* c);
static void test_instructions(Instructions expected, Instructions actual);
static void test_constants(Constants expected, ConstantBuffer *actual);

static void
run_compiler_test(
    char *input,
    Instructions expectedInstructions,
    Constants expectedConstants
) {
    Program prog = parse(input);

    Compiler c = {};
    compile(&c, &prog);
    check_compiler_errors(&c);

    Bytecode *code = bytecode(&c);

    test_instructions(expectedInstructions, code->instructions);
    test_constants(expectedConstants, &code->constants);

    free(expectedInstructions.data);
    free(expectedConstants.data);
    program_destroy(&prog);
    compiler_destroy(&c);
}

// return [Constants] of [Constant]'s of arbitrary length
Constants
constants(Constant c, ...) {
    va_list ap;
    int length = 0, capacity = 8;
    Constant *constants =
        malloc(8 * sizeof(Constant));
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

// append all Instructions after [ins] into [ins]
Instructions
concat(Instructions ins, ...) {
    va_list ap;
    Instructions cur;
    int length,
        index = ins.length;
    va_start(ap, ins);
    while (1) {
        cur = va_arg(ap, Instructions);
        if (cur.data == NULL) break;

        length = cur.length;
        instructions_fill(&ins, 0, length);
        memcpy(ins.data + index, cur.data, length * sizeof(uint8_t));
        index += length;
        free(cur.data);
    }
    va_end(ap);
    return ins;
}

static void
test_instructions(Instructions expected, Instructions actual) {
    if (actual.length != expected.length) {
        printf("wrong instructions length.\nwant=\n");
        fprint_instruction(stdout, expected);
        printf("got=\n");
        fprint_instruction(stdout, actual);
        TEST_FAIL();
    }

    for (int i = 0; i < expected.length; i++) {
        if (actual.data[i] != expected.data[i]) {
            printf("wrong instruction at %d.\nwant=%d\ngot =%d",
                    i, expected.data[i], actual.data[i]);
            TEST_FAIL();
        }
    }
}

static int
test_integer_object(long expected, Object *actual) {
    if (o_Integer != actual->typ) {
        printf("object is not Integer. got=%s\n", show_object_type(actual->typ));
        return -1;
    }

    if (expected != actual->data.integer) {
        printf("object has wrong value. want=%ld got=%ld\n",
                expected, actual->data.integer);
        return -1;
    }
    return 0;
}

static void test_constants(Constants expected, ConstantBuffer *actual) {
    if (actual->length != expected.length) {
        printf("wrong constants length.\nwant=%d\ngot =%d\n",
                expected.length, actual->length);
        TEST_FAIL();
    }

    int err;
    for (int i = 0; i < expected.length; i++) {
        switch (expected.data[i].type) {
            case c_int:
                err = test_integer_object(expected.data[i].data._int, actual->data + i);
                if (err != 0) {
                    printf("constant %d - testIntegerObject failed\n", i);
                    TEST_FAIL();
                }
                break;

            default:
                TEST_FAIL_MESSAGE("not implemented");
        }
    }
}

static void
print_errors(StringBuffer* errs) {
    for (int i = 0; i < errs->length; i++) {
        printf("%s\n", errs->data[i]);
    }
    TEST_FAIL();
}

static void
check_parser_errors(Parser* p) {
    if (p->errors.length == 0) { return; }
    printf("parser had %d errors\n", p->errors.length);
    print_errors(&p->errors);
}

static void
check_compiler_errors(Compiler* c) {
    if (c->errors.length == 0) { return; }
    printf("compiler had %d errors\n", c->errors.length);
    print_errors(&c->errors);
}

static Program
parse(char *input) {
    Lexer l = lexer_new(input);
    Parser p;
    parser_init(&p, &l);
    Program prog = parse_program(&p);

    TEST_ASSERT_NOT_NULL_MESSAGE(prog.stmts.data, "program.statements NULL");
    check_parser_errors(&p);
    parser_destroy(&p);

    return prog;
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_integer_arithmetic);
    return UNITY_END();
}
