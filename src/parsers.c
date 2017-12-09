#include <string.h>
#include <ctype.h>

#include "parsers.h"
#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"
#include "stack.h"
#include "compile_helper.h"

Expression* parseExpression(Context*);
Binding* parseBinding(Context*);

// If you see ( after identifier, push `identifier(` to the stack, this is a fn call
// with each comma, increase counter for the latest function call in stack
//when you see `)` pop everything until you see `(`. If it is with fn, then pop that and add to output.
//do not push comma to any place
//So
//f(g(a,b),c,d) will become:
//a b g2 c d f3
//maybe we should keep track of fn-calls in the op-stack separately to make increasing op-count for them more efficient
Expression* parseExpression(Context* context)
{
    ALLOC(expression, Expression);

    //TODO: there are a lot of checks that can be done here to make sure exp has correct syntax
    ExpressionNode* prev_node = NULL;

    TokenKind kind = NA;
    TokenKind prev_kind = NA;
    char token[32];
    //used to keep track of the current function call, to update it's argument count
    //process(1,2,3)
    //TODO: can we eliminate fn_stack and add a marker instead?
    //when we see process( we add a marker to mark start of an arg-list
    //when we see ) we insert function name to call
    /* Stack* fn_stack = new_stack(); */
    Stack* op_stack = new_stack();

    //f(1,2,3,4)
    //1 2 , 3 , 4 , f
    //we can think of comma as an operator
    while ( 1 )
    {
        bool add_node = true;

        //ignore newLine if it's the first thing we see
        if ( prev_node != NULL && newLineAhead(context) )
        {
            if ( kind == IDENTIFIER || isLiteralKind(kind) || kind == CLOSE_PAREN )
            {
                break;
            }
        }

        getNextToken(context, token);
        if ( token[0] == 0 ) break;

        kind = getTokenKind(token, prev_kind);
        ALLOC_NODE(temp_node, token, kind);

        if ( kind == IDENTIFIER )
        {
            if ( matchLiteral(context, OPEN_PAREN) )
            {
                //assume function has no parameter to send
                temp_node->kind = FN_CALL_SIMPLE;

                if ( !matchLiteral(context, CLOSE_PAREN) )
                {
                    temp_node->kind = FN_CALL;
                    //the function call has at least one arg
                    /* temp_node->arg_count=1; */
                    push(op_stack, temp_node);
                    /* push(fn_stack, temp_node); */
                    add_node = false;
                }
            }
        }
        /* else if ( kind == COMMA ) */
        /* { */
        /*     /1* ((ExpressionNode*)peek(fn_stack))->arg_count++; *1/ */
        /*     /1* add_node=false; *1/ */
        /* } */
        else if ( kind == OPEN_PAREN )
        {
            //use kind for all comparisons, not token
            //if token is "(" push it into stack
            ALLOC_NODE(lpar, token, kind);

            push(op_stack, lpar);
            add_node=false;
        }
        else if ( kind == CLOSE_PAREN )
        {
            //if token is ")" pop everything except "(" from stack and push to output, then pop "("
            ExpressionNode* op_stack_top = (ExpressionNode*)peek(op_stack);
            while ( op_stack_top != NULL )
            {
                if ( op_stack_top->kind == OPEN_PAREN || op_stack_top->kind == FN_CALL ) break;

                CHAIN_LIST(expression->first_node, prev_node, op_stack_top);
                pop(op_stack);
                op_stack_top = (ExpressionNode*)peek(op_stack);
            }

            //pop the open_paren or fn_call from op_stack
            pop(op_stack);

            if ( op_stack_top->kind == FN_CALL )
            {
                /* pop(fn_stack); */
                CHAIN_LIST(expression->first_node, prev_node, op_stack_top);
            }
            add_node=false;
        }
        else if ( !isLiteralKind(kind) ) //if we see a normal operator (including comma)
        {
            int prec = getOperatorPrecedence(kind);

            //pop every operator in the op_stack which has lowe precedence or has same precedence and it left associative
            //until you see an opening parenthesis (or fn_call which implied opening parenthesis)
            ExpressionNode* op_stack_top = (ExpressionNode*)peek(op_stack);
            while ( op_stack_top != NULL )
            {
                if ( op_stack_top->kind == OPEN_PAREN ) break;
                if ( op_stack_top->kind == FN_CALL ) break;

                int stack_prec = getOperatorPrecedence(op_stack_top->kind);
                if ( stack_prec < prec ) break;
                if ( stack_prec == prec && !isLeftAssociative(op_stack_top->kind) ) break;

                pop(op_stack);
                CHAIN_LIST(expression->first_node, prev_node, op_stack_top);
                op_stack_top = (ExpressionNode*)peek(op_stack);
            }

            ALLOC_NODE(opr, token, kind);
            push(op_stack, opr);
            add_node=false;
        }

        if ( add_node ) 
        {
            CHAIN_LIST(expression->first_node, prev_node, temp_node);
        }

        prev_kind = kind;
    }

    //at the end, pop from stack and move to output queue
    ExpressionNode* op_stack_top = (ExpressionNode*)pop(op_stack);
    while ( op_stack_top != NULL )
    {
        CHAIN_LIST(expression->first_node, prev_node, op_stack_top);
        op_stack_top = (ExpressionNode*)pop(op_stack);
    }

    dumpExpression(context, expression);

    return expression;
}

