#include "unity/unity.h"
#include "helpers.h"
#include "tests.h"

#include "../src/vm.h"
#include "../src/table.h"

#include <stdio.h>

void setUp(void) {}
void tearDown(void) {}

static void vm_test(char *input, Test *expected);
static void vm_test_error(char *input, char *expected_error);

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
    vm_test("1.5 < 2.", TEST(bool, true));
    vm_test("1 > 2", TEST(bool, false));
    vm_test("1 < 1", TEST(bool, false));
    vm_test("1 > 1", TEST(bool, false));
    vm_test("1.5 > 1.", TEST(bool, true));
    vm_test("1 == 1", TEST(bool, true));
    vm_test("1 != 1", TEST(bool, false));
    vm_test("1 == 2", TEST(bool, false));
    vm_test("1.50 == 1.500", TEST(bool, true));
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
test_conditionals(void) {
    vm_test("if (true) { 1.0 }", TEST(float, 1.0));
    vm_test("if (true) { 10 } else { 20 }", TEST(int, 10));
    vm_test("if (false) { 10 } else { 20 } ", TEST(int, 20));
    vm_test("if (1) { 10 }", TEST(int, 10));
    vm_test("if (1 < 2) { 10 }", TEST(int, 10));
    vm_test("if (1 < 2) { 10 } else { 20 }", TEST(int, 10));
    vm_test("if (1 > 2) { 10 } else { 20.7 }", TEST(float, 20.7));
    vm_test("if (1 > 2) { 10 }", TEST_NULL);
    vm_test("if (false) { 10 }", TEST_NULL);
    vm_test("!(if (false) { 5; })", TEST(bool, true));
    vm_test("if ((if (false) { 10 })) { 10 } else { 20 }", TEST(int, 20));
}

static void
test_global_let_statements(void) {
    vm_test("let one = 1; one", TEST(int, 1));
    vm_test("let one = 1; let two = 2; one + two", TEST(int, 3));
    vm_test("let one = 1; let two = one + one; one + two", TEST(int, 3));
}

static void
test_string_expressions(void) {
    vm_test("\"monkey\"", TEST(str, "monkey"));
    vm_test("\"mon\" + \"key\"", TEST(str, "monkey"));
    vm_test("\"mon\" + \"key\" + \"banana\"", TEST(str, "monkeybanana"));
}

// create *NULL-terminated* array of integers.
static TestArray *make_test_array(int n, ...);
#define TEST_ARR(...) TEST(arr, make_test_array(__VA_ARGS__, 0))

static void
test_array_literals(void) {
    vm_test("[]", TEST_ARR(0));
    vm_test("[1, 2, 3]", TEST_ARR(1, 2, 3));
    vm_test("[1 + 2, 3 * 4, 5 + 6]", TEST_ARR(3, 12, 11));
}

#define TEST_TABLE(...) \
    &(Test){ test_table, { ._arr = make_test_array(__VA_ARGS__, 0) }}

static void
test_table_literals(void) {
    vm_test("{}", TEST_TABLE(0));
    vm_test("{1: 2, 2: 3}", TEST_TABLE(1, 2, 2, 3));
    vm_test("{1 + 1: 2 * 2, 3 + 3: 4 * 4}", TEST_TABLE(2, 4, 6, 16));
}

static void
test_index_expressions(void) {
    vm_test("[1, 2, 3][1]", TEST(int, 2));
    vm_test("[1, 2, 3][0 + 2]", TEST(int, 3));
    vm_test("[[1.5, 1, 1]][0][0]", TEST(float, 1.5));
    vm_test("[][0]", TEST_NULL);
    vm_test("[1, 2, 3][99]", TEST_NULL);
    vm_test("[1][-1]", TEST_NULL);
    vm_test("{1: 1, 2: 2}[1]", TEST(int, 1));
    vm_test("{1: 1, 2: 2.1}[2]", TEST(float, 2.1));
    vm_test("{1: 1}[0]", TEST_NULL);
    vm_test("{}[0]", TEST_NULL);
}

