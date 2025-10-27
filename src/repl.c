#include "repl.h"
#include "compiler.h"
#include "errors.h"
#include "object.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_parser_errors(FILE* out, Parser *p);

BUFFER(Program, Program)
DEFINE_BUFFER(Program, Program)

void repl(FILE* in, FILE* out) {
    Parser p;
    parser_init(&p);
    Program prog;
    ProgramBuffer programs = {0};

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    char *input;

    error err;
    Error *e;

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

        // remove trailing newline
        len = strlen(input);
        if (input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        prog = parse(&p, input);
        if (p.errors.length > 0) {
            print_parser_errors(out, &p);
            program_free(&prog);
            goto cleanup;

        } else if (prog.stmts.length > 0) {
            ProgramBufferPush(&programs, prog);
        }

        e = compile(&c, &prog);
        if (e) {
            fprintf(out, "Woops! Compilation failed!\n");
            print_error(input, e);
            free_error(e);
            goto cleanup;
        }

        err = vm_run(&vm, bytecode(&c));
        if (err) {
            fprintf(out, "Woops! Executing bytecode failed!\n");
            puts(err);
            free(err);
            goto cleanup;
        }

        stack_elem = vm_last_popped(&vm);
        object_fprint(stack_elem, out);
        fputc('\n', out);

cleanup:
        free(input);
        c.scopes.data[0].instructions.length = 0; // reset main scope
        vm.stack[0] = (Object){0}; // remove stack_elem
        vm.sp = 0;
    }

    vm_free(&vm);
    for (int i = 0; i < programs.length; i++)
        program_free(&programs.data[i]);
    free(programs.data);
    parser_free(&p);
    compiler_free(&c);
}

void run(char* program) {
    Parser p;
    Program prog;
    Compiler c;
    VM vm;
    error err;
    Error *e;

    parser_init(&p);
    compiler_init(&c);
    vm_init(&vm, NULL, NULL, NULL);

    prog = parse(&p, program);
    if (p.errors.length > 0) {
        print_parser_errors(stdout, &p);
        goto cleanup;
    }

    e = compile(&c, &prog);
    if (e) {
        fprintf(stdout, "Woops! Compilation failed!\n");
        print_error(program, e);
        free_error(e);
        goto cleanup;
    }

    err = vm_run(&vm, bytecode(&c));
    if (err) {
        fprintf(stdout, "Woops! Executing bytecode failed!\n");
        puts(err);
        free(err);
        goto cleanup;
    }

cleanup:
    vm_free(&vm);
    program_free(&prog);
    parser_free(&p);
    compiler_free(&c);
}

static void
print_parser_errors(FILE* out, Parser *p) {
    fprintf(out, "Woops! We ran into some monkey business here!\n");
    for (int i = 0; i < p->errors.length; i++) {
        Error err = p->errors.data[i];
        print_error(p->l.input, &err);
        free(err.message);
    }
    p->errors.length = 0;
}
