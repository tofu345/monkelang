#pragma once

#include "ast.h"
#include "environment.h"
#include "object.h"

// complete copy of [obj]
Object* object_copy(Env* env, Object* obj);

Object* new_error(Env* env, char* format, ...);
Object* eval_program(Program* p, Env* env);