static void
test_calling_functions_without_arguments(void) {
    vm_test(
        "\
            let fivePlusTen = fn() { 5 + 10; };\
            fivePlusTen();\
        ",
        TEST(int, 15)
    );
    vm_test(
        "\
            let one = fn() { 1; };\
            let two = fn() { 2; };\
            one() + two()\
        ",
        TEST(int, 3)
    );
    vm_test(
        "\
            let a = fn() { 1 };\
            let b = fn() { a() + 1 };\
            let c = fn() { b() + 1 };\
            c()\
        ",
        TEST(int, 3)
    );
    vm_test(
        "\
            let earlyExit = fn() { return 99; 100; };\
            earlyExit();\
        ",
        TEST(int, 99)
    );
    vm_test(
        "\
            let earlyExit = fn() { return 99; return 100; };\
            earlyExit();\
        ",
        TEST(int, 99)
    );
}

static void
test_functions_without_return_value(void) {
    vm_test(
        "\
            let noReturn = fn() { };\
            noReturn();\
        ",
        TEST_NULL
    );
    vm_test(
        "\
            let noReturn = fn() { };\
            let noReturnTwo = fn() { noReturn(); };\
            noReturn();\
            noReturnTwo();\
        ",
        TEST_NULL
    );
}

static void
test_first_class_functions(void) {
    vm_test(
        "\
            let returnsOne = fn() { 1; };\
            let returnsOneReturner = fn() { returnsOne; };\
            returnsOneReturner()();\
        ",
        TEST(int, 1)
    );
    vm_test(
        "\
            let returnsOneReturner = fn() {\
                let returnsOne = fn() { 1; };\
                returnsOne;\
            };\
            returnsOneReturner()();\
        ",
        TEST(int, 1)
    );
}

static void
test_calling_functions_without_bindings(void) {
    vm_test(
        "\
            let one = fn() { let one = 1; one };\
            one();\
        ",
        TEST(int, 1)
    );
    vm_test(
        "\
            let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };\
            oneAndTwo();\
        ",
        TEST(int, 3)
    );
    vm_test(
        "\
            let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };\
            let threeAndFour = fn() { let three = 3; let four = 4; three + four; };\
            oneAndTwo() + threeAndFour();\
        ",
        TEST(int, 10)
    );
    vm_test(
        "\
            let firstFoobar = fn() { let foobar = 50; foobar; };\
            let secondFoobar = fn() { let foobar = 100; foobar; };\
            firstFoobar() + secondFoobar();\
        ",
        TEST(int, 150)
    );
    vm_test(
        "\
            let globalSeed = 50;\
            let minusOne = fn() {\
            let num = 1;\
                globalSeed - num;\
            }\
            let minusTwo = fn() {\
                let num = 2;\
                globalSeed - num;\
            }\
            minusOne() + minusTwo();\
        ",
        TEST(int, 97)
    );
}

static void
test_calling_functions_with_bindings(void) {
    vm_test(
        "\
            let identity = fn(a) { a; };\
            identity(4);\
        ",
        TEST(int, 4)
    );
    vm_test(
        "\
            let sum = fn(a, b) { a + b; };\
            sum(1, 2);\
        ",
        TEST(int, 3)
    );
    vm_test(
        "\
            let sum = fn(a, b) {\
                let c = a + b;\
                c;\
            };\
            sum(1, 2);\
        ",
        TEST(int, 3)
    );
    vm_test(
        "\
            let sum = fn(a, b) {\
                let c = a + b;\
                c;\
            }\
            sum(1, 2) + sum(3, 4);\
            ",
        TEST(int, 10)
    );
    vm_test(
        "\
            let sum = fn(a, b) {\
                let c = a + b;\
                c;\
            };\
            let outer = fn() {\
                sum(1, 2) + sum(3, 4);\
            };\
            outer();\
        ",
        TEST(int, 10)
    );
    vm_test(
        "\
            let globalNum = 10;\
            let sum = fn(a, b) {\
                let c = a + b;\
                c + globalNum;\
            };\
            let outer = fn() {\
                sum(1, 2) + sum(3, 4) + globalNum;\
            };\
            outer() + globalNum;\
        ",
        TEST(int, 50)
    );
}

static void
test_calling_functions_with_wrong_arguments(void) {
    vm_test_error(
        "fn() { 1; }(1)",
        "<anonymous function> takes 0 arguments got 1"
    );
    vm_test_error(
        "fn(a) { a; }()",
        "<anonymous function> takes 1 argument got 0"
    );
    vm_test_error(
        "fn(a, b) { a + b; }(1)",
        "<anonymous function> takes 2 arguments got 1"
    );
}

