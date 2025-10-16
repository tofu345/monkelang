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
        free(buf->data[i]);
    }
}

BUFFER(Program, Program);
DEFINE_BUFFER(Program, Program);

void repl(FILE* in, FILE* out) {
    Parser p;
    parser_init(&p);
    Program prog;
    ProgramBuffer programs = {};

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    char *input;
    error err;
    size_t len;
    Object stack_elem;
    while (1) {
        fprintf(out, ">> ");
        input = NULL;
        len = 0;
        if (getline(&input, &len, in) == -1) {
            free(input);
            break;
        }

        prog = parse(&p, input);
        if (p.errors.length > 0) {
            fprintf(out, "Woops! We ran into some monkey business here!\n");
            print_errors(out, &p.errors);
            program_free(&prog);
            goto cleanup;
        } else if (prog.stmts.length >= 1) {
            ProgramBufferPush(&programs, prog);
        }

        err = compile(&c, &prog);
        if (err != 0) {
            fprintf(out, "Woops! Compilation failed:\n");
            puts(err);
            free(err);
            goto cleanup;
        }

        vm_with(&vm, bytecode(&c));
        err = vm_run(&vm);
        if (err != 0) {
            fprintf(out, "Woops! Executing bytecode failed:\n");
            puts(err);
            free(err);
            goto cleanup;
        }

        stack_elem = vm_last_popped(&vm);
        object_fprint(stack_elem, out);
        fputc('\n', out);
        vm.stack[0] = (Object){}; // remove stack_elem

cleanup:
        free(input);
        free(p.peek_token.literal);
        p.peek_token.literal = NULL;
        p.errors.length = 0;
        c.scopes.data[0].instructions.length = 0; // reset main scope
    }

    for (int i = 0; i < programs.length; i++)
        program_free(&programs.data[i]);
    free(programs.data);
    parser_free(&p);
    compiler_free(&c);
    vm_free(&vm);
}
