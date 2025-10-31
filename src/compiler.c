#include "compiler.h"
#include "ast.h"
#include "builtin.h"
#include "code.h"
#include "constants.h"
#include "errors.h"
#include "object.h"
#include "symbol_table.h"
#include "utils.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int add_constant(Compiler *c, Constant obj_const);

// malloc [SymbolTable], add to [c.symbol_tables].
static SymbolTable *new_symbol_table(Compiler *c);

// calloc new [CompiledFunction].
static CompiledFunction *new_function();

// append new [SourceMapping] with [c.current_instructions.length] and
// [stmt.token] to [c.cur_scope.mappings].
static void source_map_statement(Compiler *c, Node stmt);

// create [Error] with [n.token]
static Error *compiler_error(Node n, char* format, ...);

DEFINE_BUFFER(Scope, CompilationScope)
DEFINE_BUFFER(SymbolTable, SymbolTable *)
DEFINE_BUFFER(Function, CompiledFunction *)

void compiler_init(Compiler *c) {
    memset(c, 0, sizeof(Compiler));

    // init global symbol table
    SymbolTable *global_symbol_table = new_symbol_table(c);
    symbol_table_init(global_symbol_table);
    c->current_symbol_table = global_symbol_table;

    // add builtin functions to global symbol table
    const Builtin *builtin = get_builtins();
    int i = 0;
    while (builtin->name != NULL) {
        sym_builtin(global_symbol_table, i, builtin->name);
        builtin++;
        i++;
    }

    // main function
    CompiledFunction *main_fn = new_function();

    ScopeBufferPush(&c->scopes, (CompilationScope){
        .function = main_fn
    });
    c->cur_scope = &c->scopes.data[0];
    c->current_instructions = &main_fn->instructions;
}

void compiler_free(Compiler *c) {
    int i;

    // free main function and functions that failed compilation.
    for (i = 0; i < c->scopes.length; i++) {
        free_function(c->scopes.data[i].function);
    }
    free(c->scopes.data);

    for (i = 0; i < c->symbol_tables.length; i++) {
        SymbolTable *cur = c->symbol_tables.data[i];
        symbol_table_free(cur);
        free(cur);
    }
    free(c->symbol_tables.data);

    free(c->constants.data);

    for (i = 0; i < c->functions.length; i++) {
        free_function(c->functions.data[i]);
    }
    free(c->functions.data);
}

static bool
last_instruction_is(Compiler *c, Opcode op) {
    if (c->current_instructions->length == 0) {
        return false;
    }
    return c->cur_scope->last_instruction.opcode == op;
}

static void
append_return_if_not_present(Compiler *c) {
    Opcode last = c->cur_scope->last_instruction.opcode;
    if (last == OpReturn || last == OpReturnValue) {
        return;
    }

    emit(c, OpReturn);
}

static void
remove_last_pop(Compiler *c) {
    c->current_instructions->length =
        c->cur_scope->last_instruction.position;
    c->cur_scope->last_instruction = c->cur_scope->previous_instruction;
}

static void
replace_instruction(Compiler *c, int pos, Instructions new) {
    uint8_t* ins = c->current_instructions->data;
    for (int i = 0; i < new.length; i++) {
        ins[pos + i] = new.data[i];
    }
    free(new.data);
}

static void
replace_last_opcode_with(Compiler *c, Opcode old, Opcode new) {
    int last_pos = c->cur_scope->last_instruction.position;

    assert(c->current_instructions->data[last_pos] == old);

    replace_instruction(c, last_pos, make(new));
    c->cur_scope->last_instruction.opcode = new;
}

static void
replace_last_pop_with_return(Compiler *c) {
    replace_last_opcode_with(c, OpPop, OpReturnValue);
}

static void
change_operand(Compiler *c, int op_pos, int operand) {
    Opcode op = c->current_instructions->data[op_pos];
    replace_instruction(c, op_pos, make(op, operand));
}

static void
load_symbol(Compiler *c, Symbol *s) {
    switch (s->scope) {
        case GlobalScope:
            emit(c, OpGetGlobal, s->index);
            break;

        case LocalScope:
            emit(c, OpGetLocal, s->index);
            break;

        case FunctionScope:
            emit(c, OpCurrentClosure);
            break;

        case BuiltinScope:
            emit(c, OpGetBuiltin, s->index);
            break;

        case FreeScope:
            emit(c, OpGetFree, s->index);
            break;
    }
}

