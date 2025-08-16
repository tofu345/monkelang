#include "unity/unity.h"
#include "token.h"
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void test_print(void) {
    Node stmts[] = {
        {
            n_LetStatement,
            &(LetStatement){
                .tok = { t_Let, "let" },
                .name =
                    &(Identifier){
                        .tok = { t_Ident, "myVar" },
                        .value = "myVar",
                    },
                .value =
                    {
                        n_Identifier,
                        &(Identifier){
                            .tok = { t_Ident, "anotherVar" },
                            .value = "anotherVar",
                        },
                    }
            }
        }
    };
    Program prog = { .statements = stmts, .len = 1, .cap = 1 };

    char* expected = "let myVar = anotherVar;";
    size_t len = strlen(expected) + 2; // in case of stupidity
    char buf[len];
    FILE* fp = fmemopen(buf, len, "w");
    if (fp == NULL) {
        fprintf(stderr, "no memory");
        exit(1);
    }

    program_print(&prog, fp);
    fflush(fp);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            expected, buf, "program_print wrong");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
            strlen(expected), strlen(buf), "program_print wrong len");

    fclose(fp);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_print);
    return UNITY_END();
}
