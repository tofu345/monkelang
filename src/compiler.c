#include "compiler.h"
#include "ast.h"
#include "builtin.h"
#include "code.h"
#include "errors.h"
#include "object.h"
#include "symbol_table.h"
#include "table.h"
#include "utils.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// create [Error] with [n.token]
static error c_error(Node, char* format, ...);

// create `SourceMapping` to point to next emitted instruction and
// [Node.token].
static void source_map(Compiler *, Node);

static error perform_assignment(Compiler *, Node); // emit opcodes to assign to `Node`.

int add_constant(Compiler *, Constant);

static char *add_constant_err = "too many constant variables";

DEFINE_BUFFER(Constant, Constant)
DEFINE_BUFFER(Scope, CompilationScope)

void compiler_init(Compiler *c) {
    memset(c, 0, sizeof(Compiler));

    if (table_init(&c->constants_table) == NULL) {
        die("compiler_init - create constants table:");
    }
}

void compiler_free(Compiler *c) {
    for (int i = 0; i < c->scopes.length; ++i) {
        free_function(c->scopes.data[i].function);
    }
    free(c->scopes.data);

    SymbolTable *next, *cur = c->cur_symbol_table;
    while (cur) {
        next = cur->outer;
        symbol_table_free(cur);
        cur = next;
    }

    for (int i = 0; i < c->constants.length; i++) {
        Constant *constant = &c->constants.data[i];
        if (constant->type == c_Function) {
            free_function(constant->data.function);
        }
    }
    free(c->constants.data);
    table_free(&c->constants_table);

    memset(c, 0, sizeof(Compiler));
}

void compiler_reset(Compiler *c) {
    if (c->scopes.length > 0) {
        // free unsuccessfully `CompiledFunctions`
        for (; c->scopes.length > 1; --c->scopes.length) {
            free_function(c->scopes.data[c->scopes.length].function);
        }

        // set c.cur_* variables
        c->cur_scope = &c->scopes.data[0];
        CompiledFunction *function = c->cur_scope->function;
        c->cur_instructions = &function->instructions;
        c->cur_mappings = &function->mappings;
    }

    SymbolTable *next, *cur = c->cur_symbol_table;
    while (cur && cur->outer) {
        next = cur->outer;
        symbol_table_free(cur);
        cur = next;
    }
}

static uint64_t
hash(Identifier *id) {
    return hash_string_fnv1a(id->tok.start, id->tok.length);
}

