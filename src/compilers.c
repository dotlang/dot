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

LLVMValueRef compileExpression(Context* context, Expression* expression);
void compileBinding(Context* context, Binding* binding);

LLVMValueRef compileUnaryExpression(Context* context, UnaryExpression* unary_expression)
{
    LLVMTypeRef intType = LLVMIntType(32);
    /* printf("basic is %d", unary_expression->primary_expression->basic_expression->number); */

    BasicExpression* basic_expression = unary_expression->primary_expression->basic_expression;

    if ( basic_expression->expression == NULL && basic_expression->binding_name[0] == 0 )
    {
        return LLVMConstInt(intType, basic_expression->number, true);
    }
    else if ( basic_expression->binding_name[0] != 0 )
    {
        LLVMValueRef ptr = (LLVMValueRef)ht_get(context->bindings, basic_expression->binding_name);
        return LLVMBuildLoad(context->builder, ptr, "");
    }
    else
    {
        return compileExpression(context, basic_expression->expression);
    }
}

LLVMValueRef compileMulExpression(Context* context, MulExpression* mul_expression)
{
    struct MulExpressionElement* current_element = mul_expression->first_element;
    LLVMValueRef current_value = compileUnaryExpression(context, current_element->unary_expression);

    current_element = current_element->next;

    while ( current_element != NULL )
    {
        LLVMValueRef next_value = compileUnaryExpression(context, current_element->unary_expression);

        switch ( current_element->op )
        {
            case OP_MUL:
                {
                    current_value = LLVMBuildMul(context->builder, current_value, next_value, "temp");
                    break;
                }
            case OP_DIV:
                {
                    current_value = LLVMBuildSDiv(context->builder, current_value, next_value, "temp");
                    break;
                }
            case OP_REM:
                {
                    current_value = LLVMBuildSRem(context->builder, current_value, next_value, "temp");
                    break;
                }
            case OP_DVT:
                {
                    LLVMTypeRef intType = LLVMIntType(32);
                    LLVMValueRef one = LLVMConstInt(intType, 1, true);
                    LLVMValueRef zero = LLVMConstInt(intType, 0, true);

                    LLVMValueRef rem = LLVMBuildSRem(context->builder, current_value, next_value, "temp");
                    LLVMValueRef is_divisible =  LLVMBuildICmp(context->builder, LLVMIntEQ, rem, zero, "is_divisible");
                    current_value =  LLVMBuildSelect(context->builder, is_divisible, one, zero, "int_is_divisible");
                }
        }

        current_element = current_element->next;
    }


    return current_value;
}

LLVMValueRef compileAddExpression(Context* context, AddExpression* add_expression)
{
    //here we have a list of one or more AddExpressionElements
    struct AddExpressionElement* current_element = add_expression->first_element;
    //op for the first element is not important/used
    LLVMValueRef current_value = compileMulExpression(context, current_element->mul_expression);
    
    current_element = current_element->next;

    while ( current_element != NULL )
    {
        LLVMValueRef next_value = compileMulExpression(context, current_element->mul_expression);

        if ( current_element->op == OP_ADD )
        {
            current_value = LLVMBuildAdd(context->builder, current_value, next_value, "temp");
        }
        else if ( current_element->op == OP_SUB )
        {
            current_value = LLVMBuildSub(context->builder, current_value, next_value, "temp");
        }

        current_element = current_element->next;
    }

    return current_value;
}

LLVMValueRef compileShiftExpression(Context* context, ShiftExpression* shift_expression)
{
    return compileAddExpression(context, shift_expression->first_element->add_expression);
}

LLVMValueRef compileCmpExpression(Context* context, CmpExpression* cmp_expression)
{
    return compileShiftExpression(context, cmp_expression->first_element->shift_expression);
}

LLVMValueRef compileEqExpression(Context* context, EqExpression* eq_expression)
{
    return compileCmpExpression(context, eq_expression->first_element->cmp_expression);
}

LLVMValueRef compileExpression(Context* context, Expression* expression)
{
    return compileEqExpression(context, expression->first_element->eq_expression);
}

void compileCodeBlock(Context* context, CodeBlock* code_block)
{
    struct CodeBlockElement* element = code_block->first_element;

    while ( element != NULL )
    {
        if ( element->return_expression != NULL )
        {
            LLVMValueRef return_exp = compileExpression(context, code_block->last_element->return_expression);
            LLVMBuildRet(context->builder, return_exp);
        }
        else
        {
            compileBinding(context, element->binding);
        }

        element = element->next;
    }
}

void compileFunctionDecl(Context* context, FunctionDecl* function_decl)
{
    if ( function_decl->expression != NULL )
    {
        LLVMValueRef return_exp = compileExpression(context, function_decl->expression);
        LLVMBuildRet(context->builder, return_exp);
    }
    else
    {
        compileCodeBlock(context, function_decl->code_block);
    }
}

void compileBinding(Context* context, Binding* binding)
{
    if ( binding->function_decl != NULL )
    {
        LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
        LLVMValueRef mainfunc = LLVMAddFunction(context->module, binding->lhs, funcType);
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
        LLVMPositionBuilderAtEnd(context->builder, entry);

        compileFunctionDecl(context, binding->function_decl);
    }
    else
    {
        LLVMValueRef r_value = compileExpression(context, binding->expression);
        LLVMTypeRef int_type = LLVMIntType(32);
        LLVMValueRef alloc_ref = LLVMBuildAlloca(context->builder, int_type, binding->lhs);
        LLVMBuildStore(context->builder, r_value, alloc_ref);

        ht_set(context->bindings, binding->lhs, alloc_ref);
    }
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