bool readTypeDecl(Context* context, char* token)
{
    SAVE_POSITION;

    getNextToken(context, token);

    if ( !strcmp(token, "int") ) return true;
    if ( !strcmp(token, "float") ) return  true;
    if ( !strcmp(token, "char") ) return true;
    if ( !strcmp(token, "bool") ) return true;

    RESTORE_POSITION;
    return false;
}

FunctionDecl* parseFunctionDecl(Context* context)
{
    ALLOC(function_decl, FunctionDecl);

    if ( !matchLiteral(context, OPEN_PAREN) ) return NULL;
    //does the function have any inputs? (name1: type1, name2: type2, ...)
    if ( !matchLiteral(context, CLOSE_PAREN) ) 
    {
        ArgDef* prev_arg_def = NULL;

        while ( 1 )
        {
            ALLOC(temp_arg_def, ArgDef);

            getNextToken(context, temp_arg_def->name);
            if ( !matchLiteral(context, OP_COLON) ) return NULL;
            getNextToken(context, temp_arg_def->type);

            CHAIN_LIST(function_decl->first_arg, prev_arg_def, temp_arg_def);

            if ( matchLiteral(context, CLOSE_PAREN) ) break;
            if ( !matchLiteral(context, COMMA) ) return NULL;
        }
    }

    if ( !matchLiteral(context, OP_ARROW) ) return NULL;

    char output_type[32];
    if ( readTypeDecl(context, output_type) )
    {
        strcpy(function_decl->output_type, output_type);

        if ( !matchLiteral(context, OPEN_BRACE ) ) return NULL;

        Binding* prev_binding = NULL;

        while ( 1 ) 
        {
            if ( matchLiteral(context, CLOSE_BRACE) ) break;

            Binding* temp_binding = NULL;
            if ( matchLiteral(context, OP_RETURN) )
            {
                temp_binding = (Binding*)calloc(1, sizeof(Binding));
                temp_binding->is_return = true;
                temp_binding->expression = parseExpression(context);
            }
            else
            {
                temp_binding = parseBinding(context);
            }

            if ( temp_binding == NULL ) break;
            CHAIN_LIST(function_decl->first_binding, prev_binding, temp_binding);
        }
    }
    else
    {
        debugLog(context, "function's result is simple expression");
        //this is an expression in front of `funcName := () ->`
        ALLOC(temp_binding, Binding);
        temp_binding->expression = parseExpression(context);
        temp_binding->is_return = true;
        function_decl->first_binding = temp_binding;
    }

    return function_decl;
}

Binding* parseBinding(Context* context)
{
    //IDENTIFIER : TYPE := EXPRESSION
    //IDENTIFIER := EXPRESSION
    //EXPRESSION (output should be discarded)
    ALLOC(binding, Binding);
    long start_position = ftell(context->input_file);

    getNextToken(context, binding->lhs);
    if ( binding->lhs[0] == 0 ) return NULL;

    if ( getTokenKind(binding->lhs, NA) == IDENTIFIER )
    {
        //read the type if it is specified
        if ( matchLiteral(context, OP_COLON) )
        {
            getNextToken(context, binding->decl_type);
        }

        if ( !matchLiteral(context, OP_BIND) )
        {
            //we were wrong from the beginning! this is the third case (only an expression)
            //seek back
            fseek(context->input_file, start_position, SEEK_SET);
            strcpy(binding->lhs, "_");
            binding->decl_type[0] = 0;
        }
    }
    
    //now parse what comes after `:=` (or the given text if binding does not have lhs)
    //there are two cases : function declaration or an expression
    SAVE_POSITION;
    binding->function_decl = parseFunctionDecl(context);
    if ( binding->function_decl == NULL )
    {
        RESTORE_POSITION;
        debugLog(context, "%s is an expression.", binding->lhs);
        binding->expression = parseExpression(context);
    }
    else debugLog(context, "%s is a function binding", binding->lhs);

    return binding;
}

Module* parseModule(Context* context)
{
    ALLOC(module, Module);
    Binding* prev_binding = NULL;

    while ( 1 ) 
    {
        ALLOC(temp_binding, Binding);

        temp_binding = parseBinding(context);

        if ( temp_binding == NULL )
        {
            return module;
        }

        CHAIN_LIST(module->first_binding, prev_binding, temp_binding);
    }

    return module;
}

