#include "ast.h"
#include "compiler.h"
#include "object.h"
#include "parser.h"
#include "lexer.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
print_errors(FILE* out, StringBuffer *buf) {
    for (int i = 0; i < buf->length; i++) {
        fprintf(out, "%s\n", buf->data[i]);
    }
    if (buf->length >= MAX_ERRORS)
        fprintf(out, "too many errors, stopping now\n");
}

BUFFER(Program, Program);
DEFINE_BUFFER(Program, Program);

void repl(FILE* in, FILE* out) {
    Program prog;
    ProgramBuffer progs;
    ProgramBufferInit(&progs);
    Parser p;
    p.l = NULL;
    Compiler c;
    VM vm;

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

        prog = parse_program(&p);
        ProgramBufferPush(&progs, prog);
        if (p.errors.length > 0) {
            fprintf(out, "Woops! We ran into some monkey business here!\n");
            print_errors(out, &p.errors);
            free(input);
            parser_destroy(&p);
            continue;
        }
        free(input);

        c = (Compiler){};
        int err = compile(&c, &prog);
        if (err != 0) {
            fprintf(out, "Woops! Compilation failed:\n");
            print_errors(out, &c.errors);
            compiler_destroy(&c);
            continue;
        }

        vm_init(&vm, bytecode(&c));
        err = vm_run(&vm);
        if (err != 0) {
            fprintf(out, "Woops! Executing bytecode failed:\n");
            print_errors(out, &vm.errors);
            vm_destroy(&vm);
            continue;
        }

        Object stack_elem = vm_last_popped(&vm);
        object_fprint(&stack_elem, out);
        fputc('\n', out);

        parser_destroy(&p);
        compiler_destroy(&c);
        vm_destroy(&vm);
    }

    for (int i = 0; i < progs.length; i++)
        program_destroy(&progs.data[i]);
    free(progs.data);
}
