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
#include <sys/poll.h>

BUFFER(Input, char *)
DEFINE_BUFFER(Input, char *)
BUFFER(Program, Program)
DEFINE_BUFFER(Program, Program)

static void print_parser_errors(FILE* out, Parser *p);

// getline() but if first line ends with '{' or '(', read and append multiple
// lines to [input] until a blank line is encountered and there is no more data
// to read in [in].
int multigetline(char **input, size_t *input_cap, FILE *in, FILE *out);

// TODO: repl tests
void repl(FILE* in, FILE* out) {
    Parser p;
    parser_init(&p);
    Program prog;

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    Error *e;

    // TODO: append all inputs into one string and parse incrementally
    char *input;
    size_t cap;
    int len;

    Object stack_elem;

    // Keep previous inputs and ASTs around because tokens point to the source
    // code directly.
    InputBuffer inputs = {0};
    ProgramBuffer programs = {0};

    while (1) {
        fprintf(out, ">> ");
        input = NULL;
        cap = 0;
        len = multigetline(&input, &cap, in, out);
        if (len == -1) {
            free(input);
            break;

        // [input] only contains '\n'.
        } else if (len == 1) {
            free(input);
            continue;
        }

        prog = parse(&p, input);
        if (p.errors.length > 0) {
            print_parser_errors(out, &p);
            program_free(&prog);
            free(input);
            goto cleanup;
        } else if (prog.stmts.length == 0) {
            free(input);
            continue;
        }

        InputBufferPush(&inputs, input);
        ProgramBufferPush(&programs, prog);

        e = compile(&c, &prog);
        if (e) {
            fprintf(out, "Woops! Compilation failed!\n");
            print_error(e);
            free_error(e);
            goto cleanup;
        }

        error err = vm_run(&vm, bytecode(&c));
        if (err) {
            fprintf(out, "Woops! Executing bytecode failed!\n");
            print_vm_stack_trace(&vm);
            puts(err);
            free(err);
            goto cleanup;
        }

        stack_elem = vm_last_popped(&vm);
        if (stack_elem.type != o_Null) {
            object_fprint(stack_elem, out);
            fputc('\n', out);
        }

cleanup:
        // free unsuccessfully [CompiledFunction]s and reset main function
        // instructions.
        if (c.cur_scope_index > 0) {
            for (int i = c.cur_scope_index; i > 0; i--) {
                free_function(c.scopes.data[i].function);
            }
            c.cur_scope_index = 0;
            c.cur_scope = c.scopes.data;
            c.current_instructions = &c.cur_scope->function->instructions;
        }
        c.current_instructions->length = 0;
        // reset VM
        vm.sp = 0;
        vm.frames_index = 0;
        vm.stack[0] = (Object){0}; // remove stack_elem
    }

    vm_free(&vm);
    compiler_free(&c);

    for (int i = 0; i < programs.length; i++)
        program_free(&programs.data[i]);
    free(programs.data);
    parser_free(&p);

    for (int i = 0; i < inputs.length; i++)
        free(inputs.data[i]);
    free(inputs.data);
}

void run(char* program) {
    Parser p;
    Program prog;
    Compiler c;
    VM vm;
    Error *e;

    parser_init(&p);
    compiler_init(&c);
    vm_init(&vm, NULL, NULL, NULL);

    prog = parse(&p, program);
    if (p.errors.length > 0) {
        print_parser_errors(stdout, &p);
        goto cleanup;
    } else if (prog.stmts.length == 0) {
        goto cleanup;
    }

    e = compile(&c, &prog);
    if (e) {
        fprintf(stdout, "Woops! Compilation failed!\n");
        print_error(e);
        free_error(e);
        goto cleanup;
    }

    error err = vm_run(&vm, bytecode(&c));
    if (err) {
        fprintf(stdout, "Woops! Executing bytecode failed!\n");
        print_vm_stack_trace(&vm);
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
        print_error(&err);
        free(err.message);
    }
    p->errors.length = 0;
}

int multigetline(char **input, size_t *input_cap, FILE *in, FILE *out) {
    // [len] is number of chars read.
    int len = getline(input, input_cap, in);
    if (len == -1) { return -1; }

    // blank line, only '\n'
    if (len == 1) { return len; }

    // last character before '\n'
    char last_th = (*input)[len - 2];
    if (last_th != '{' && last_th != '(') { return len; }

    char *line = NULL;
    size_t line_cap = 0,
           capacity = *input_cap;

    int ret;
    struct pollfd fds;
    fds.fd = fileno(in); // fileno gets file descr of FILE *
    fds.events = POLLIN; // check if there is data to read.

    while (1) {
        // Skip printing '.. ' when a multiline string is pasted.
        ret = poll(&fds, 1, 0);
        // ret is 1 if there is data to read.
        if (ret != 1) {
            fprintf(out, ".. ");
        }

        int line_len = getline(&line, &line_cap, in);
        if (line_len == -1) {
            free(line);
            return -1;
        }

        // blank line and no data to read.
        if (line_len == 1 && ret != 1) {
            (*input)[len] = '\0';
            break;
        }

        // realloc [input] if necessary
        if ((size_t)(len + line_len) > capacity) {
            capacity *= 2;
            char *ptr = realloc(*input, capacity);
            if (ptr == NULL) {
                free(line);
                return -1;
            }
            *input = ptr;
            *input_cap = capacity;
        }

        // append new line to [input]
        strncpy((*input) + len, line, line_len);
        len += line_len;
    }

    free(line);
    return len;
}
