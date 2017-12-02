#ifndef __EXP_COMPILER__
#define __EXP_COMPILER__

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <stdbool.h>

#include "debug_helpers.h"
#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/TargetMachine.h"
#include "parsers.h"
#include "ast.h"

LLVMValueRef compileExpression(Context* context, Expression* expression);

#endif
