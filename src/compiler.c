#include "compiler.h"
#include "ast.h"
#include "builtin.h"
#include "code.h"
#include "constants.h"
#include "symbol_table.h"
#include "utils.h"
#include "vm.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int add_constant(Compiler *c, Constant obj_const);

// allocate new [SymbolTable] and add ptr to [c.symbol_tables]
static SymbolTable *new_symbol_table(Compiler *c);

DEFINE_BUFFER(Scope, CompilationScope);
DEFINE_BUFFER(SymbolTable, SymbolTable *);

void compiler_init(Compiler *c) {
    memset(c, 0, sizeof(Compiler));

    // init global symbol table
    SymbolTable *global_symbol_table = new_symbol_table(c);
    symbol_table_init(global_symbol_table);
    c->cur_symbol_table = global_symbol_table;

    // add builtin functions to global symbol table
    const Builtins *builtin = get_builtins();
    int i = 0;
    while (builtin->name != NULL) {
        sym_builtin(global_symbol_table, i, builtin->name);
        builtin++;
        i++;
    }

    // init main scope
    ScopeBufferPush(&c->scopes, (CompilationScope){});
    c->current_instructions = &c->scopes.data[0].instructions;
}

void compiler_free(Compiler *c) {
    free(c->scopes.data[0].instructions.data); // main scope is not copied to
                                               // [c.constants]
    free(c->scopes.data);

    SymbolTable *cur;
    for (int i = 0; i < c->symbol_tables.length; i++) {
        cur = c->symbol_tables.data[i];
        symbol_table_free(cur);
        free(cur);
    }
    free(c->symbol_tables.data);

    for (int i = 0; i < c->constants.length; i++) {
        free_constant(c->constants.data[i]);
    }
    free(c->constants.data);
}

static bool
last_instruction_is(Compiler *c, Opcode op) {
    if (c->current_instructions->length == 0) {
        return false;
    }
    return c->scopes.data[c->scope_index].last_instruction.opcode == op;
}

static void
remove_last_pop(Compiler *c) {
    CompilationScope *cur_scope = c->scopes.data + c->scope_index;
    cur_scope->instructions.length = cur_scope->last_instruction.position;
    cur_scope->last_instruction = cur_scope->previous_instruction;
}

static void
replace_instruction(Compiler *c, int pos, Instructions new) {
    uint8_t* ins = c->current_instructions->data;
    for (int i = 0; i < new.length; i++) {
        ins[pos + i] = new.data[i];
    }
}

static void
replace_last_pop_with_return(Compiler *c) {
    CompilationScope *cur_scope = c->scopes.data + c->scope_index;
    int last_pos = cur_scope->last_instruction.position;
    Instructions new = make(OpReturnValue);
    replace_instruction(c, last_pos, new);
    free(new.data);

    cur_scope->last_instruction.opcode = OpReturnValue;
}