static Error *
_compile(Compiler *c, Node n) {
    Error *err;
    switch (n.typ) {
        case n_BlockStatement:
            {
                BlockStatement* bs = n.obj;
                for (int i = 0; i < bs->stmts.length; i++) {
                    err = _compile(c, bs->stmts.data[i]);
                    if (err) { return err; }
                }
                return 0;
            }

        case n_ExpressionStatement:
            {
                source_map_statement(c, n);

                ExpressionStatement *es = n.obj;
                err = _compile(c, es->expression);
                if (err) { return err; }

                emit(c, OpPop);
                return 0;
            }

        case n_LetStatement:
            {
                source_map_statement(c, n);

                LetStatement *ls = n.obj;

                Identifier *ident = ls->name;
                char *name = (char *)ident->tok.start;
                Symbol *symbol =
                    sym_define(c->current_symbol_table, name, ident->hash);

                if (symbol->index >= GlobalsSize) {
                    return compiler_error(
                            (Node){.obj = ident}, "too many global variables");
                }

                err = _compile(c, ls->value);
                if (err) { return err; }

                if (symbol->scope == GlobalScope) {
                    emit(c, OpSetGlobal, symbol->index);
                } else {
                    emit(c, OpSetLocal, symbol->index);
                }
                return 0;
            }

        case n_ReturnStatement:
            {
                source_map_statement(c, n);

                if (c->cur_scope_index == 0) {
                    return compiler_error(n,
                            "return statement outside function");
                }

                ReturnStatement *rs = n.obj;

                if (rs->return_value.obj == NULL) {
                    emit(c, OpReturn);

                } else {
                    err = _compile(c, rs->return_value);
                    if (err) { return err; }

                    emit(c, OpReturnValue);
                }
                return 0;
            }

        case n_Identifier:
            {
                Identifier *id = n.obj;
                Symbol *symbol = sym_resolve(c->current_symbol_table, id->hash);
                if (symbol == NULL) {
                    return compiler_error(n,
                            "undefined variable '%.*s'", LITERAL(id->tok));
                }

                load_symbol(c, symbol);
                return 0;
            }

        case n_AssignStatement:
            {
                source_map_statement(c, n);

                AssignStatement *stmt = n.obj;

                err = _compile(c, stmt->right);
                if (err) { return err; }

                if (stmt->left.typ == n_Identifier) {
                    Identifier *id = stmt->left.obj;
                    Node id_node = { .obj = id };

                    Symbol *symbol = sym_resolve(c->current_symbol_table, id->hash);
                    if (symbol == NULL) {
                        return compiler_error(id_node,
                                "undefined variable '%.*s' is not assignable",
                                LITERAL(id->tok));
                    }

                    char *format;
                    switch (symbol->scope) {
                        case GlobalScope:
                            emit(c, OpSetGlobal, symbol->index);
                            return 0;

                        case LocalScope:
                            emit(c, OpSetLocal, symbol->index);
                            return 0;

                        case FreeScope:
                            emit(c, OpSetFree, symbol->index);
                            return 0;

                        case FunctionScope:
                            format = "function '%.*s' is not assignable";
                            break;

                        case BuiltinScope:
                            format = "builtin %.*s() is not assignable";
                            break;
                    }

                    return compiler_error(id_node, format, LITERAL(id->tok));

                } else if (stmt->left.typ == n_IndexExpression) {
                    err = _compile(c, stmt->left);
                    if (err) { return err; }

                    replace_last_opcode_with(c, OpIndex, OpSetIndex);
                    return 0;
                }

                return compiler_error(stmt->left,
                        "cannot assign non-variable");
            }

        case n_InfixExpression:
            {
                InfixExpression *ie = n.obj;
                if ('<' == ie->tok.start[0]) {
                    err = _compile(c, ie->right);
                    if (err) { return err; }

                    err = _compile(c, ie->left);
                    if (err) { return err; }

                    emit(c, OpGreaterThan);
                    return 0;
                }

                err = _compile(c, ie->left);
                if (err) { return err; }

                err = _compile(c, ie->right);
                if (err) { return err; }

                if ('+' == ie->tok.start[0]) {
                    emit(c, OpAdd);

                } else if ('-' == ie->tok.start[0]) {
                    emit(c, OpSub);

                } else if ('*' == ie->tok.start[0]) {
                    emit(c, OpMul);

                } else if ('/' == ie->tok.start[0]) {
                    emit(c, OpDiv);

                } else if ('>' == ie->tok.start[0]) {
                    emit(c, OpGreaterThan);

                } else if (!strncmp("==", ie->tok.start, 2)) {
                    emit(c, OpEqual);

                } else if (!strncmp("!=", ie->tok.start, 2)) {
                    emit(c, OpNotEqual);

                } else {
                    die("compiler: unknown infix operator %.*s",
                            LITERAL(ie->tok));
                }
                return 0;
            }

        case n_PrefixExpression:
            {
                PrefixExpression *pe = n.obj;
                err = _compile(c, pe->right);
                if (err) { return err; }

                if ('!' == pe->tok.start[0]) {
                    emit(c, OpBang);

                } else if ('-' == pe->tok.start[0]) {
                    emit(c, OpMinus);

                } else {
                    die("compiler: unknown prefix operator %.*s",
                            LITERAL(pe->tok));
                }
                return 0;
            }

        case n_IfExpression:
            {
                IfExpression *ie = n.obj;
                err = _compile(c, ie->condition);
                if (err) { return err; }

                // Emit an `OpJumpNotTruthy` with a bogus value
                int jump_not_truthy_pos = emit(c, OpJumpNotTruthy, 9999);

                err = _compile(c, NODE(n_BlockStatement, ie->consequence));
                if (err) { return err; }

                if (last_instruction_is(c, OpPop)) {
                    remove_last_pop(c);
                }

                // TODO: when if branch ends in ReturnStatement, do not
                // emit OpJump

                // Emit an `OpJump` with a bogus value
                int jump_pos = emit(c, OpJump, 9999);

                int after_consequence_pos = c->current_instructions->length;
                change_operand(c, jump_not_truthy_pos, after_consequence_pos);

                if (ie->alternative == NULL) {
                    emit(c, OpNull);
                } else {
                    err = _compile(c, NODE(n_BlockStatement, ie->alternative));
                    if (err) { return err; }

                    if (last_instruction_is(c, OpPop)) {
                        remove_last_pop(c);
                    }
                }

                int after_alternative_pos = c->current_instructions->length;
                change_operand(c, jump_pos, after_alternative_pos);
                return 0;
            }

        case n_IndexExpression:
            {
                IndexExpression *ie = n.obj;
                err = _compile(c, ie->left);
                if (err) { return err; }

                err = _compile(c, ie->index);
                if (err) { return err; }

                emit(c, OpIndex);
                return 0;
            }

        case n_CallExpression:
            {
                CallExpression *ce = n.obj;
                err = _compile(c, ce->function);
                if (err) { return err; }

                NodeBuffer args = ce->args;
                for (int i = 0; i < args.length; i++) {
                    err = _compile(c, args.data[i]);
                    if (err) { return err; }
                }

                emit(c, OpCall, args.length);
                return 0;
            }

        case n_NullLiteral:
            emit(c, OpNull);
            return 0;

        case n_ArrayLiteral:
            {
                NodeBuffer elems = ((ArrayLiteral *)n.obj)->elements;
                for (int i = 0; i < elems.length; i++) {
                    err = _compile(c, elems.data[i]);
                    if (err) { return err; }
                }

                emit(c, OpArray, elems.length);
                return 0;
            }

        case n_TableLiteral:
            {
                PairBuffer pairs = ((TableLiteral *)n.obj)->pairs;
                for (int i = 0; i < pairs.length; i++) {
                    err = _compile(c, pairs.data[i].key);
                    if (err) { return err; }

                    err = _compile(c, pairs.data[i].val);
                    if (err) { return err; }
                }
                emit(c, OpTable, pairs.length * 2);
                return 0;
            }

        case n_IntegerLiteral:
            {
                IntegerLiteral *il = n.obj;
                Constant integer = {
                    .type = c_Integer,
                    .data = { .integer = il->value }
                };
                emit(c, OpConstant, add_constant(c, integer));
                return 0;
            }

        case n_FloatLiteral:
            {
                FloatLiteral *il = n.obj;
                Constant floating = {
                    .type = c_Float,
                    .data = { .floating = il->value }
                };
                emit(c, OpConstant, add_constant(c, floating));
                return 0;
            }

        case n_BooleanLiteral:
            {
                BooleanLiteral *bl = n.obj;
                if (bl->value) {
                    emit(c, OpTrue);
                } else {
                    emit(c, OpFalse);
                }
                return 0;
            }

        case n_StringLiteral:
            {
                StringLiteral *sl = n.obj;
                Constant str_const = {
                    .type = c_String,
                    .data = { .string = &sl->tok }
                };
                emit(c, OpConstant, add_constant(c, str_const));
                return 0;
            }

        case n_FunctionLiteral:
            {
                FunctionLiteral *fl = n.obj;

                enter_scope(c);

                if (fl->name) {
                    sym_function_name(c->current_symbol_table,
                            fl->name->tok.start, fl->name->hash);
                }

                ParamBuffer params = fl->params;
                Identifier *cur;
                for (int i = 0; i < params.length; i++) {
                    cur = params.data[i];
                    sym_define(c->current_symbol_table, cur->tok.start,
                            cur->hash);
                }

                err = _compile(c, NODE(n_BlockStatement, fl->body));
                if (err) { return err; }

                if (last_instruction_is(c, OpPop)) {
                    replace_last_pop_with_return(c);
                } else {
                    append_return_if_not_present(c);
                }

                CompiledFunction *fn = c->cur_scope->function;
                fn->num_locals = c->current_symbol_table->num_definitions;
                fn->num_parameters = params.length;

                // add to list of compiled functions
                FunctionBufferPush(&c->functions, fn);

                SymbolBuffer free_symbols =
                    c->current_symbol_table->free_symbols;

                leave_scope(c);

                // push free_symbols onto the stack, to go into [Closure].
                for (int i = 0; i < free_symbols.length; i++) {
                    load_symbol(c, free_symbols.data[i]);
                }

                Constant fn_const = {
                    .type = c_Function,
                    .data = { .function = fn }
                };
                int const_index = add_constant(c, fn_const);
                emit(c, OpClosure, const_index, free_symbols.length);
                return 0;
            }

        default:
            return 0;
    }
}

