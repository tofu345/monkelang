#include "module.h"
#include "compiler.h"
#include "errors.h"
#include "object.h"
#include "shared.h"
#include "token.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void module_bytecode(Module *m, Bytecode code) {
    if (code.num_globals > m->num_globals) {
        m->globals = realloc(m->globals, code.num_globals * sizeof(Object));
        if (m->globals == NULL) { die("module_resize_globals:"); }
    }

    int added = code.num_globals - m->num_globals;
    if (added > 0) {
        Object *globals = m->globals + m->num_globals;
        memset(globals, 0, added * sizeof(Object));
    }
    m->num_globals = code.num_globals;

    m->main_function = code.main_function;
}

bool has_monke_suffix(const char *str, int len) {
    return len > 6
        && str[len - 6] == '.'
        && str[len - 5] == 'm'
        && str[len - 4] == 'o'
        && str[len - 3] == 'n'
        && str[len - 2] == 'k'
        && str[len - 1] == 'e';
}

// load source code at [filename] into `Module`
static error module_load(Module *module, const char *name, int length) {
    char *filename = malloc(length + 7);
    if (filename == NULL) { die("create_module - filename:"); }
    strncpy(filename, name, length);
    strncpy(&filename[length], ".monke", 7);
    module->name = filename;

    struct stat mstat;
    if (stat(filename, &mstat) != 0) {
        return errorf("could not open file '%s'", filename);
    }
    module->mtime = mstat.st_mtime;

    return load_file(filename, (char **) &module->source);
}

void module_free(Module *m) {
#ifdef DEBUG
    printf("unrequire: %s\n", m->name);
#endif

    free((char *) m->name);
    free((char *) m->source);
    free_function(m->main_function);
    program_free(&m->program);
    free(m->globals);
    free(m);
}

static error module_compile(VM *vm, Module *m) {
    bool success = false;

    // create dummy file that appends writes into [buf], to return as error.
    char *buf = NULL;
    size_t buf_len = 0;
    FILE *fp = open_memstream(&buf, &buf_len);
    if (fp == NULL) {
        return errorf("error loading module - %s", strerror(errno));
    }

    Lexer l;
    lexer_init(&l, m->source, strlen(m->source));

    Program program = {0};
    ParseErrorBuffer errors = parse(parser(), &l, &program);
    if (errors.length > 0) {
        fputs(parser_error_msg, fp);
        print_parse_errors(&errors, fp);
        program_free(&program);
        goto cleanup;
    }

    Compiler *c = vm->compiler;

    // Enter Independent Compilation Scope
    SymbolTable *prev = NULL;
    prev = c->cur_symbol_table;
    c->cur_symbol_table = NULL;
    enter_scope(c);

    error err = compile(c, &program, 0);
    if (err) {
        fputs(compiler_error_msg, fp);
        print_error(err, fp);
        free_error(err);
        compiler_reset(c);
        goto cleanup;
    }

    m->program = program;
    module_bytecode(m, bytecode(c));

    // Exit Independent Compilation Scope
    c->cur_symbol_table->outer = prev;
    SymbolTable *module_table = c->cur_symbol_table;
    leave_scope(c);
    symbol_table_free(module_table);

    success = true;

cleanup:
    fclose(fp);

    if (!success) {
        program_free(&program);
        return create_error(buf);
    }

    free(buf);
    return 0;
}

error require_module(VM *vm, Constant constant) {
    error err;

    if (constant.type != c_String) {
        die("require_module - filename constant of type %d is not String",
            constant.type);
    }

    const char *filename = constant.data.string->start;
    int len = constant.data.string->length;
    // remove ".monke" suffix, later added in module_load()
    if (has_monke_suffix(filename, len)) {
        len -= 6;
    }

#ifdef DEBUG
    printf("require: %.*s\n", len, filename);
#endif

    uint64_t hash = hash_fnv1a_(filename, len);
    Module *module = ht_get_hash(vm->modules, hash);
    if (errno == ENOKEY) {
        module = calloc(1, sizeof(Module));
        if (module == NULL) { die("require - create Module"); }

        err = module_load(module, filename, len);
        if (err) { goto fail; }

        err = module_compile(vm, module);
        if (err) { goto fail; }

        ht_set_hash(vm->modules, (void *) filename, module, hash);
        if (errno == ENOMEM) {
            err = errorf("error adding module - %s", strerror(errno));
            goto fail;
        }

    } else {
        struct stat mstat;
        if (stat(module->name, &mstat) != 0) {
            err = errorf("could not open file %s:", module->name);
            ht_remove(vm->modules, module->name);
            goto fail;
        }

        if (mstat.st_mtime > module->mtime) {
#ifdef DEBUG
            printf("file %s was modified, reloading...\n", module->name);
#endif

            free((char *) module->source);
            program_free(&module->program);
            free_function(module->main_function);
            module->source = NULL;
            module->main_function = NULL;

            err = load_file(module->name, (char **) &module->source);
            if (err) { goto fail; }

            err = module_compile(vm, module);
            if (err) { goto fail; }

            module->mtime = mstat.st_mtime;
        }
    }

#ifdef DEBUG
    printf("call: ");
    object_fprint(OBJ(o_Module, .module = module), stdout);
    putc('\n', stdout);
#endif

    module->parent = vm->cur_module;
    vm->cur_module = module;
    return 0;

fail:
    module_free(module);
    return err;
}

int fprint_module(Module *m, FILE *fp) {
    int len = strlen(m->name);
    if (has_monke_suffix(m->name, len)) {
        len -= 6;
    }
    FPRINTF(fp, "<module: %.*s>", len, m->name);
    return 0;
}
