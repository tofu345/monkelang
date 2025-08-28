#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast.h"
#include "object.h"

Object eval_program(const Program* p);
Object eval(Node n);

#endif
