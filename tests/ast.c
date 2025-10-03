#include "unity/unity.h"

#include "../src/token.h"
#include "../src/ast.h"

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
                .tok = { t_Let, 0, 0, "let" },
                .name =
                    &(Identifier){
                        .tok = { t_Ident, 0, 0, "myVar" },
                    },
                .value =
                    {
                        n_Identifier,
                        &(Identifier){
                            .tok = { t_Ident, 0, 0, "anotherVar" },
                        },
                    }
            }
        }
    };
    Program prog = {{ stmts, 1, 0 }};

    char* expected = "let myVar = anotherVar;";
    size_t len = strlen(expected) + 2; // in case of stupidity
    char* buf = calloc(len, sizeof(char));

    // `open_memstream` would be better
    FILE* fp = fmemopen(buf, len, "w");
    if (fp == NULL) {
        fprintf(stderr, "no memory");
        exit(1);
    }

    TEST_ASSERT_MESSAGE(program_fprint(&prog, fp) != -1, "program_fprint fail");
    fflush(fp);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            expected, buf, "program_fprint wrong");

    fclose(fp);
    free(buf);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_print);
    return UNITY_END();
}
