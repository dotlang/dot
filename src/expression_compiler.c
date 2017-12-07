#include "expression_compiler.h"

//TODO: add shift operators
#define DO_POP(N) LLVMValueRef N = (LLVMValueRef)pop(stack)
#define DO_PUSH(N) push(stack, N)

LLVMValueRef compileExpression(Context* context, Expression* expression)
{
    //go through expression nodes and parse them as postfix notation
    LLVMTypeRef intType = LLVMIntType(64);
    LLVMValueRef one = LLVMConstInt(intType, 1, true);
    LLVMValueRef zero = LLVMConstInt(intType, 0, true);

    Stack* stack = new_stack();

    ExpressionNode* node = expression->first_node;
    while ( node != NULL )
    {
        if ( node->kind == OP_POS ) 
        {
            node = node->next;
            continue;
        }

        if ( node->kind == INT_LITERAL )
        {
            DO_PUSH(LLVMConstInt(intType, atoi(node->token), true));
        }
        else if ( node->kind == IDENTIFIER )
        {
			LLVMValueRef ptr = (LLVMValueRef)ht_get(context->function_bindings, node->token);
			//if this is a function level binding, load it's value from stack
			if ( ptr != NULL )
            {
                DO_PUSH(LLVMBuildLoad(context->builder, ptr, ""));
            }
            else
            {
                printf("Cannot find identifier: %s\n", node->token);
                abort();
            }
		}
		else
		{
            if ( node->kind == OP_ADD )
            {
                DO_POP(op1); DO_POP(op2);
                DO_PUSH(LLVMBuildAdd(context->builder, op1, op2, "temp"));
            }
            else if ( node->kind == OP_SUB )
            {
                DO_POP(op1); DO_POP(op2);
                DO_PUSH(LLVMBuildSub(context->builder, op2, op1, "temp"));
            }
            else if ( node->kind == OP_MUL )
            {
                DO_POP(op1); DO_POP(op2);
                DO_PUSH(LLVMBuildMul(context->builder, op1, op2, "temp"));
            }
            else if ( node->kind == OP_DIV )
            {
                DO_POP(op1); DO_POP(op2);
                DO_PUSH(LLVMBuildSDiv(context->builder, op2, op1, "temp"));
            }
            else if ( node->kind == OP_REM )
            {
                DO_POP(op1); DO_POP(op2);
                DO_PUSH(LLVMBuildSRem(context->builder, op2, op1, "temp"));
            }
            else if ( node->kind == OP_NEG )
            {
                DO_POP(op);
                DO_PUSH(LLVMBuildNeg (context->builder, op, "temp"));
            }
            else if ( node->kind == OP_SHR )
            {
                DO_POP(op1); DO_POP(op2);
                DO_PUSH(LLVMBuildAShr(context->builder, op2, op1, "temp"));
            }
            else if ( node->kind == OP_SHL )
            {
                DO_POP(op1); DO_POP(op2);
                DO_PUSH(LLVMBuildShl(context->builder, op2, op1, "temp"));
            }
            else if ( node->kind == FN_CALL )
            {
                //for now, functions do not have input
                LLVMValueRef args[] = {NULL};
                LLVMValueRef fn_ref = LLVMGetNamedFunction(context->module, node->token);
                //so the result here is a function we have to invoke
                DO_PUSH(LLVMBuildCall(context->builder, fn_ref, args, 0, ""));
            }
            else if ( node->kind == OP_DVT )
            {
                //this operator only applies to integers
                DO_POP(op1); DO_POP(op2);
                LLVMValueRef rem = LLVMBuildSRem(context->builder, op2, op1, "temp");
                LLVMValueRef is_divisible =  LLVMBuildICmp(context->builder, LLVMIntEQ, rem, zero, "is_divisible");
                DO_PUSH(LLVMBuildSelect(context->builder, is_divisible, one, zero, "int_is_divisible"));
            }
        }

        node = node->next;
    }

    return (LLVMValueRef)pop(stack);
}


