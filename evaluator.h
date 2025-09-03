#pragma once

#include "ast.h"
#include "environment.h"
#include "object.h"

Object eval_program(Program* p, Env* env);
Object eval_program_from(Program* p, int idx, Env* env);
