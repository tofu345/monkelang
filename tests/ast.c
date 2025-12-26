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
                .tok = {
                    .type = t_Let,
                    .start = "let",
                    .length = 3
                },
                .names = {
                    .data = (void*[]) {
                        &(Identifier){
                            .tok = {
                                .type = t_Ident,
                                .start = "myVar",
                                .length = 5
                            },
                        },
                    },
                    .length = 1,
                },
                .values = {
                    .data = (Node[]) {
                        n_Identifier,
                        &(Identifier){
                            .tok = {
                                .type = t_Ident,
                                .start = "anotherVar",
                                .length = 10
                            },
                        },
                    },
                    .length = 1,
                }
            }
        }
    };
    char* expected = "let myVar = anotherVar;";

    Program prog = { .stmts = { .data = stmts, .length = 1 } };

    char *buf = NULL;
    size_t len;
    FILE *fp = open_memstream(&buf, &len);
    TEST_ASSERT_NOT_NULL_MESSAGE(fp, "open_memstream fail");

    int err = program_fprint(&prog, fp);
    TEST_ASSERT_EQUAL_MESSAGE(0, err, "program_fprint fail");

    fflush(fp);
    if (strcmp(expected, buf) != 0) {
        printf("program wrong.\nwant=%s\n got =%s", expected, buf);
        TEST_FAIL();
    }

    fclose(fp);
    free(buf);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_print);
    return UNITY_END();
}
