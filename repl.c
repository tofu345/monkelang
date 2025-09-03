#include "ast.h"
#include "environment.h"
#include "object.h"
#include "parser.h"
#include "lexer.h"
#include "evaluator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_parser_errors(FILE* out, Parser* p) {
    fprintf(out, "Woops! We ran into some monkey business here!\n");
    // fprintf(out, "parser errors:\n");
    for (int i = 0; i < p->errors.length; i++) {
        fprintf(out, "%s\n", p->errors.data[i]);
    }
}

void start(FILE* in, FILE* out) {
    Lexer l = {};
    Parser p;
    parser_init(&p, &l);
    Program prog;
    NodeBufferInit(&prog.stmts);
    int current_statement = 0;
    Env* env = env_new();

    while (1) {
        fprintf(out, ">> ");
        char* input = NULL;
        size_t len = 0;
        if (getline(&input, &len, in) == -1) {
            free(input);
            break;
        }

        lexer_init(&l, input);
        p.cur_token = lexer_next_token(&l);
        p.peek_token = lexer_next_token(&l);
        parse_program_into(&p, &prog);

        if (p.errors.length != 0) {
            print_parser_errors(out, &p);
            for (int i = 0; i < p.errors.length; i++)
                free(p.errors.data[i]);
            p.errors.length = 0;
            continue;
        }

        Object evaluated = eval_program_from(&prog, current_statement, env);
        current_statement = prog.stmts.length;
        if (evaluated.typ != o_Null) {
            object_fprint(evaluated, out);
            fprintf(out, "\n");
            object_destroy(&evaluated);
        }
        free(input);
    }

    env_destroy(env);
    parser_destroy(&p);
    program_destroy(&prog);
}
