#ifndef __COMPILE_HELPER_H__
#define __COMPILE_HELPER_H__

#include "ast.h"
#include "debug_helpers.h"

ExpressionType getType(LLVMValueRef value);
bool isLiteralKind(TokenKind kind);

#endif
