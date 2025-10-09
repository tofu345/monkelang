#include "compiler.h"
#include "object.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
print_errors(FILE* out, ErrorBuffer *buf) {
    for (int i = 0; i < buf->length; i++) {
        fprintf(out, "%s\n", buf->data[i]);
    }
    if (buf->length >= MAX_ERRORS)
        fprintf(out, "too many errors, stopping now\n");
}

static void
free_errors(ErrorBuffer *errs) {
    for (int i = 0; i < errs->length; i++) {
        free(errs->data[i]);
    }
    errs->length = 0;
}

BUFFER(Program, Program);
DEFINE_BUFFER(Program, Program);

void repl(FILE* in, FILE* out) {
    Parser p;
    parser_init(&p);
    Program prog;
    ProgramBuffer progs = {};

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    int err;
    while (1) {
        fprintf(out, ">> ");
        char *input = NULL;
        size_t len = 0;
        if (getline(&input, &len, in) == -1) {
            free(input);
            break;
        }

        prog = parse(&p, input);
        ProgramBufferPush(&progs, prog);
        if (p.errors.length > 0) {
            fprintf(out, "Woops! We ran into some monkey business here!\n");
            print_errors(out, &p.errors);
            goto cleanup;
        }

        err = compile(&c, &prog);
        if (err != 0) {
            fprintf(out, "Woops! Compilation failed:\n");
            print_errors(out, &c.errors);
            goto cleanup;
        }

        vm_with(&vm, bytecode(&c));
        err = vm_run(&vm);
        if (err != 0) {
            fprintf(out, "Woops! Executing bytecode failed:\n");
            print_errors(out, &vm.errors);
            goto cleanup;
        }

        Object stack_elem = vm_last_popped(&vm);
        object_fprint(stack_elem, out);
        fputc('\n', out);
        vm.stack[0] = (Object){}; // remove stack_elem

cleanup:
        free(input);
        free(p.peek_token.literal);
        p.peek_token.literal = NULL;
        free_errors(&p.errors);
        free_errors(&c.errors);
        c.instructions.length = 0;
        free_errors(&vm.errors);
    }

    for (int i = 0; i < progs.length; i++)
        program_free(&progs.data[i]);
    free(progs.data);
    parser_free(&p);
    compiler_free(&c);
    vm_free(&vm);
}