static void
change_operand(Compiler *c, int op_pos, int operand) {
    Opcode op = c->current_instructions->data[op_pos];
    Instructions new = make(op, operand);
    replace_instruction(c, op_pos, new);
    free(new.data);
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

static error
_compile(Compiler *c, Node n) {
    error err;
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
                ExpressionStatement *es = n.obj;
                err = _compile(c, es->expression);
                if (err) { return err; }

                emit(c, OpPop);
                return 0;
            }

        case n_LetStatement:
            {
                LetStatement *ls = n.obj;

                Symbol *symbol = sym_define(c->cur_symbol_table, ls->name->tok.literal);
                if (symbol->index >= GlobalsSize) {
                    return new_error("too many global variables");
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
                if (c->scope_index == 0) {
                    return new_error("return statement outside function");
                }

                ReturnStatement *rs = n.obj;
                err = _compile(c, rs->return_value);
                if (err) { return err; }

                emit(c, OpReturnValue);
                return 0;
            }

        case n_Identifier:
            {
                Identifier *id = n.obj;
                Symbol *symbol = sym_resolve(c->cur_symbol_table, id->tok.literal);
                if (symbol == NULL) {
                    return new_error("undefined variable '%s'", id->tok.literal);
                }

                load_symbol(c, symbol);
                return 0;
            }

        case n_InfixExpression:
            {
                InfixExpression *ie = n.obj;
                if ('<' == ie->op[0]) {
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

                if ('+' == ie->op[0]) {
                    emit(c, OpAdd);

                } else if ('-' == ie->op[0]) {
                    emit(c, OpSub);

                } else if ('*' == ie->op[0]) {
                    emit(c, OpMul);

                } else if ('/' == ie->op[0]) {
                    emit(c, OpDiv);

                } else if ('>' == ie->op[0]) {
                    emit(c, OpGreaterThan);

                } else if (!strcmp("==", ie->op)) {
                    emit(c, OpEqual);

                } else if (!strcmp("!=", ie->op)) {
                    emit(c, OpNotEqual);

                } else {
                    return new_error("unknown operator %s", ie->op);
                }
                return 0;
            }

        case n_PrefixExpression:
            {
                PrefixExpression *pe = n.obj;
                err = _compile(c, pe->right);
                if (err) { return err; }

                if ('!' == pe->op[0]) {
                    emit(c, OpBang);

                } else if ('-' == pe->op[0]) {
                    emit(c, OpMinus);

                } else {
                    return new_error("unknown operator %s", pe->op);
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

        case n_HashLiteral:
            {
                PairBuffer pairs = ((HashLiteral *)n.obj)->pairs;
                for (int i = 0; i < pairs.length; i++) {
                    err = _compile(c, pairs.data[i].key);
                    if (err) { return err; }

                    err = _compile(c, pairs.data[i].val);
                    if (err) { return err; }
                }
                emit(c, OpHash, pairs.length * 2);
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
                StringConstant *str = malloc(sizeof(StringConstant));
                if (str == NULL) { die("malloc"); }
                str->data = sl->tok.literal;
                str->length = strlen(sl->tok.literal);

                Constant str_const = {
                    .type = c_String,
                    .data = { .string = str }
                };
                emit(c, OpConstant, add_constant(c, str_const));
                return 0;
            }

        case n_FunctionLiteral:
            {
                FunctionLiteral *fl = n.obj;
                enter_scope(c);

                if (fl->name) {
                    sym_function_name(c->cur_symbol_table, fl->name);
                }

                ParamBuffer params = fl->params;
                for (int i = 0; i < params.length; i++) {
                    sym_define(c->cur_symbol_table, params.data[i]->tok.literal);
                }

                err = _compile(c, NODE(n_BlockStatement, fl->body));
                if (err) { return err; }

                if (last_instruction_is(c, OpPop)) {
                    replace_last_pop_with_return(c);
                }
                if (!last_instruction_is(c, OpReturnValue)) {
                    emit(c, OpReturn);
                }

                SymbolBuffer free_symbols =
                    c->cur_symbol_table->free_symbols;
                int num_locals =
                    c->cur_symbol_table->num_definitions;
                Instructions *ins = leave_scope(c);

                // load all free_symbols into symbol table of function
                for (int i = 0; i < free_symbols.length; i++) {
                    load_symbol(c, free_symbols.data[i]);
                }

                Function *fn = malloc(sizeof(Function));
                if (fn == NULL) { die("malloc"); }
                *fn = (Function) {
                    .instructions = *ins,
                    .num_locals = num_locals,
                    .num_parameters = params.length,
                };
                Constant fn_const = {
                    .type = c_Function,
                    .data = { .function = fn }
                };
                emit(c, OpClosure, add_constant(c, fn_const),
                        free_symbols.length);
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
    CompilationScope *cur_scope = c->scopes.data + c->scope_index;
    cur_scope->previous_instruction = cur_scope->last_instruction;
    cur_scope->last_instruction =
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
    int new_scope_idx = ++c->scope_index;

    // allocate new scope if necessary.
    ScopeBufferPush(&c->scopes, (CompilationScope){});
    c->current_instructions =
        &c->scopes.data[new_scope_idx].instructions;

    SymbolTable *new = new_symbol_table(c);
    enclosed_symbol_table(new, c->cur_symbol_table);
    c->cur_symbol_table = new;
}

// return to previous compilation scope and symbol table
Instructions *leave_scope(Compiler *c) {
    // instructions is copied into [c.constants] as [Function]
    Instructions *ins = c->current_instructions;

    int prev_scope_idx = --c->scope_index;
    c->current_instructions =
        &c->scopes.data[prev_scope_idx].instructions;
    c->scopes.length--; // remove from [c.scopes], to be replaced at next
                        // [enter_scope()] call.

    c->cur_symbol_table = c->cur_symbol_table->outer;
    return ins;
}

error compile(Compiler *c, Program *prog) {
    error err;
    for (int i = 0; i < prog->stmts.length; i++) {
        err = _compile(c, prog->stmts.data[i]);
        if (err) { return err; }
    }
    return 0;
}

Bytecode bytecode(Compiler *c) {
    return (Bytecode){
        .instructions = c->current_instructions,
        .constants = &c->constants,

        // global symbol table
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