static void
test_builtin_functions(void) {
    vm_test("len(\"\")", TEST(int, 0));
    vm_test("len(\"four\")", TEST(int, 4));
    vm_test("len(\"hello world\")", TEST(int, 11));
    vm_test("len([1, 2, 3])", TEST(int, 3));
    vm_test("len([])", TEST(int, 0));
    vm_test("puts(\"hello\", \"world!\")", TEST_NULL);
    vm_test("first([1, 2, 3])", TEST(int, 1));
    vm_test("first([])", TEST_NULL);
    vm_test("last([1, 2, 3])", TEST(int, 3));
    vm_test("last([])", TEST_NULL);
    vm_test("rest([1, 2, 3])", TEST_ARR(2, 3));
    vm_test("rest([])", TEST_ARR(0));
    vm_test("push([], 1)", TEST_ARR(1));

    vm_test_error("len(1)", "builtin len(): argument of integer not supported");
    vm_test_error("len(\"one\", \"two\")", "builtin len() takes 1 argument got 2");
    vm_test_error("first(1)", "builtin first(): argument of integer not supported");
    vm_test_error("last(1)", "builtin last(): argument of integer not supported");
    vm_test_error("push(1, 1)", "builtin push() expects first argument to be array got integer");
}

static void
test_closures(void) {
    vm_test(
        "\
        let newClosure = fn(a) {\
            fn() { a; };\
        };\
        let closure = newClosure(99);\
        closure();\
        ",
        TEST(int, 99)
    );
    vm_test(
        "\
        let newAdder = fn(a, b) {\
            fn(c) { a + b + c };\
        };\
        let adder = newAdder(1, 2);\
        adder(8);\
        ",
        TEST(int, 11)
    );
    vm_test(
        "\
        let newAdder = fn(a, b) {\
            let c = a + b;\
            fn(d) { c + d };\
        };\
        let adder = newAdder(1, 2);\
        adder(8);\
        ",
        TEST(int, 11)
    );
    vm_test(
        "\
        let newAdderOuter = fn(a, b) {\
            let c = a + b;\
            fn(d) {\
                let e = d + c;\
                fn(f) { e + f; };\
            };\
        };\
        let newAdderInner = newAdderOuter(1, 2)\
        let adder = newAdderInner(3);\
        adder(8);\
        ",
        TEST(int, 14)
    );
    vm_test(
        "\
        let a = 1;\
        let newAdderOuter = fn(b) {\
            fn(c) {\
                fn(d) { a + b + c + d };\
            };\
        };\
        let newAdderInner = newAdderOuter(2)\
        let adder = newAdderInner(3);\
        adder(8);\
        ",
        TEST(int, 14)
    );
    vm_test(
        "\
        let newClosure = fn(a, b) {\
            let one = fn() { a; };\
            let two = fn() { b; };\
            fn() { one() + two(); };\
        };\
        let closure = newClosure(9, 90);\
        closure();\
        ",
        TEST(int, 99)
    );
}

static void
test_recursive_functions(void) {
    vm_test(
        "\
        let countDown = fn(x) {\
            if (x == 0) {\
                return 0;\
            } else {\
                countDown(x - 1);\
            }\
        };\
        countDown(1);\
        ",
        TEST(int, 0)
    );
    vm_test(
        "\
        let countDown = fn(x) {\
            if (x == 0) {\
                return 0;\
            } else {\
                countDown(x - 1);\
            }\
        };\
        let wrapper = fn() {\
            countDown(1);\
        };\
        wrapper();\
        ",
        TEST(int, 0)
    );
    vm_test(
        "\
        let wrapper = fn() {\
            let countDown = fn(x) {\
                if (x == 0) {\
                    return 0;\
                } else {\
                    countDown(x - 1);\
                }\
            };\
            countDown(1);\
        };\
        wrapper();\
        ",
        TEST(int, 0)
    );
}

static void
test_recursive_fibonacci(void) {
    vm_test(
        "\
        let fibonacci = fn(x) {\
            if (x == 0) {\
                return 0;\
            } else {\
                if (x == 1) {\
                    return 1;\
                } else {\
                    fibonacci(x - 1) + fibonacci(x - 2);\
                }\
            }\
        };\
        fibonacci(15);\
        ",
        TEST(int, 610)
    );
}

