#include "expression_compiler.h"

LLVMValueRef compileExpression(Context* context, Expression* expression)
{
    //go through expression nodes and parse them as postfix notation
    LLVMTypeRef intType = LLVMIntType(32);
    LLVMValueRef stack[100];
    int stack_ptr = 0;
    
    ExpressionNode* node = expression->first_node;

    while ( node != NULL )
    {
        if ( node->kind == INT_LITERAL )
        {
            stack[stack_ptr++] = LLVMConstInt(intType, atoi(node->token), true);
        }
        else
        {
            LLVMValueRef op1 = stack[--stack_ptr];
            LLVMValueRef op2 = stack[--stack_ptr];

            if ( node->kind == OP_ADD )
                stack[stack_ptr++] = LLVMBuildAdd(context->builder, op1, op2, "temp");
            else if ( node->kind == OP_SUB )
                stack[stack_ptr++] = LLVMBuildSub(context->builder, op2, op1, "temp");
            else if ( node->kind == OP_MUL )
                stack[stack_ptr++] = LLVMBuildMul(context->builder, op1, op2, "temp");
            else if ( node->kind == OP_DIV )
                stack[stack_ptr++] = LLVMBuildSDiv(context->builder, op2, op1, "temp");
            else if ( node->kind == OP_REM )
                stack[stack_ptr++] = LLVMBuildSRem(context->builder, op2, op1, "temp");
            else if ( node->kind == OP_DVT )
            {
                LLVMTypeRef intType = LLVMIntType(32);
                LLVMValueRef one = LLVMConstInt(intType, 1, true);
                LLVMValueRef zero = LLVMConstInt(intType, 0, true);

                LLVMValueRef rem = LLVMBuildSRem(context->builder, op2, op1, "temp");
                LLVMValueRef is_divisible =  LLVMBuildICmp(context->builder, LLVMIntEQ, rem, zero, "is_divisible");
                stack[stack_ptr++] =  LLVMBuildSelect(context->builder, is_divisible, one, zero, "int_is_divisible");
            }
        }

        node = node->next;
    }

    return stack[0];
}