static bool
last_instruction_is(Compiler *c, Opcode op) {
    if (c->cur_instructions->length == 0) {
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

// if the last statement is an Expression Statement, remove its OpPop.
//
// Otherwise emit an OpNull, because no other Statement should produce a value.
static void
remove_last_expression_stmt_pop(Compiler *c, NodeBuffer stmts) {
    if (last_instruction_is(c, OpPop)
            && stmts.length > 0
            && stmts.data[stmts.length - 1].typ == n_ExpressionStatement) {
        // remove last pop
        c->cur_instructions->length = c->cur_scope->last_instruction.position;
        c->cur_scope->last_instruction = c->cur_scope->previous_instruction;

    } else {
        emit(c, OpNothing);
    }
}

static void
replace_instruction(Compiler *c, int pos, Instructions new) {
    uint8_t* ins = c->cur_instructions->data;
    for (int i = 0; i < new.length; i++) {
        ins[pos + i] = new.data[i];
    }
    free(new.data);
}

static void
replace_last_opcode_with(Compiler *c, Opcode old, Opcode new) {
    int last_pos = c->cur_scope->last_instruction.position;

    assert(c->cur_instructions->data[last_pos] == old);

    c->cur_instructions->data[last_pos] = new;
    c->cur_scope->last_instruction.opcode = new;
}

static void
replace_last_pop_with_return(Compiler *c) {
    replace_last_opcode_with(c, OpPop, OpReturnValue);
}

static void
change_operand(Compiler *c, int op_pos, int operand) {
    Opcode op = c->cur_instructions->data[op_pos];
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

        case FreeScope:
            emit(c, OpGetFree, s->index);
            break;
    }
}

static void
compile_operator(Compiler *c, Token op) {
    if ('+' == op.start[0]) {
        emit(c, OpAdd);

    } else if ('-' == op.start[0]) {
        emit(c, OpSub);

    } else if ('*' == op.start[0]) {
        emit(c, OpMul);

    } else if ('/' == op.start[0]) {
        emit(c, OpDiv);

    } else if ('<' == op.start[0]) {
        emit(c, OpLessThan);

    } else if ('>' == op.start[0]) {
        emit(c, OpGreaterThan);

    } else if (!strncmp("==", op.start, 2)) {
        emit(c, OpEqual);

    } else if (!strncmp("!=", op.start, 2)) {
        emit(c, OpNotEqual);

    } else {
        die("compiler: unknown infix operator %.*s", LITERAL(op));
    }
}

static int get_builtin(Token *tok) {
    int len;
    const Builtin *builtin = get_builtins(&len);
    for (int i = 0; i < len; i++) {
        if (tok->length == builtin[i].name_len
                && strncmp(builtin[i].name, tok->start, tok->length) == 0) {
            return i;
        }
    }
    return -1;
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
                source_map(c, n);

                ExpressionStatement *es = n.obj;
                err = _compile(c, es->expression);
                if (err) { return err; }

                emit(c, OpPop);
                return 0;
            }

        case n_LetStatement:
            {
                LetStatement *ls = n.obj;

                for (int i = 0; i < ls->names.length; ++i) {
                    Identifier *id = ls->names.data[i];
                    source_map(c, NODE(n_Identifier, id));

                    Node value = ls->values.data[i];
                    if (value.obj) {
                        err = _compile(c, value);
                        if (err) { return err; }
                    } else {
                        emit(c, OpNothing);
                    }

                    Symbol *symbol =
                        sym_define(c->cur_symbol_table, &id->tok, hash(id));

                    if (symbol->index >= GlobalsSize) {
                        return c_error((Node){.obj = id},
                                "too many global variables");
                    }

                    if (symbol->scope == GlobalScope) {
                        emit(c, OpSetGlobal, symbol->index);
                    } else {
                        emit(c, OpSetLocal, symbol->index);
                    }
                }
                return 0;
            }

        case n_ReturnStatement:
            {
                source_map(c, n);

                if (c->scopes.length == 1) {
                    return c_error(n,
                            "return statement outside function or module");
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

        case n_ForStatement:
            {
                // 00 initilization statement
                // 01 condition expression
                // 02 JumpNotTruthy 06
                // 03 body
                // 04 update statement
                // 05 Jump 01
                // 06 ...

                source_map(c, n);

                ForStatement *fs = n.obj;

                err = _compile(c, fs->init);
                if (err) { return err; }

                int before_condition_pos = c->cur_instructions->length;

                if (fs->condition.obj != NULL) {
                    err = _compile(c, fs->condition);
                    if (err) { return err; }
                } else {
                    emit(c, OpTrue);
                }

                int jump_not_truthy_pos = emit(c, OpJumpNotTruthy, 9999);

                err = _compile(c, NODE(n_BlockStatement, fs->body));
                if (err) { return err; }

                err = _compile(c, fs->update);
                if (err) { return err; }

                emit(c, OpJump, before_condition_pos);

                int after_jump_pos = c->cur_instructions->length;
                change_operand(c, jump_not_truthy_pos, after_jump_pos);

                return 0;
            }

        case n_Identifier:
            {
                Identifier *id = n.obj;

                int builtin = get_builtin(&id->tok);
                if (builtin != -1) {
                    emit(c, OpGetBuiltin, builtin);
                    return 0;
                }

                Symbol *symbol = sym_resolve(c->cur_symbol_table, hash(id));
                if (symbol == NULL) {
                    return c_error(n, "undefined variable '%.*s'", LITERAL(id->tok));
                }

                load_symbol(c, symbol);
                return 0;
            }

        case n_Assignment:
            {
                Assignment *stmt = n.obj;

                err = _compile(c, stmt->right);
                if (err) { return err; }

                source_map(c, n);
                err = perform_assignment(c, stmt->left);
                if (err) { return err; }

                return 0;
            }

        case n_OperatorAssignment:
            {
                OperatorAssignment *stmt = n.obj;

                err = _compile(c, stmt->left);
                if (err) { return err; }

                err = _compile(c, stmt->right);
                if (err) { return err; }

                source_map(c, n);
                compile_operator(c, stmt->tok);

                source_map(c, n);
                err = perform_assignment(c, stmt->left);
                if (err) { return err; }

                return 0;
            }

        case n_InfixExpression:
            {
                InfixExpression *ie = n.obj;

                err = _compile(c, ie->left);
                if (err) { return err; }

                err = _compile(c, ie->right);
                if (err) { return err; }

                source_map(c, n);
                compile_operator(c, ie->tok);

                return 0;
            }

        case n_PrefixExpression:
            {
                PrefixExpression *pe = n.obj;
                err = _compile(c, pe->right);
                if (err) { return err; }

                source_map(c, n);

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

                remove_last_expression_stmt_pop(c, ie->consequence->stmts);

                // Emit an `OpJump` with a bogus value
                int jump_pos = emit(c, OpJump, 9999);

                int after_consequence_pos = c->cur_instructions->length;
                change_operand(c, jump_not_truthy_pos, after_consequence_pos);

                if (ie->alternative == NULL) {
                    emit(c, OpNothing);
                } else {
                    err = _compile(c, NODE(n_BlockStatement, ie->alternative));
                    if (err) { return err; }

                    remove_last_expression_stmt_pop(c, ie->alternative->stmts);
                }

                int after_alternative_pos = c->cur_instructions->length;
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

                source_map(c, n);

                emit(c, OpIndex);
                return 0;
            }

        case n_CallExpression:
            {
                CallExpression *ce = n.obj;

                NodeBuffer args = ce->args;
                for (int i = 0; i < args.length; i++) {
                    err = _compile(c, args.data[i]);
                    if (err) { return err; }
                }

                err = _compile(c, ce->function);
                if (err) { return err; }

                source_map(c, n);

                emit(c, OpCall, args.length);
                return 0;
            }

        case n_RequireExpression:
            {
                RequireExpression *re = n.obj;

                NodeBuffer args = re->args;
                if (args.length == 1 && args.data[0].typ == n_StringLiteral) {
                    err = _compile(c, args.data[0]);
                    if (err) { return err; }
                } else {
                    return c_error(n, "require() expects 1 argument of type string got %d",
                                   args.length);
                }

                source_map(c, n);

                replace_last_opcode_with(c, OpConstant, OpRequire);
                return 0;
            }

        case n_NothingLiteral:
            emit(c, OpNothing);
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
                int idx = add_constant(c, integer);
                if (idx == -1) { return c_error(n, add_constant_err); }

                emit(c, OpConstant, idx);
                return 0;
            }

        case n_FloatLiteral:
            {
                FloatLiteral *il = n.obj;
                Constant float_ = {
                    .type = c_Float,
                    .data = { .floating = il->value }
                };
                int idx = add_constant(c, float_);
                if (idx == -1) { return c_error(n, add_constant_err); }

                emit(c, OpConstant, idx);
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
                int idx = add_constant(c, str_const);
                if (idx == -1) { return c_error(n, add_constant_err); }

                emit(c, OpConstant, idx);
                return 0;
            }


        case n_FunctionLiteral:
            {
                FunctionLiteral *fl = n.obj;

                enter_scope(c);

                if (fl->name) {
                    sym_function_name(c->cur_symbol_table, &fl->name->tok,
                                      hash(fl->name));
                }

                Identifier *cur;
                for (int i = 0; i < fl->params.length; i++) {
                    cur = fl->params.data[i];
                    sym_define(c->cur_symbol_table, &cur->tok, hash(cur));
                }

                err = _compile(c, NODE(n_BlockStatement, fl->body));
                if (err) { return err; }

                if (last_instruction_is(c, OpPop)) {
                    replace_last_pop_with_return(c);
                } else {
                    append_return_if_not_present(c);
                }

                CompiledFunction *fn = c->cur_scope->function;
                fn->num_locals = c->cur_symbol_table->num_definitions;
                fn->num_parameters = fl->params.length;
                fn->literal = fl;

                SymbolTable *function_symbol_table = c->cur_symbol_table;
                Buffer free_symbols = function_symbol_table->free_symbols;
                leave_scope(c);

                // push free_symbols onto the stack, to go into [Closure].
                for (int i = 0; i < free_symbols.length; i++) {
                    load_symbol(c, free_symbols.data[i]);
                }

                symbol_table_free(function_symbol_table);

                Constant fn_const = {
                    .type = c_Function,
                    .data = { .function = fn }
                };

                int idx = add_constant(c, fn_const);
                if (idx == -1) { return c_error(n, add_constant_err); }

                emit(c, OpClosure, idx, free_symbols.length);
                return 0;
            }

        default:
            return 0;
    }
}

