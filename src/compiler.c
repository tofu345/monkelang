#include "compiler.h"
#include "ast.h"
#include "code.h"
#include "symbol_table.h"
#include "utils.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

// emit append [Instruction] with [Opcode] and [int] operands
static int emit(Compiler *c, Opcode op, ...);
static int add_constant(Compiler *c, Constant obj_const);

DEFINE_BUFFER(Constant, Constant);

void compiler_init(Compiler *c) {
    memset(c, 0, sizeof(Compiler));
    symbol_table_init(&c->symbol_table);
}

void compiler_with(Compiler *c, SymbolTable *symbol_table,
        ConstantBuffer constants) {
    memset(c, 0, sizeof(Compiler));
    c->symbol_table = *symbol_table;
    c->constants = constants;
}

void compiler_free(Compiler *c) {
    for (int i = 0; i < c->errors.length; i++) {
        free(c->errors.data[i]);
    }
    free(c->errors.data);
    free(c->instructions.data);
    free(c->constants.data);
    symbol_table_free(&c->symbol_table);
}

static bool
last_instruction_is_pop(Compiler *c) {
    return c->last_instruction.opcode == OpPop;
}

static void
remove_last_pop(Compiler *c) {
    c->instructions.length = c->last_instruction.position;
    c->last_instruction = c->previous_instruction;
}

static void
replace_instruction(Compiler *c, int pos, Instructions new) {
    for (int i = 0; i < new.length; i++) {
        c->instructions.data[pos + i] = new.data[i];
    }
}

static void
change_operand(Compiler *c, int op_pos, int operand) {
    Opcode op = c->instructions.data[op_pos];
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
                Symbol *symbol = sym_define(&c->symbol_table, ls->name->tok.literal);
                emit(c, OpSetGlobal, symbol->index);
                return 0;
            }

        case n_Identifier:
            {
                Identifier *id = n.obj;
                Symbol *symbol = sym_resolve(&c->symbol_table, id->tok.literal);
                if (symbol == NULL) {
                    error(&c->errors, "undefined variable %s", id->tok.literal);
                    return -1;
                }
                emit(c, OpGetGlobal, symbol->index);
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
                if (last_instruction_is_pop(c)) {
                    remove_last_pop(c);
                }

                // Emit an `OpJump` with a bogus value
                int jump_pos = emit(c, OpJump, 9999);

                int after_consequence_pos = c->instructions.length;
                change_operand(c, jump_not_truthy_pos, after_consequence_pos);

                if (ie->alternative == NULL) {
                    emit(c, OpNull);
                } else {
                    if (_compile(c, NODE(n_BlockStatement, ie->alternative))) {
                        return -1;
                    }
                    if (last_instruction_is_pop(c)) {
                        remove_last_pop(c);
                    }
                }

                int after_alternative_pos = c->instructions.length;
                change_operand(c, jump_pos, after_alternative_pos);
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
                Constant str = {
                    .type = c_String,
                    .data = { .string = sl->tok.literal }
                };
                emit(c, OpConstant, add_constant(c, str));
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
    c->previous_instruction = c->last_instruction;
    c->last_instruction =
        (EmittedInstruction){ .opcode = op, .position = pos };
}

static int
emit(Compiler *c, Opcode op, ...) {
    va_list operands;
    va_start(operands, op);
    int pos = make_valist_into(&c->instructions, op, operands);
    va_end(operands);
    set_last_instruction(c, op, pos);
    return pos;
}

int compile(Compiler *c, Program *prog) {
    for (int i = 0; i < prog->stmts.length; i++) {
        if (_compile(c, prog->stmts.data[i])) {
            return -1;
        }
    }
    return 0;
}

Bytecode *bytecode(Compiler *c) {
    return (Bytecode *)c;
}
