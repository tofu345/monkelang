#pragma once

#include "ast.h"
#include "environment.h"
#include "object.h"

Object eval_program(const Program* p, Env* env);
