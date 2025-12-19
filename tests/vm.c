#include "unity/unity.h"
#include "helpers.h"

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

    vm_test_error("1 < 1.", "unkown operation: integer < float");
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
    vm_test("if (1 > 2) { 10 }", TEST_NOTHING);
    vm_test("if (false) { 10 }", TEST_NOTHING);
    vm_test("!(if (false) { 5; })", TEST(bool, true));
    vm_test("if ((if (false) { 10 })) { 10 } else { 20 }", TEST(int, 20));
}

// Truthy:
// - boolean true
// - numbers not 0 (C-like)
// - arrays and tables with more than one element
// - everything else except nothing.
static void
test_truthy(void) {
    vm_test("if (true) {1}", TEST(int, 1));
    vm_test("if (1) {1}", TEST(int, 1));
    vm_test("if (1.5) {1}", TEST(int, 1));
    vm_test("if (-1) {1}", TEST(int, 1));
    vm_test("if (-1.5) {1}", TEST(int, 1));
    vm_test("if ([1]) {1}", TEST(int, 1));
    vm_test("if ({1: 2}) {1}", TEST(int, 1));
    vm_test("if (fn(){}) {1}", TEST(int, 1));

    vm_test("if (false) {1}", TEST_NOTHING);
    vm_test("if (0) {1}", TEST_NOTHING);
    vm_test("if ([]) {1}", TEST_NOTHING);
    vm_test("if ({}) {1}", TEST_NOTHING);
    vm_test("if (nothing) {1}", TEST_NOTHING);
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

// create NULL-terminated (0-terminated) array of integers.
static IntArray make_int_array(int n, ...);
#define TEST_ARR(...) TEST(arr, make_int_array(__VA_ARGS__, 0))

static void
test_array_literals(void) {
    vm_test("[]", TEST_ARR(0));
    vm_test("[1, 2, 3]", TEST_ARR(1, 2, 3));
    vm_test("[1 + 2, 3 * 4, 5 + 6]", TEST_ARR(3, 12, 11));

    vm_test("[] * 3", TEST_ARR(0));
    vm_test("[2] * 1", TEST_ARR(2));
    vm_test("[1, 2] * 3", TEST_ARR(1, 2, 1, 2, 1, 2));
}

#define TEST_TABLE(...) \
    &(Test){ test_table, { ._arr = make_int_array(__VA_ARGS__, 0) }}

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
    vm_test("[][0]", TEST_NOTHING);
    vm_test("[1, 2, 3][99]", TEST_NOTHING);
    vm_test("[1][-1]", TEST_NOTHING);
    vm_test("{1: 1, 2: 2}[1]", TEST(int, 1));
    vm_test("{1: 1, 2: 2.1}[2]", TEST(float, 2.1));
    vm_test("{1: 1}[0]", TEST_NOTHING);
    vm_test("{}[0]", TEST_NOTHING);
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
        TEST_NOTHING
    );
    vm_test(
        "\
            let noReturn = fn() { };\
            let noReturnTwo = fn() { noReturn(); };\
            noReturn();\
            noReturnTwo();\
        ",
        TEST_NOTHING
    );

    vm_test_error(
        "let infRec = fn() { infRec(); }; infRec()",
        "exceeded maximum function call stack"
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
            };\
            let minusTwo = fn() {\
                let num = 2;\
                globalSeed - num;\
            };\
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
            };\
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
    vm_test("puts(\"hello\", \"world!\")", TEST_NOTHING);
    vm_test("first([1, 2, 3])", TEST(int, 1));
    vm_test("first([])", TEST_NOTHING);
    vm_test("last([1, 2, 3])", TEST(int, 3));
    vm_test("last([])", TEST_NOTHING);
    vm_test("rest([1, 2, 3])", TEST_ARR(2, 3));
    vm_test("rest([])", TEST_ARR(0));
    vm_test("push([], 1)", TEST_ARR(1));
    vm_test(
        "\
        let arr = [1, 2];\
        let arr2 = copy(arr);\
        push(arr2, 3);\
        arr\
        ",
        TEST_ARR(1, 2)
    );
    vm_test("type([])", TEST(str, "array"));
    vm_test("type({})", TEST(str, "table"));
    vm_test("type(1) != type(1.0)", TEST(bool, true));

    vm_test_error("len(1)", "builtin len(): argument of integer not supported");
    vm_test_error("len(\"one\", \"two\")", "builtin len() takes 1 argument got 2");
    vm_test_error("first(1)", "builtin first(): argument of integer not supported");
    vm_test_error("last(1)", "builtin last(): argument of integer not supported");
    vm_test_error("push(1, 1)", "builtin push() expects first argument to be array got integer");
    vm_test_error("type(1, 1)", "builtin type() takes 1 argument got 2");
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
        let newAdderInner = newAdderOuter(1, 2);\
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
        let newAdderInner = newAdderOuter(2);\
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
test_assignments(void) {
    vm_test("let a = 15; a = 5; a", TEST(int, 5));
    vm_test("let a; a = 5;", TEST(int, 5));

    // assign index expressions
    vm_test("let arr = [1, 2, 3]; arr[0] = 5; arr[0]", TEST(int, 5));

    // table key removal
    vm_test(
        "\
        let hash = {1: 2, 3: 4};\
        hash[1] = nothing;\
        hash\
        ",
        TEST_TABLE(3, 4)
    );

    // assign free variable
    vm_test(
        "\
        let a = 66;\
        fn() {\
            let b = 77;\
            fn() {\
                b = 88;\
                let c = 99;\
                a + b + c;\
            }() + b\
        }()\
        ",
        TEST(int, 330)
    );

    vm_test("let var = 1; var += 3;", TEST(int, 4));
    vm_test(
        "\
        let var = [1];\
        var[0] += 2;\
        var[0];\
        ",
        TEST(int, 3)
    );
    vm_test(
        "\
        let var = {1: 2};\
        var[1] += 2;\
        var;\
        ",
        TEST_TABLE(1, 4)
    );

    vm_test_error(
        "let var = []; var[0] += 2;",
        "unkown operation: nothing + integer"
    );
    vm_test_error(
        "let var = {1: 2}; var[2] += 2;",
        "unkown operation: nothing + integer"
    );
}