static int
add_constant(Compiler *c, Constant obj_const) {
    ConstantBufferPush(&c->constants, obj_const);
    return c->constants.length - 1;
}

static void
set_last_instruction(Compiler *c, Opcode op, int pos) {
    c->cur_scope->previous_instruction = c->cur_scope->last_instruction;
    c->cur_scope->last_instruction =
        (EmittedInstruction){ .opcode = op, .position = pos };
}

int emit(Compiler *c, Opcode op, ...) {
    va_list operands;
    va_start(operands, op);
    int pos = make_valist_into(c->current_instructions, op, operands);
    va_end(operands);
    set_last_instruction(c, op, pos);
    return pos;
}

// create new compilation scope and enclosed symbol table
void enter_scope(Compiler *c) {
    // allocate new [CompiledFunction]
    CompiledFunction *fn = new_function();

    int new_scope_idx = ++c->cur_scope_index;

    CompilationScope new_scope = {
        .function = fn
    };
    ScopeBufferPush(&c->scopes, new_scope);

    c->cur_scope = &c->scopes.data[new_scope_idx];
    c->current_instructions = &fn->instructions;

    SymbolTable *new = new_symbol_table(c);
    enclosed_symbol_table(new, c->current_symbol_table);
    c->current_symbol_table = new;
}

