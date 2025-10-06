#include "compiler.h"
#include "ast.h"
#include "code.h"
#include "object.h"
#include "utils.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

// emit append [Instruction] with [Opcode] and [int] operands
static int emit(Compiler *c, Opcode op, ...);
static int add_constant(Compiler *c, Object obj);
static int add_instruction(Compiler *c, Instructions ins);

DEFINE_BUFFER(Constant, Object);

void compiler_init(Compiler *c) {
    memset(c, 0, sizeof(Compiler));
}

void compiler_destroy(Compiler *c) {
    for (int i = 0; i < c->errors.length; i++) {
        free(c->errors.data[i]);
    }
    free(c->errors.data);
    free(c->constants.data);
    free(c->instructions.data);
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

        case n_IntegerLiteral:
            {
                IntegerLiteral *il = n.obj;
                Object integer = OBJ(o_Integer, .integer = il->value);
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

        default:
            return 0;
    }
}

static int
add_constant(Compiler *c, Object obj) {
    ConstantBufferPush(&c->constants, obj);
    return c->constants.length - 1;
}

static int
add_instruction(Compiler *c, Instructions ins) {
    int pos_new_instruction = c->instructions.length;
    instructions_allocate(&c->instructions, ins.length);
    memcpy(c->instructions.data + pos_new_instruction,
            ins.data, ins.length * sizeof(uint8_t));
    return pos_new_instruction;
}

static void
set_last_instruction(Compiler *c, Opcode op, int pos) {
    c->previous_instruction = c->last_instruction;
    c->last_instruction =
        (EmittedInstruction){ .opcode = op, .position = pos };
}

static int
emit(Compiler *c, Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    Instructions ins = make_valist(op, ap);
    int pos = add_instruction(c, ins);
    set_last_instruction(c, op, pos);
    free(ins.data);
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