static void
test_assign_expressions(void) {
    vm_test("let a = 15; a = 5; a", TEST(int, 5));
    vm_test("let arr = [1, 2, 3]; arr[0] = 5; arr[0]", TEST(int, 5));
    vm_test(
        "\
            let hash = {1: 2, 3: 4};\
            hash[1] = null;\
            hash[1]\
        ",
        TEST_NULL
    );
    vm_test("let a = null; a = 5;", TEST(int, 5));
}

static TestArray *
make_test_array(int n, ...) {
    va_list ap;
    int length = 0, capacity = 8;
    TestArray *nums = malloc(sizeof(TestArray));
    if (nums == NULL) { die("test_array: malloc"); }
    nums->data = malloc(capacity * sizeof(int));
    if (nums->data == NULL) { die("test_array: malloc arr data"); }

    if (n == 0) {
        nums->length = 0;
        return nums;
    }

    va_start(ap, n);
    do {
        if (length >= capacity) {
            nums = realloc(nums, capacity *= 2);
            if (nums == NULL) die("realloc");
        }
        nums->data[length++] = n;
        n = va_arg(ap, int);
    } while (n != 0);
    va_end(ap);
    nums->length = length;
    return nums;
}

static bool
expect_object_is(ObjectType expected, Object actual) {
    if (actual.type != expected) {
        printf("object is not %s. got=%s (",
                show_object_type(expected),
                show_object_type(actual.type));
        object_fprint(actual, stdout);
        puts(")");
        return false;
    }
    return true;
}

static int
test_integer_object(long expected, Object actual) {
    if (!expect_object_is(o_Integer, actual)) {
        return -1;
    }

    if (expected != actual.data.integer) {
        printf("object has wrong value. want=%ld got=%ld\n",
                expected, actual.data.integer);
        return -1;
    }
    return 0;
}

static int
test_float_object(double expected, Object actual) {
    if (!expect_object_is(o_Float, actual)) {
        return -1;
    }

    if (expected != actual.data.floating) {
        printf("object has wrong value. want=%f got=%f\n",
                expected, actual.data.floating);
        return -1;
    }
    return 0;
}

static int
test_boolean_object(bool expected, Object actual) {
    if (!expect_object_is(o_Boolean, actual)) {
        return -1;
    }

    if (actual.data.boolean != expected) {
        printf("object has wrong value. got=%s, want=%s\n",
                actual.data.boolean ? "true" : "false",
                expected ? "true" : "false");
        return -1;
    }
    return 0;
}

static int
test_string_object(char *expected, Object actual) {
    if (!expect_object_is(o_String, actual)) {
        return -1;
    }

    if (strcmp(actual.data.string->data, expected) != 0) {
        printf("object has wrong value. got='%s', want='%s'\n",
                actual.data.string->data, expected);
        return -1;
    }
    return 0;
}

static int
test_expected_object(Test expected, Object actual) {
    switch (expected.typ) {
        case test_int:
            return test_integer_object(expected.val._int, actual);

        case test_float:
            return test_float_object(expected.val._float, actual);

        case test_bool:
            return test_boolean_object(expected.val._bool, actual);

        case test_str:
            return test_string_object(expected.val._str, actual);

        case test_null:
            if (!expect_object_is(o_Null, actual)) {
                return -1;
            }
            return 0;

        case test_arr:
            {
                if (!expect_object_is(o_Array, actual)) {
                    return -1;
                }

                ObjectBuffer *arr = actual.data.ptr;
                if (expected.val._arr->length != arr->length) {
                    printf("wrong number of elements. want=%d, got=%d\n",
                            expected.val._arr->length, arr->length);
                    return -1;
                }

                int err;
                for (int i = 0; i < expected.val._arr->length; i++) {
                    err = test_integer_object(expected.val._arr->data[i],
                            arr->data[i]);
                    if (err != 0) {
                        printf("test array element %d: test_integer_object failed", i);
                        return -1;
                    }
                }
                free(expected.val._arr->data);
                free(expected.val._arr);
                return 0;
            }

        case test_table:
            {
                if (!expect_object_is(o_Table, actual)) {
                    return -1;
                }

                table *tbl = actual.data.table;
                TestArray *exp = expected.val._arr;
                size_t num_pairs = exp->length / 2;
                if (num_pairs != tbl->length) {
                    printf("wrong number of elements. want=%zu, got=%zu\n",
                            num_pairs, tbl->length);
                    return -1;
                }

                // test `tbl[expected key]` == `expected value`
                int err;
                for (int i = 0; i + 1 < exp->length; i += 2) {
                    Object value = table_get(tbl, OBJ(o_Integer, exp->data[i]));
                    if (value.type == o_Null) {
                        printf("no pair for key %d", i);
                        return -1;
                    }

                    err = test_integer_object(exp->data[i + 1], value);
                    if (err != 0) {
                        printf("test table element %d failed", i);
                        return -1;
                    }
                }

                free(exp->data);
                free(exp);
                return 0;
            }

        default:
            die("test_expected_object: Test of type %d not handled",
                    expected.typ);
            return -1;
    }
}

