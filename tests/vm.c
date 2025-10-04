#include "unity/unity.h"
#include "helpers.h"
#include "tests.h"
#include "unity/unity_internals.h"

#include "../src/vm.h"

void setUp(void) {}
void tearDown(void) {}

static void vm_test(char *input, Test expected);

static void
test_integer_arithmetic(void) {
    vm_test("1", TEST(int, 1));
    vm_test("2", TEST(int, 2));
    vm_test("1 + 2", TEST(int, 3));
    vm_test("1 - 2", TEST(int, -1));
    vm_test("1 * 2", TEST(int, 2));
    vm_test("4 / 2", TEST(int, 2));
    vm_test("50 / 2 * 2 + 10 - 5", TEST(int, 55));
    vm_test("5 + 5 + 5 + 5 - 10", TEST(int, 10));
    vm_test("2 * 2 * 2 * 2 * 2", TEST(int, 32));
    vm_test("5 * 2 + 10", TEST(int, 20));
    vm_test("5 + 2 * 10", TEST(int, 25));
    vm_test("5 * (2 + 10)", TEST(int, 60));
    vm_test("-5", TEST(int, -5));
    vm_test("-10", TEST(int, -10));
    vm_test("-50 + 100 + -50", TEST(int, 0));
    vm_test("(5 + 10 * 2 + 15 / 3) * 2 + -10", TEST(int, 50));
}

static void
test_boolean_expressions(void) {
    vm_test("true", TEST(bool, true));
    vm_test("false", TEST(bool, false));
    vm_test("1 < 2", TEST(bool, true));
    vm_test("1 > 2", TEST(bool, false));
    vm_test("1 < 1", TEST(bool, false));
    vm_test("1 > 1", TEST(bool, false));
    vm_test("1 == 1", TEST(bool, true));
    vm_test("1 != 1", TEST(bool, false));
    vm_test("1 == 2", TEST(bool, false));
    vm_test("1 != 2", TEST(bool, true));
    vm_test("true == true", TEST(bool, true));
    vm_test("false == false", TEST(bool, true));
    vm_test("true == false", TEST(bool, false));
    vm_test("true != false", TEST(bool, true));
    vm_test("false != true", TEST(bool, true));
    vm_test("(1 < 2) == true", TEST(bool, true));
    vm_test("(1 < 2) == false", TEST(bool, false));
    vm_test("(1 > 2) == true", TEST(bool, false));
    vm_test("(1 > 2) == false", TEST(bool, true));
    vm_test("!true", TEST(bool, false));
    vm_test("!false", TEST(bool, true));
    vm_test("!5", TEST(bool, false));
    vm_test("!!true", TEST(bool, true));
    vm_test("!!false", TEST(bool, false));
    vm_test("!!5", TEST(bool, true));
}

static void
test_expected_object(Test expected, Object actual) {
    int err;
    switch (expected.typ) {
        case test_int:
            err = test_integer_object(expected.val._int, actual);
            if (err != 0) {
                TEST_FAIL_MESSAGE("test_integer_object failed");
            }
            break;

        case test_bool:
            err = test_boolean_object(expected.val._bool, actual);
            if (err != 0) {
                TEST_FAIL_MESSAGE("test_boolean_object failed");
            }
            break;

        default:
            die("test_expected_object: Test of type %d not handled",
                    expected.typ);
    }
}

static void
vm_test(char *input, Test expected) {
    Program prog = parse(input);
    Compiler c = {};
    compile(&c, &prog);
    check_compiler_errors(&c);
    Bytecode *code = bytecode(&c);

    VM vm;
    vm_init(&vm, code);
    if (vm_run(&vm) == -1) {
        printf("vm had %d errors\n", vm.errors.length);
        print_errors(&vm.errors);
    }

    Object stack_elem = vm_last_popped(&vm);

    test_expected_object(expected, stack_elem);

    program_destroy(&prog);
    compiler_destroy(&c);
    vm_destroy(&vm);
}
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_integer_arithmetic);
    RUN_TEST(test_boolean_expressions);
    return UNITY_END();
}
