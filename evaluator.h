#pragma once

#include "ast.h"
#include "environment.h"
#include "object.h"

Object eval_program(Program* p, Env* env);
