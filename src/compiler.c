#include "compiler.h"
#include "ast.h"
#include "code.h"
#include "constants.h"
#include "symbol_table.h"
#include "utils.h"
#include "vm.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

static int add_constant(Compiler *c, Constant obj_const);
static SymbolTable *allocate_symbol_table(Compiler *c);

DEFINE_BUFFER(Scope, CompilationScope);

void compiler_init(Compiler *c) {
    memset(c, 0, sizeof(Compiler));

    SymbolTable *global_symbol_table = allocate_symbol_table(c);
    symbol_table_init(global_symbol_table);
    c->cur_symbol_table = global_symbol_table;

    CompilationScope main_scope = {};
    ScopeBufferPush(&c->scopes, main_scope);
    c->current_instructions = &c->scopes.data[0].instructions;
}

void compiler_free(Compiler *c) {
    for (int i = 0; i < c->errors.length; i++) {
        free(c->errors.data[i]);
    }
    free(c->errors.data);

    free(c->scopes.data[0].instructions.data); // main scope
    free(c->scopes.data);

    for (int i = 0; i < c->symbol_tables.length; i++) {
        symbol_table_free(c->symbol_tables.data + i);
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

static int
_compile(Compiler *c, Node n) {
    switch (n.typ) {
        case n_BlockStatement:
            {
                BlockStatement* bs = n.obj;
                for (int i = 0; i < bs->stmts.length; i++) {
                    if (_compile(c, bs->stmts.data[i])) {
                        return -1;
                    }
                }
                return 0;
            }

        case n_ExpressionStatement:
            {
                ExpressionStatement *es = n.obj;
                if (_compile(c, es->expression)) {
                    return -1;
                }

                emit(c, OpPop);
                return 0;
            }

        case n_LetStatement:
            {
                LetStatement *ls = n.obj;
                if (_compile(c, ls->value)) {
                    return -1;
                }

                Symbol *symbol = sym_define(c->cur_symbol_table, ls->name->tok.literal);
                if (symbol->index >= GlobalsSize) { die("too many globals"); }

                if (symbol->scope == GlobalScope) {
                    emit(c, OpSetGlobal, symbol->index);
                } else {
                    emit(c, OpSetLocal, symbol->index);
                }
                return 0;
            }

        case n_ReturnStatement:
            {
                ReturnStatement *rs = n.obj;
                if (_compile(c, rs->return_value)) {
                    return -1;
                }

                emit(c, OpReturnValue);
                return 0;
            }

        case n_Identifier:
            {
                Identifier *id = n.obj;
                Symbol *symbol = sym_resolve(c->cur_symbol_table, id->tok.literal);
                if (symbol == NULL) {
                    error(&c->errors, "undefined variable '%s'", id->tok.literal);
                    return -1;
                }

                if (symbol->scope == GlobalScope) {
                    emit(c, OpGetGlobal, symbol->index);
                } else {
                    emit(c, OpGetLocal, symbol->index);
                }
                return 0;
            }

        case n_InfixExpression:
            {
                InfixExpression *ie = n.obj;
                if ('<' == ie->op[0]) {
                    if (_compile(c, ie->right)) {
                        return -1;
                    }

                    if (_compile(c, ie->left)) {
                        return -1;
                    }

                    emit(c, OpGreaterThan);
                    return 0;
                }

                if (_compile(c, ie->left)) {
                    return -1;
                }

                if (_compile(c, ie->right)) {
                    return -1;
                }

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
                    error(&c->errors, "unknown operator %s", ie->op);
                    return -1;
                }
                return 0;
            }

        case n_PrefixExpression:
            {
                PrefixExpression *pe = n.obj;
                if (_compile(c, pe->right)) {
                    return -1;
                }

                if ('!' == pe->op[0]) {
                    emit(c, OpBang);

                } else if ('-' == pe->op[0]) {
                    emit(c, OpMinus);

                } else {
                    error(&c->errors, "unknown operator %s", pe->op);
                    return -1;
                }
                return 0;
            }

        case n_IfExpression:
            {
                IfExpression *ie = n.obj;
                if (_compile(c, ie->condition)) {
                    return -1;
                }

                // Emit an `OpJumpNotTruthy` with a bogus value
                int jump_not_truthy_pos = emit(c, OpJumpNotTruthy, 9999);

                if (_compile(c, NODE(n_BlockStatement, ie->consequence))) {
                    return -1;
                }
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
                    if (_compile(c, NODE(n_BlockStatement, ie->alternative))) {
                        return -1;
                    }
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
                if (_compile(c, ie->left)) {
                    return -1;
                }

                if (_compile(c, ie->index)) {
                    return -1;
                }

                emit(c, OpIndex);
                return 0;
            }

        case n_CallExpression:
            {
                CallExpression *ce = n.obj;
                if (_compile(c, ce->function)) {
                    return -1;
                }

                NodeBuffer args = ce->args;
                for (int i = 0; i < args.length; i++) {
                    if (_compile(c, args.data[i])) {
                        return -1;
                    }
                }

                emit(c, OpCall, args.length);
                return 0;
            }

        case n_ArrayLiteral:
            {
                NodeBuffer elems = ((ArrayLiteral *)n.obj)->elements;
                for (int i = 0; i < elems.length; i++) {
                    if (_compile(c, elems.data[i])) {
                        return -1;
                    }
                }

                emit(c, OpArray, elems.length);
                return 0;
            }

        case n_HashLiteral:
            {
                PairBuffer pairs = ((HashLiteral *)n.obj)->pairs;
                for (int i = 0; i < pairs.length; i++) {
                    if (_compile(c, pairs.data[i].key)) {
                        return -1;
                    }
                    if (_compile(c, pairs.data[i].val)) {
                        return -1;
                    }
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

                ParamBuffer params = fl->params;
                for (int i = 0; i < params.length; i++) {
                    sym_define(c->cur_symbol_table, params.data[i]->tok.literal);
                }

                if (_compile(c, NODE(n_BlockStatement, fl->body))) {
                    return -1;
                }

                if (last_instruction_is(c, OpPop)) {
                    replace_last_pop_with_return(c);
                }
                if (!last_instruction_is(c, OpReturnValue)) {
                    emit(c, OpReturn);
                }

                // lots of redirection
                int num_locals = c->cur_symbol_table->store->length;

                Instructions *ins = leave_scope(c);
                CompiledFunction *fn = malloc(sizeof(CompiledFunction));
                if (fn == NULL) { die("malloc"); }

                *fn = (CompiledFunction) {
                    .instructions = *ins,
                    .num_locals = num_locals,
                    .num_parameters = params.length,
                };
                Constant compiled_fn = {
                    .type = c_Instructions,
                    .data = { .function = fn }
                };
                emit(c, OpConstant, add_constant(c, compiled_fn));
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

void enter_scope(Compiler *c) {
    CompilationScope scope = {};
    ScopeBufferPush(&c->scopes, scope);
    c->current_instructions =
        &c->scopes.data[++c->scope_index].instructions;

    SymbolTable *new = allocate_symbol_table(c);
    enclosed_symbol_table(new, c->cur_symbol_table);
    c->cur_symbol_table = new;
}

Instructions *leave_scope(Compiler *c) {
    Instructions *ins = c->current_instructions;
    c->current_instructions =
        &c->scopes.data[--c->scope_index].instructions;
    c->scopes.length--;
    c->cur_symbol_table = c->cur_symbol_table->outer;
    return ins;
}

int compile(Compiler *c, Program *prog) {
    for (int i = 0; i < prog->stmts.length; i++) {
        if (_compile(c, prog->stmts.data[i])) {
            return -1;
        }
    }
    return 0;
}

Bytecode bytecode(Compiler *c) {
    return (Bytecode){
        .instructions = c->current_instructions,
        .constants = &c->constants,
    };
}

static SymbolTable *
allocate_symbol_table(Compiler *c) {
    SymbolTables *buf = &c->symbol_tables;
    buf->length++;
    if (buf->length > buf->capacity) {
        int capacity = power_of_2_ceil(buf->length);
        buf->data = realloc(buf->data, capacity * sizeof(SymbolTable));
        if (buf->data == NULL) { die("allocate_symbol_table"); }
        buf->capacity = capacity;
    }
    return buf->data + buf->length - 1;
}
