#include "basic_compilers.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <stdbool.h>

#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/TargetMachine.h"

#include "ast.h"
#include "debug_helpers.h"
#include "parsers.h"
#include "basic_compilers.h"

LLVMValueRef compileMathExpression(Context* context, 
        MathExpression* exp, LLVMBuilderRef builder);

LLVMValueRef compileMathFactor(Context* context, MathFactor* factor, 
                               LLVMBuilderRef builder)
{
    if ( factor->expression != NULL ) 
    {
        return compileMathExpression(context, factor->expression->math_expression, builder);
    }

    LLVMTypeRef intType = LLVMIntType(32);
    return LLVMConstInt(intType, factor->number, true);
}

LLVMValueRef compileMathExpression(Context* context, MathExpression* exp, LLVMBuilderRef builder)
{
    LLVMValueRef val1 = compileMathFactor(context, exp->factor, builder);

    char op = exp->op;

    if ( op == 0 ) 
    {
        return val1;
    }

    LLVMValueRef val2 = compileMathExpression(context, exp->expression, builder);

    if ( op == '+' )
    {
        return LLVMBuildAdd(builder, val1, val2, "temp");
    }
    else if ( op == '-' )
    {
        return LLVMBuildSub(builder, val1, val2, "temp");
    }
    else if ( op == '*' )
    {
        return LLVMBuildMul(builder, val1, val2, "temp");
    }
    else if ( op == '/' )
    {
        return LLVMBuildSDiv(builder, val1, val2, "temp");
    }

    abort();
}

void compileFunction(Context* context, FunctionDecl* function_decl)
{
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef mainfunc = LLVMAddFunction(context->module, "main", funcType);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
    LLVMPositionBuilderAtEnd(context->builder, entry);

    LLVMBuildRet(context->builder, 
            compileMathExpression(context, 
                function_decl->expression->math_expression, context->builder));
}

void compileModule(Context* context, Module* m)
{
    context->module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(context->module, "");
    LLVMSetTarget(context->module, LLVMGetDefaultTargetTriple());
    context->builder = LLVMCreateBuilder();

    compileFunction(context, m->items_head->static_binding->function_decl);

    char *error = NULL;
    LLVMVerifyModule(context->module, LLVMAbortProcessAction, &error);
    LLVMPrintModuleToFile(context->module, context->llvmir_file_path, &error);
    LLVMDisposeMessage(error);
}

void disposeLlvm(Context* context)
{
    LLVMDisposeBuilder(context->builder);
    LLVMDisposeModule(context->module);
}

