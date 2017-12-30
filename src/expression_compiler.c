#include "expression_compiler.h"

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
        //TODO: change this to switch
        if ( node->kind == OP_POS ) 
        {
            node = node->next;
            continue;
        }

        if ( node->kind == INT_LITERAL )
        {
            //TODO: add a function to convert expressionNode to literal (int, float, bool, ...)
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
			if ( ptr == NULL )
            {
                errorLog("Cannot find identifier: %s\n", node->token);
            }

            DO_PUSH(LLVMBuildLoad(context->builder, ptr, ""));
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
            else if ( node->kind == FN_CALL_SIMPLE )
            {
                LLVMValueRef fn_ref = LLVMGetNamedFunction(context->module, node->token);
                if ( fn_ref == NULL )
                {
                    errorLog("Cannot find function: %s\n", node->token);
                }

                //so the result here is a function we have to invoke
                DO_PUSH(LLVMBuildCall(context->builder, fn_ref, NULL, 0, ""));
            }
            else if ( node->kind == COMMA )
            {
                //add as place holder. This means: if you want me, just pop previous two elements from stack
                DO_PUSH(NULL);
            }
            else if ( node->kind == FN_CALL )
            {
                //1 2 , 3 , proc 
                //1 push
                //2 push
                //, pop both and push (1,2)
                //3 push
                //, pop last two and push (1,2,3)
                //proc: pop (1,2,3) and use it for call
                //so we can store LLVMValueRef or a linked list of value-refs in our stack
                //or we can keep them in stack and increment some counter?
                //
                //1 2 , 3 , proc 
                //1 push
                //2 push
                //, setup counter of 2
                //1 2 + proc
                //process(1,2,save(3,4),5)
                //1 2 , 3 4 , save , 5 , process
                //TODO: treat comma just like a normal value and push it into stack.
                //As we store LLVMValueRefs there, just put NULL as a place-holder
                //when you see a simple func call -> just render like normal
                //when you see fn_call: pop last item as the argument for function
                //if it is NULL (comma), pop two previous items and if any of them are 
                //NULL, pop their two previous and go on, until you have the data
                ALLOC(arg_list, FunctionArgList);
                struct FunctionArg* prev_item = NULL;

                int remaining_to_pop = 1;
                int arg_count = 0;
                while ( remaining_to_pop > 0 ) 
                {
                    void* data = pop(stack);
                    remaining_to_pop--;

                    if ( data == NULL ) remaining_to_pop += 2;
                    else
                    {
                        ALLOC(temp_item, struct FunctionArg);
                        arg_count++;

                        temp_item->arg = (LLVMValueRef)data;
                        CHAIN_LIST(arg_list->first_arg, prev_item, temp_item);
                    }
                }

                //now we have all the arguments inside arg_list linked list
                ALLOC_ARRAY(args, arg_count, LLVMValueRef);
                
                struct FunctionArg* current_arg = arg_list->first_arg;
                for(int i=arg_count-1;i>=0;i--)
                {
                    args[i] = current_arg->arg;
                    current_arg = current_arg->next;
                }

                char fn_name[128];
                strcpy(fn_name, node->token);

                //TODO: design a naming convention for functions, so we don't make checks for all types here
                //just create a function name (rename `int` to `int_f` for example)
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
            else if ( node->kind == OP_EQUALS )
            {
                DO_POP(op1); DO_POP(op2);
                if ( getType(op1) == FLOAT ) DO_PUSH(LLVMBuildFCmp(context->builder, LLVMRealOEQ, op1, op2, "temp"));
                else DO_PUSH(LLVMBuildICmp(context->builder, LLVMIntEQ, op1, op2, "temp"));
            }
        }

        node = node->next;
    }

    return (LLVMValueRef)pop(stack);
}