static void
test_free_variable_list(void) {
    // should have an effect
    vm_test(
        "\
        fn() {\
            let arr = [];\
            fn() { push(arr, 1); }();\
            arr\
        }()\
        ",
        TEST_ARR(1)
    );
    vm_test(
        "\
        fn() {\
            let arr = [1, 2, 3];\
            fn() { arr[0] = 4; }();\
            arr\
        }()\
        ",
        TEST_ARR(4, 2, 3)
    );
    vm_test(
        "\
        fn() {\
            let tbl = {1: 2, 3: 4};\
            fn() { tbl[1] = true; }();\
            tbl[1]\
        }()\
        ",
        TEST(bool, true)
    );

    // should have no effect
    vm_test(
        "\
        fn() {\
            let arr = [];\
            fn() { arr = [1]; }();\
            arr\
        }()\
        ",
        TEST_ARR(0) // empty
    );
    vm_test(
        "\
        fn() {\
            let num = 1;\
            fn() { num = 2; }();\
            num\
        }()\
        ",
        TEST(int, 1)
    );
}

static void
test_for_statement(void) {
    vm_test(
        "\
        let a = 5;\
        for (let b = 1; b < 5; b += 1) {\
            a = a + b;\
        };\
        a\
        ",
        TEST(int, 15)
    );
    vm_test(
        "\
        let a = 0;\
        for (; a < 5; a += 1) {};\
        a\
        ",
        TEST(int, 5)
    );
    vm_test(
        "\
        let a = 0;\
        for (; a < 5; ) {\
            a += 1;\
        };\
        a\
        ",
        TEST(int, 5)
    );
    vm_test(
        "\
        fn() {\
            let a = 0;\
            for (;;) {\
                if (a < 5) {\
                    a += 1;\
                } else {\
                    return a;\
                }\
            }\
        }()\
        ",
        TEST(int, 5)
    );
}

