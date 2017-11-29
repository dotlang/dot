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
#include "compilers.h"

/* LLVMValueRef compileMathExpression(Context* context, */ 
/*         MathExpression* exp, LLVMBuilderRef builder); */

/* LLVMValueRef compileMathFactor(Context* context, MathFactor* factor, */ 
/*                                LLVMBuilderRef builder) */
/* { */
/*     if ( factor->expression != NULL ) */ 
/*     { */
/*         return compileMathExpression(context, factor->expression->math_expression, builder); */
/*     } */

/*     LLVMTypeRef intType = LLVMIntType(32); */
/*     return LLVMConstInt(intType, factor->number, true); */
/* } */

/* LLVMValueRef compileMathExpression(Context* context, MathExpression* exp, LLVMBuilderRef builder) */
/* { */
/*     LLVMValueRef val1 = compileMathFactor(context, exp->factor, builder); */

/*     char op = exp->op; */

/*     if ( op == OP_NOP ) */ 
/*     { */
/*         return val1; */
/*     } */

/*     LLVMValueRef val2 = compileMathExpression(context, exp->expression, builder); */

/*     if ( op == OP_ADD ) */
/*     { */
/*         return LLVMBuildAdd(builder, val1, val2, "temp"); */
/*     } */
/*     else if ( op == OP_SUB ) */
/*     { */
/*         return LLVMBuildSub(builder, val1, val2, "temp"); */
/*     } */
/*     else if ( op == OP_MUL ) */
/*     { */
/*         return LLVMBuildMul(builder, val1, val2, "temp"); */
/*     } */
/*     else if ( op == OP_DIV ) */
    /* { */
    /*     return LLVMBuildSDiv(builder, val1, val2, "temp"); */
    /* } */
    /* else if ( op == OP_REM ) */
    /* { */
    /*     return LLVMBuildSRem(builder, val1, val2, "temp"); */
    /* } */
    /* else if ( op == OP_DVT ) */
    /* { */
    /*     debugLog(context, "Compiling dvt expression"); */

    /*     LLVMTypeRef intType = LLVMIntType(32); */
    /*     LLVMValueRef one = LLVMConstInt(intType, 1, true); */
    /*     LLVMValueRef zero = LLVMConstInt(intType, 0, true); */

    /*     LLVMValueRef rem = LLVMBuildSRem(builder, val1, val2, "temp"); */
    /*     LLVMValueRef is_divisible =  LLVMBuildICmp(builder, LLVMIntEQ, rem, zero, "is_divisible"); */
    /*     return LLVMBuildSelect(builder, is_divisible, one, zero, "int_is_divisible"); */
    /* } */

    /* abort(); */
/* } */

/* void compileFunction(Context* context, FunctionDecl* function_decl) */
/* { */
    /* LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0); */
    /* LLVMValueRef mainfunc = LLVMAddFunction(context->module, "main", funcType); */
    /* LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry"); */
    /* LLVMPositionBuilderAtEnd(context->builder, entry); */

    /* LLVMBuildRet(context->builder, */ 
    /*         compileMathExpression(context, */ 
    /*             function_decl->expression->math_expression, context->builder)); */
/* } */

void compileBinding(Context* context, Binding* binding)
{
    LLVMTypeRef intType = LLVMIntType(32);
    int num = binding->function_decl->expression->first_element->eq_expression->
                first_element->cmp_expression->first_element->shift_expression->
                first_element->add_expression->first_element->mul_expression->
                first_element->unary_expression->primary_expression->
                basic_expression->number;
    LLVMValueRef nineteen = LLVMConstInt(intType, num, true);

    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef mainfunc = LLVMAddFunction(context->module, "main", funcType);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
    LLVMPositionBuilderAtEnd(context->builder, entry);

    LLVMBuildRet(context->builder, nineteen);
}

void compileModule(Context* context, Module* m)
{
    context->module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(context->module, "");
    LLVMSetTarget(context->module, LLVMGetDefaultTargetTriple());
    context->builder = LLVMCreateBuilder();

    compileBinding(context, m->first_element->binding);
    /* compileFunction(context, m->items_head->static_binding->function_decl); */

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