// return to previous compilation scope and symbol table, and return
// current scope [Instructions]
void leave_scope(Compiler *c) {
    int prev_scope_idx = --c->cur_scope_index;

    c->cur_scope = &c->scopes.data[prev_scope_idx];
    c->current_instructions = &c->cur_scope->function->instructions;

    // remove from [c.scopes], to be replaced at next [enter_scope()]
    // call.
    --c->scopes.length;

    c->current_symbol_table = c->current_symbol_table->outer;
}

Error *compile(Compiler *c, Program *prog) {
    Error *err;
    for (int i = 0; i < prog->stmts.length; i++) {
        err = _compile(c, prog->stmts.data[i]);
        if (err) { return err; }
    }
    return 0;
}

Bytecode bytecode(Compiler *c) {
    return (Bytecode) {
        .main_function = c->cur_scope->function,
        .constants = &c->constants,
        .num_globals = c->symbol_tables.data[0]->num_definitions,
    };
}

static SymbolTable *
new_symbol_table(Compiler *c) {
    SymbolTable *new = malloc(sizeof(SymbolTable));
    if (new == NULL) { die("malloc"); }
    SymbolTableBufferPush(&c->symbol_tables, new);
    return new;
}

static CompiledFunction *
new_function() {
    CompiledFunction *fn = calloc(1, sizeof(CompiledFunction));
    if (fn == NULL) { die("malloc"); }
    return fn;
}

static void
source_map_statement(Compiler *c, Node stmt) {
    SourceMapping mapping = {
        .position = c->current_instructions->length,
        .statement = stmt
    };
    // TODO: [c.cur_function]
    SourceMappingBufferPush(&c->cur_scope->function->mappings, mapping);
}

static Error *
compiler_error(Node n, char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) die("compiler_error");
    va_end(args);

    Error *err = malloc(sizeof(Error));
    *err = (Error){
        .token = *node_token(n),
        .message = msg
    };
    return err;
}