static IntArray
make_int_array(int n, ...) {
    IntArray arr = {0};

    if (n == 0) { return arr; }

    int capacity = 0;
    va_list ap;
    va_start(ap, n);
    do {
        if (arr.length == capacity) {
            capacity = power_of_2_ceil(arr.length + 1);
            arr.data = realloc(arr.data, capacity * sizeof(int));
            if (arr.data == NULL) die("realloc");
        }

        arr.data[arr.length++] = n;
        n = va_arg(ap, int);
    } while (n != 0);
    va_end(ap);

    return arr;
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

        case test_nothing:
            if (!expect_object_is(o_Nothing, actual)) {
                return -1;
            }
            return 0;

        case test_arr:
            {
                if (!expect_object_is(o_Array, actual)) {
                    return -1;
                }

                ObjectBuffer *arr = actual.data.ptr;
                IntArray exp_arr = expected.val._arr;

                if (exp_arr.length != arr->length) {
                    printf("wrong number of elements. want=%d, got=%d\n",
                            exp_arr.length, arr->length);
                    return -1;
                }

                int err;
                for (int i = 0; i < exp_arr.length; i++) {
                    err = test_integer_object(exp_arr.data[i], arr->data[i]);
                    if (err != 0) {
                        printf("test array element %d: test_integer_object failed", i);
                        return -1;
                    }
                }
                free(expected.val._arr.data);
                return 0;
            }

        case test_table:
            {
                if (!expect_object_is(o_Table, actual)) {
                    return -1;
                }

                Table *tbl = actual.data.table;
                IntArray exp_pairs = expected.val._arr;

                size_t num_pairs = exp_pairs.length / 2;
                if (num_pairs != tbl->length) {
                    printf("wrong number of elements. want=%zu, got=%zu\n",
                            num_pairs, tbl->length);
                    return -1;
                }

                // test `tbl[expected key]` == `expected value`
                int err;
                for (int i = 0; i + 1 < exp_pairs.length; i += 2) {
                    Object value =
                        table_get(tbl, OBJ(o_Integer, exp_pairs.data[i]));

                    if (value.type == o_Nothing) {
                        printf("no pair for key %d (%d)\n", i, exp_pairs.data[i]);
                        return -1;
                    }

                    err = test_integer_object(exp_pairs.data[i + 1], value);
                    if (err != 0) {
                        printf("test table element %d (%d) failed\n",
                                i, exp_pairs.data[i + 1]);
                        return -1;
                    }
                }

                free(exp_pairs.data);
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
    bool fail = false;

    Program prog = test_parse(input);

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    error err = compile(&c, &prog);
    if (err) {
        print_error(err, stdout);

        fail = true;
        goto cleanup;
    };

    // // display constants
    // for (int i = 0; i < c.constants.length; i++) {
    //     Constant cn = c.constants.data[i];
    //     printf("CONSTANT %d ", i);
    //     switch (cn.type) {
    //         case c_Function:
    //             {
    //                 CompiledFunction *fn = cn.data.function;
    //                 FunctionLiteral *lit = fn->literal;
    //                 if (lit->name) {
    //                     printf("%.*s", LITERAL(lit->tok));
    //                 } else {
    //                     printf("<anonymous function>");
    //                 }
    //                 puts(" Instructions:");
    //                 fprint_instructions(stdout, fn->instructions);
    //             }
    //             break;
    //         case c_Integer:
    //             printf("Value: %ld\n", cn.data.integer);
    //             break;
    //         case c_Float:
    //             printf("Value: %f\n", cn.data.floating);
    //             break;
    //         case c_String:
    //             printf("Value: \"%.*s\"\n",
    //                     cn.data.string->length, cn.data.string->start);
    //             break;
    //     }
    // }
    // putc('\n', stdout);

    err = vm_run(&vm, bytecode(&c));
    if (err) {
        printf("vm error: %s\n", err->message);

        fail = true;
        goto cleanup;
    }

    if (vm.sp != 0) {
        printf("stack pointer not zero got %d\n", vm.sp);

        fail = true;
        goto cleanup;
    }

    Object stack_elem = vm_last_popped(&vm);
    fail = test_expected_object(*expected, stack_elem) == -1;

cleanup:
    free_error(err);
    vm_free(&vm);
    compiler_free(&c);
    program_free(&prog);

    if (fail) {
        printf("in test: '%s'\n\n", input);
        TEST_FAIL();
    }
}

static void
vm_test_error(char *input, char *expected_error) {
    bool fail = false;

    Program prog = test_parse(input);

    Compiler c;
    compiler_init(&c);

    error err = compile(&c, &prog);
    if (err) {
        print_error(err, stdout);
        fail = true;
        goto cleanup;
    };

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    err = vm_run(&vm, bytecode(&c));
    if (!err) {
        printf("expected VM error for test: %s\n", input);
        fail = true;

    } else if (strcmp(err->message, expected_error) != 0) {
        printf("wrong VM error\nwant= %s\ngot = %s\n",
                expected_error, err->message);
        fail = true;
    }

    vm_free(&vm);

cleanup:
    free_error(err);
    compiler_free(&c);
    program_free(&prog);

    if (fail) {
        printf("in test: '%s'\n\n", input);
        TEST_FAIL();
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_integer_arithmetic);
    RUN_TEST(test_boolean_expressions);
    RUN_TEST(test_conditionals);
    RUN_TEST(test_truthy);
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
    RUN_TEST(test_assignments);
    RUN_TEST(test_free_variable_list);
    RUN_TEST(test_for_statement);
    return UNITY_END();
}
