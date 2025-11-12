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

// A successfully compiled and run evaluation.
typedef struct {
    char *source;
    int len;

    Program program;

    // main function
    CompiledFunction *function;
} Eval;

BUFFER(Eval, Eval)
DEFINE_BUFFER(Eval, Eval)

static void free_eval(Eval *current);
static void print_parser_errors(FILE* out, Parser *p);

// getline() but if first line ends with '{' or '(', read and append multiple
// lines to [input] until a blank line is encountered and there is no more data
// to read in [in_stream].
static int multigetline(char **input, size_t *input_cap,
                        FILE *in_stream, FILE *out_stream);

void repl(FILE* in_stream, FILE* out_stream) {
    Parser p;
    parser_init(&p);

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    EvalBuffer done = {0};
    Eval eval = {0};
    bool success = false;

    size_t cap = 0;
    while (1) {
        eval.len = multigetline(&eval.source, &cap, in_stream, out_stream);
        if (eval.len == -1) {
            free(eval.source);
            break;

        // only '\n' was entered.
        } else if (eval.len == 1) {
            free(eval.source);
            eval.source = NULL;
            continue;
        }

        eval.program = parse_(&p, eval.source, eval.len);
        if (p.errors.length > 0) {
            print_parser_errors(out_stream, &p);
            goto cleanup;

        } else if (eval.program.stmts.length == 0) {
            free(eval.source);
            eval.source = NULL;
            continue;
        }

        eval.function = c.cur_scope->function;
        Error *e = compile(&c, &eval.program);
        if (e) {
            fprintf(out_stream, "Woops! Compilation failed!\n");
            print_error(e);
            free_error(e);
            goto cleanup;
        }

        error err = vm_run(&vm, bytecode(&c));
        if (err) {
            fprintf(out_stream, "Woops! Executing bytecode failed!\n");
            print_vm_stack_trace(&vm);
            puts(err);
            free(err);
            goto cleanup;
        }

        Object stack_elem = vm_last_popped(&vm);
        if (stack_elem.type != o_Null) {
            object_fprint(stack_elem, out_stream);
            fputc('\n', out_stream);
        }

        EvalBufferPush(&done, eval);
        success = true;

cleanup:
        if (!success) {
            free_eval(&eval);
        }
        eval = (Eval){0};
        success = false;

        // free unsuccessfully [CompiledFunctions]
        for (; c.cur_scope_index > 0; --c.cur_scope_index) {
            free_function(c.scopes.data[c.cur_scope_index].function);
        }
        c.scopes.length = 1;

        // new main function
        CompiledFunction *main_fn = new_function();
        c.scopes.data[0] = (CompilationScope){ .function = main_fn };
        c.cur_scope = c.scopes.data;
        c.current_instructions = &main_fn->instructions;
        c.cur_mapping = &main_fn->mappings;

        // reset VM
        vm.sp = 0;
        vm.frames_index = 0;
        vm.stack[0] = (Object){0}; // remove stack_elem
    }

    vm_free(&vm);
    compiler_free(&c);
    parser_free(&p);
    for (int i = 0; i < done.length; i++) {
        free_eval(done.data + i);
    }
    free(done.data);
}

void run(char* program) {
    Parser p;
    Program prog;
    Compiler c;
    VM vm;

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

    Error *e = compile(&c, &prog);
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
    compiler_free(&c);
    program_free(&prog);
    parser_free(&p);
}

static void
free_eval(Eval *eval) {
    free(eval->source);

    program_free(&eval->program);

    if (eval->function) {
        free_function(eval->function);
    }
}

static void
print_parser_errors(FILE* out_stream, Parser *p) {
    fprintf(out_stream, "Woops! We ran into some monkey business here!\n");
    for (int i = 0; i < p->errors.length; i++) {
        Error err = p->errors.data[i];
        print_error(&err);
        free(err.message);
    }
    p->errors.length = 0;
}

static int
multigetline(char **input, size_t *input_cap,
             FILE *in_stream, FILE *out_stream) {

    fprintf(out_stream, ">> ");

    // [len] is number of chars read.
    int len = getline(input, input_cap, in_stream);
    if (len == -1) { return -1; }

    if (len == 1) {
        // remove ending '\n'
        (*input)[0] = '\0';
        return len;
    }

    // last character before '\n'
    char last_th = (*input)[len - 2];
    if (last_th != '{' && last_th != '(') { return len; }

    char *line = NULL;
    size_t line_cap = 0,
           capacity = *input_cap;

    int ret;
    struct pollfd fds;
    fds.fd = fileno(in_stream);
    fds.events = POLLIN; // check if there is data to read.

    while (1) {
        // Skip printing '.. ' when a multiline string is pasted.
        ret = poll(&fds, 1, 0);
        // ret is 1 if there is data to read.
        if (ret != 1) {
            fprintf(out_stream, ".. ");
        }

        int line_len = getline(&line, &line_cap, in_stream);
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
