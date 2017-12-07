#include "expression_compiler.h"

//TODO: add shift operators
#define DO_POP(N) LLVMValueRef N = (LLVMValueRef)pop(stack)
#define DO_PUSH(N) push(stack, N)

LLVMValueRef compileExpression(Context* context, Expression* expression)
{
    //go through expression nodes and parse them as postfix notation
    LLVMTypeRef intType = LLVMInt64Type();
    LLVMTypeRef int1Type = LLVMInt1Type();
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
        else if ( node->kind == FLOAT_LITERAL )
        {
            DO_PUSH(LLVMConstReal(LLVMDoubleType(), atof(node->token)));
        }
        else if ( node->kind == CHAR_LITERAL )
        {
            DO_PUSH(LLVMConstInt(LLVMInt8Type(), (int)node->token[1], true));
        }
        else if ( node->kind == BOOL_LITERAL )
        {
            if ( !strcmp(node->token, "true") )
            {
                DO_PUSH(LLVMConstInt(int1Type, 1, true));
            }
            else
            {
                DO_PUSH(LLVMConstInt(int1Type, 0, true));
            }
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
                errorLog("Cannot find identifier: %s\n", node->token);
            }
		}
		else
		{
            if ( node->kind == OP_ADD )
            {
                DO_POP(op1); DO_POP(op2);
                if ( getType(op1) == INT ) DO_PUSH(LLVMBuildAdd(context->builder, op1, op2, "temp"));
                else DO_PUSH(LLVMBuildFAdd(context->builder, op1, op2, "temp"));
            }
            else if ( node->kind == OP_SUB )
            {
                DO_POP(op1); DO_POP(op2);
                if ( getType(op1) == INT ) DO_PUSH(LLVMBuildSub(context->builder, op2, op1, "temp"));
                else DO_PUSH(LLVMBuildFSub(context->builder, op2, op1, "temp"));
            }
            else if ( node->kind == OP_MUL )
            {
                DO_POP(op1); DO_POP(op2);
                if ( getType(op1) == INT ) DO_PUSH(LLVMBuildMul(context->builder, op1, op2, "temp"));
                else DO_PUSH(LLVMBuildFMul(context->builder, op1, op2, "temp"));
            }
            else if ( node->kind == OP_DIV )
            {
                DO_POP(op1); DO_POP(op2);
                if ( getType(op1) == INT ) DO_PUSH(LLVMBuildSDiv(context->builder, op2, op1, "temp"));
                else DO_PUSH(LLVMBuildFDiv(context->builder, op2, op1, "temp"));

            }
            else if ( node->kind == OP_REM )
            {
                DO_POP(op1); DO_POP(op2);
                if ( getType(op1) == INT ) DO_PUSH(LLVMBuildSRem(context->builder, op2, op1, "temp"));
                else DO_PUSH(LLVMBuildFRem(context->builder, op2, op1, "temp"));
            }
            else if ( node->kind == OP_NEG )
            {
                DO_POP(op);
                if ( getType(op) == INT ) DO_PUSH(LLVMBuildNeg(context->builder, op, "temp"));
                else DO_PUSH(LLVMBuildFNeg(context->builder, op, "temp"));
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
                int arg_count = node->arg_count;
                //for now, functions do not have input
                ALLOC_ARRAY(args, arg_count, LLVMValueRef);
                
                for(int i=0;i<arg_count;i++)
                {
                    DO_POP(temp);
                    args[i] = temp;
                }

                char fn_name[128];
                strcpy(fn_name, node->token);

                if ( !strcmp(node->token, "int") )
                {
                    if ( getType(args[0]) == FLOAT )
                    {
                        strcpy(fn_name, "float_to_int");
                    }
                    else if ( getType(args[0]) == BOOL )
                    {
                        strcpy(fn_name, "bool_to_int");
                    }
                    else
                    {
                        strcpy(fn_name, "char_to_int");
                    }
                }

                LLVMValueRef fn_ref = LLVMGetNamedFunction(context->module, fn_name);
                if ( fn_ref == NULL )
                {
                    errorLog("Cannot find function: %s\n", node->token);
                }

                //so the result here is a function we have to invoke
                DO_PUSH(LLVMBuildCall(context->builder, fn_ref, args, arg_count, ""));
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


