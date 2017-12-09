#ifndef __COMPILE_HELPER_H__
#define __COMPILE_HELPER_H__

#include "ast.h"
#include "debug_helpers.h"

ExpressionType getType(LLVMValueRef value);
bool isLiteralKind(TokenKind kind);
LLVMTypeRef expressionTypeToLLVMType(char* type);
LLVMTypeRef getFunctionType(Context* context, FunctionDecl* function_decl);
LLVMTypeRef getBindingType(Binding* binding);

#endif
