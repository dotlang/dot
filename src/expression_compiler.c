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
            printf("int pushed: %s\n", node->token);
            stack[stack_ptr++] = LLVMConstInt(intType, atoi(node->token), true);
        }
        else
        {
            LLVMValueRef op1 = stack[--stack_ptr];
            LLVMValueRef op2 = stack[--stack_ptr];
            if ( node->kind == OP_ADD )
                stack[stack_ptr++] = LLVMBuildAdd(context->builder, op1, op2, "temp");
            else if ( node->kind == OP_SUB )
                stack[stack_ptr++] = LLVMBuildSub(context->builder, op1, op2, "temp");
        }

        node = node->next;
    }

    return stack[0];
}


