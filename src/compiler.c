#include "compiler.h"
#include "ast.h"
#include "code.h"
#include "parser.h"
#include "object.h"
#include "utils.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

// emit append [Instruction] with [Opcode] and [int] operands
static int emit(Compiler *c, Opcode op, ...);
static int add_constant(Compiler *c, Object obj);
// add [ins] to [Compiler.instructions] and free [ins].
static int add_instruction(Compiler *c, uint8_t *ins, int length);

// TODO: eventually replace [evaluator.c/OBJ]
#define OBJ(t, d) (Object){ .typ = t, .is_marked = false, .data = { d } }

DEFINE_BUFFER(Constant, Object);

void compiler_init(Compiler *c) {
    *c = (Compiler){}; // equiv to memset(0)?
}

void compiler_destroy(Compiler *c) {
    for (int i = 0; i < c->errors.length; i++) {
        free(c->errors.data[i]);
    }
    free(c->errors.data);
    free(c->constants.data);
    free(c->instructions.data);
}

static void
compiler_error(Compiler* c, char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    if (vasprintf(&msg, format, args) == -1) ALLOC_FAIL();
    StringBufferPush(&c->errors, msg);
    va_end(args);
}

static int
_compile(Compiler *c, Node n) {
    int err;
    switch (n.typ) {
        case n_ExpressionStatement:
            {
                ExpressionStatement *es = n.obj;
                return _compile(c, es->expression);
            }

        case n_InfixExpression:
            {
                InfixExpression *ie = n.obj;
                err = _compile(c, ie->left);
                if (err == -1) {
                    return -1;
                }

                err = _compile(c, ie->right);
                if (err == -1) {
                    return -1;
                }
                return 0;
            }

        case n_IntegerLiteral:
            {
                IntegerLiteral *il = n.obj;
                Object integer = OBJ(o_Integer, .integer = il->value);
                emit(c, OpConstant, add_constant(c, integer));
                return 0;
            }

        default:
            die("_compile: node type %d not implemented", n.typ);
    }
    return -1;
}

static int
add_constant(Compiler *c, Object obj) {
    ConstantBufferPush(&c->constants, obj);
    return c->constants.length - 1;
}

static int
add_instruction(Compiler *c, uint8_t *ins, int length) {
    int pos_new_instruction = c->instructions.length;
    instructions_allocate(&c->instructions, length);
    memcpy(c->instructions.data + pos_new_instruction,
            ins, length * sizeof(uint8_t));
    free(ins);
    return pos_new_instruction;
}

static int
emit(Compiler *c, Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    Instructions ins = make_valist(op, ap);
    return add_instruction(c, ins.data, ins.length);
}

int compile(Compiler *c, Program *prog) {
    int err;
    for (int i = 0; i < prog->stmts.length; i++) {
        err = _compile(c, prog->stmts.data[i]);
        if (err == -1) { return -1; }
    }
    return 0;
}

Bytecode *bytecode(Compiler *c) {
    return (Bytecode *)c;
}
