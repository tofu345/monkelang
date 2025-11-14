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
                .name =
                    &(Identifier){
                        .tok = {
                            .type = t_Ident,
                            .start = "myVar",
                            .length = 5
                        },
                    },
                .value =
                    {
                        n_Identifier,
                        &(Identifier){
                            .tok = {
                                .type = t_Ident,
                                .start = "anotherVar",
                                .length = 10
                            },
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

    int err = program_fprint(&prog, fp);
    if (err == -1) {
        printf("program_fprint fail");
        goto cleanup;
    }

    fflush(fp);
    if (strcmp(expected, buf) != 0) {
        printf("program wrong.\nwant=%s\n got =%s", expected, buf);
        err = -1;
    }

cleanup:
    fclose(fp);
    free(buf);

    TEST_ASSERT(err != -1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_print);
    return UNITY_END();
}
