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
    if (p->errors.length >= MAX_ERRORS)
        fprintf(out, "too many errors, stopping now\n");
}

BUFFER(Program, Program);
DEFINE_BUFFER(Program, Program);

void repl(FILE* in, FILE* out) {
    ProgramBuffer progs;
    ProgramBufferInit(&progs);
    Env* env = env_new();

    Parser p;
    p.l = NULL;
    while (1) {
        fprintf(out, ">> ");
        char* input = NULL;
        size_t len = 0;
        if (getline(&input, &len, in) == -1) {
            free(input);
            break;
        }

        Lexer l = lexer_new(input);
        if (p.l == NULL)
            parser_init(&p, &l);
        else {
            p.l = &l;
            StringBufferInit(&p.errors);
            p.cur_token = lexer_next_token(&l);
            p.peek_token = lexer_next_token(&l);
        }

        Program prog = parse_program(&p);
        ProgramBufferPush(&progs, prog);

        if (p.errors.length > 0) {
            print_parser_errors(out, &p);
            free(input);
            parser_destroy(&p);
            p.errors.data = NULL;
            continue;
        }

        Object* evaluated = eval_program(&prog, env);
        if (evaluated->typ != o_Null) {
            object_fprint(evaluated, out);
            fprintf(out, "\n");
        }

        free(input);
        parser_destroy(&p);
    }

    env_destroy(env);
    for (int i = 0; i < progs.length; i++)
        program_destroy(&progs.data[i]);
    free(progs.data);
}

void run(char* program) {
    Lexer l = lexer_new(program);
    Parser p;
    parser_init(&p, &l);
    Program prog = parse_program(&p);
    if (p.errors.length > 0) {
        print_parser_errors(stdout, &p);
        parser_destroy(&p);
        return;
    }

    Env* env = env_new();
    eval_program(&prog, env);

    parser_destroy(&p);
    program_destroy(&prog);
    env_destroy(env);
}