static void
vm_test(char *input, Test *expected) {
    Compiler c;
    VM vm;
    Program prog = test_parse(input);
    compiler_init(&c);
    vm_init(&vm, NULL, NULL, NULL);

    error err = compile(&c, &prog);
    if (err) {
        printf("test: %s\n", input);
        printf("compiler error: %s\n\n", err);
        free(err);
        program_free(&prog);
        compiler_free(&c);
        TEST_FAIL();
    };

    // // display constants
    // for (int i = 0; i < c.constants.length; i++) {
    //     Constant cn = c.constants.data[i];
    //     printf("CONSTANT %d ", i);
    //     switch (cn.type) {
    //         case c_Function:
    //             printf("%s() Instructions:\n", cn.data.function->name);
    //             fprint_instructions(stdout, cn.data.function->instructions);
    //             break;
    //         case c_Integer:
    //             printf("Value: %ld\n", cn.data.integer);
    //             break;
    //         case c_Float:
    //             printf("Value: %f\n", cn.data.floating);
    //             break;
    //         case c_String:
    //             printf("Value: \"%s\"\n", cn.data.string->data);
    //             break;
    //     }
    // }
    // putc('\n', stdout);

    err = vm_run(&vm, bytecode(&c));
    if (err) {
        printf("test: %s\n", input);
        printf("vm error: %s\n\n", err);
        free(err);
        TEST_FAIL();
    }

    Object stack_elem = vm_last_popped(&vm);
    int _err = test_expected_object(*expected, stack_elem);
    if (_err != 0) {
        printf("test_expected_object failed for test: %s\n\n", input);
        TEST_FAIL();
    }

    vm_free(&vm);
    program_free(&prog);
    compiler_free(&c);
}

static void
vm_test_error(char *input, char *expected_error) {
    Compiler c;
    VM vm;
    Program prog = test_parse(input);
    compiler_init(&c);
    vm_init(&vm, NULL, NULL, NULL);

    error err = compile(&c, &prog);
    if (err) {
        printf("test: %s\n", input);
        printf("compiler error: %s\n\n", err);
        free(err);
        program_free(&prog);
        compiler_free(&c);
        TEST_FAIL();
    };

    err = vm_run(&vm, bytecode(&c));
    if (!err) {
        printf("expected VM error for test: %s\n", input);
        TEST_FAIL();
    }

    vm_free(&vm);
    program_free(&prog);
    compiler_free(&c);

    if (strcmp(err, expected_error) != 0) {
        printf("wrong VM error for test: %s\nwant= %s\ngot = %s\n",
                input, expected_error, err);
        TEST_FAIL();
    }

    free(err);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_integer_arithmetic);
    RUN_TEST(test_boolean_expressions);
    RUN_TEST(test_conditionals);
    RUN_TEST(test_global_let_statements);
    RUN_TEST(test_string_expressions);
    RUN_TEST(test_array_literals);
    RUN_TEST(test_table_literals);
    RUN_TEST(test_index_expressions);
    RUN_TEST(test_calling_functions_without_arguments);
    RUN_TEST(test_functions_without_return_value);
    RUN_TEST(test_first_class_functions);
    RUN_TEST(test_calling_functions_without_bindings);
    RUN_TEST(test_calling_functions_with_bindings);
    RUN_TEST(test_calling_functions_with_wrong_arguments);
    RUN_TEST(test_builtin_functions);
    RUN_TEST(test_closures);
    RUN_TEST(test_recursive_functions);
    RUN_TEST(test_recursive_fibonacci);
    RUN_TEST(test_assign_expressions);
    return UNITY_END();
}