static error
perform_assignment(Compiler *c, Node node) {
    if (node.typ == n_Identifier) {
        Identifier *id = node.obj;
        Node id_node = { .obj = id };

        if (get_builtin(&id->tok) != -1) {
            return c_error(id_node, "builtin %.*s() is not assignable",
                           LITERAL(id->tok));
        }

        Symbol *symbol = sym_resolve(c->cur_symbol_table, hash(id));
        if (symbol == NULL) {
            return c_error(id_node,
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

            default:
                die("perform_assignment: unexpected symbol scope %d", symbol->scope);
        }

        return c_error(id_node, format, LITERAL(id->tok));

    } else if (node.typ == n_IndexExpression) {
        error err = _compile(c, node);
        if (err) { return err; }

        replace_last_opcode_with(c, OpIndex, OpSetIndex);
        return 0;
    }

    return c_error(node, "cannot assign to non-variable");
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
    int pos = make_valist_into(c->cur_instructions, op, operands);
    va_end(operands);
    set_last_instruction(c, op, pos);
    return pos;
}

void enter_scope(Compiler *c) {
    CompiledFunction *fn = calloc(1, sizeof(CompiledFunction));
    if (fn == NULL) { die("enter_scope - allocate Compiled Function:"); }

    CompilationScope new_scope = { .function = fn };
    int new_scope_idx = c->scopes.length;
    ScopeBufferPush(&c->scopes, new_scope);
    c->cur_scope = &c->scopes.data[new_scope_idx];
    c->cur_instructions = &fn->instructions;
    c->cur_mappings = &fn->mappings;

    c->cur_symbol_table = enclosed_symbol_table(c->cur_symbol_table);
}

void leave_scope(Compiler *c) {
    --c->scopes.length;
    c->cur_scope = &c->scopes.data[c->scopes.length - 1];

    CompiledFunction *function = c->cur_scope->function;
    c->cur_instructions = &function->instructions;
    c->cur_mappings = &function->mappings;

    c->cur_symbol_table = c->cur_symbol_table->outer;
}

error compile(Compiler *c, Program *prog) {
    // Create Top Level Compilation Scope if not exists.
    if (c->scopes.length == 0) {
        enter_scope(c);
    }

    error err;
    for (int i = 0; i < prog->stmts.length; i++) {
        err = _compile(c, prog->stmts.data[i]);
        if (err) { return err; }
    }

    // compiling module
    if (c->scopes.length > 1) {
        append_return_if_not_present(c);
    }
    return 0;
}

Bytecode bytecode(Compiler *c) {
    return (Bytecode) {
        .main_function = c->cur_scope->function,
        .num_globals = c->cur_symbol_table->num_definitions,
    };
}

static void
source_map(Compiler *c, Node node) {
    SourceMapping mapping = {
        .position = c->cur_instructions->length,
        .node = node
    };
    SourceMappingBufferPush(c->cur_mappings, mapping);
}

static error
c_error(Node n, char* format, ...) {
    va_list args;
    va_start(args, format);
    error err = verrorf(format, args);
    va_end(args);

    err->token = node_token(n);
    return err;
}

int add_constant(Compiler *c, Constant constant) {
    long position;
    uint64_t hash;

    switch (constant.type) {
        case c_String:
            hash = hash_string_fnv1a(constant.data.string->start,
                                     constant.data.string->length);
            break;
        case c_Integer:
        case c_Float:
            hash = constant.data.integer;
            break;
        case c_Function:
            position = c->constants.length;
            ConstantBufferPush(&c->constants, constant);
            return position;
        default:
            die("add_constant - constant type not handled %d", constant.type);
    }

    // initialize constants table if not done already
    if (c->constants_table.buckets == NULL) {
        compiler_init(c);
    }

    Object key = { .type = (ObjectType) constant.type };
    Object res = table_get_hash(&c->constants_table, key, hash);
    if (res.type == o_Nothing) {
        position = c->constants.length;
        ConstantBufferPush(&c->constants, constant);

        table_set_hash(&c->constants_table, key, OBJ(o_Integer, position), hash);
        if (errno == ENOMEM) { die("add_constant - table_set_hash:"); }
    } else {
        position = res.data.integer;
    }

    if (position <= INT_MAX) {
        return position;
    }
    return -1;
}
