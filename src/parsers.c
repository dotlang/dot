#include <string.h>
#include <ctype.h>

#include "parsers.h"
#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"
#include "stack.h"

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
//TODO: can we simplify this?
Expression* parseExpression(Context* context)
{
    //TODO: there are a lot of checks that can be done here to make sure exp has correct syntax
    ALLOC(expression, Expression);

    ExpressionNode* node = NULL;
    TokenKind kind = NA;
    TokenKind prev_kind = NA;
    char token[32];
    Stack* fn_stack = new_stack();
    Stack* op_stack = new_stack();

    while ( 1 )
    {
        //ignore newLine if it's the first thing we see
        if ( node != NULL && newLineAhead(context) )
        {
            if ( kind == IDENTIFIER ||
                 kind == INT_LITERAL ||
                 kind == BOOL_LITERAL ||
                 kind == CLOSE_PAREN )
            {
                break;
            }
        }

        getNextToken(context, token);
        if ( token[0] == 0 ) break;

        kind = getTokenKind(token, prev_kind);

        if ( kind == INT_LITERAL )
        {
            //if token is number or identifier, just move it to output
            ALLOC_NODE(temp_node, token, kind);

            if ( node == NULL ) { node = expression->first_node = temp_node; }
            else { node->next = temp_node; node = node->next; } 
        }
        else if ( kind == BOOL_LITERAL )
        {
            ALLOC_NODE(temp_node, token, kind);

            if ( node == NULL ) { node = expression->first_node = temp_node; }
            else { node->next = temp_node; node = node->next; } 
        }
        else if ( kind == IDENTIFIER )
        {
            ALLOC_NODE(temp_node, token, kind);

            if ( matchLiteral(context, OPEN_PAREN) )
            {
                //add it to stack
                temp_node->kind = FN_CALL;

                if ( matchLiteral(context, CLOSE_PAREN) )
                {
                    //this is a function call without arg
                    //do not add it to the stack but add to the output
                    if ( node == NULL ) { node = expression->first_node = temp_node; }
                    else { node->next = temp_node; node = node->next; } 
                }
                else
                {
                    push(op_stack, temp_node);
                    push(fn_stack, temp_node);
                }
            }
            else
            {
                //this is a binding name - add to output
                if ( node == NULL ) { node = expression->first_node = temp_node; }
                else { node->next = temp_node; node = node->next; } 
            }
        }
        else if ( kind == COMMA )
        {
            ((ExpressionNode*)peek(fn_stack))->arg_count++;
        }
        else if ( kind == OPEN_PAREN )
        {
            //use kind for all comparisons, not token
            //if token is "(" push it into stack
            ALLOC_NODE(lpar, token, kind);

            push(op_stack, lpar);
        }
        else if ( kind == CLOSE_PAREN )
        {
            //if token is ")" pop everything except "(" from stack and push to output, then pop "("
            ExpressionNode* op_stack_top = (ExpressionNode*)peek(op_stack);
            while ( op_stack_top != NULL )
            {
                if ( op_stack_top->kind == OPEN_PAREN || op_stack_top->kind == FN_CALL ) break;

                node->next = op_stack_top;
                node = node->next;
                pop(op_stack);
                op_stack_top = (ExpressionNode*)peek(op_stack);
            }

            //pop the open_paren or fn_call from op_stack
            pop(op_stack);

            if ( op_stack_top->kind == FN_CALL )
            {
                //we do not count the last argument, as we inc arg_count with each comma
                op_stack_top->arg_count++;
                node->next = op_stack_top;
                node = node->next;
            }
        }
        else //if we see a normal operator
        {
            int prec = getOperatorPrecedence(kind);

            //pop every operator in the op_stack which has lowe precedence or has same precedence and it left associative
            //until you see an opening parenthesis
            ExpressionNode* op_stack_top = (ExpressionNode*)peek(op_stack);
            while ( op_stack_top != NULL )
            {
                if ( op_stack_top->kind == OPEN_PAREN ) break;

                int stack_prec = getOperatorPrecedence(op_stack_top->kind);
                if ( stack_prec < prec ) break;
                if ( stack_prec == prec && !isLeftAssociative(op_stack_top->kind) ) break;

                pop(op_stack);
                node->next = op_stack_top;
                node = node->next;

                op_stack_top = (ExpressionNode*)peek(op_stack);
            }

            ALLOC_NODE(opr, token, kind);
            push(op_stack, opr);
        }

        prev_kind = kind;
    }

    //at the end, pop from stack and move to output queue
    ExpressionNode* op_stack_top = (ExpressionNode*)pop(op_stack);
    while ( op_stack_top != NULL )
    {
        node->next = op_stack_top;
        node = node->next;

        op_stack_top = (ExpressionNode*)pop(op_stack);
    }

    expression->last_node = node;

    dumpExpression(context, expression);

    return expression;
}

FunctionDecl* parseFunctionDecl(Context* context)
{
    ALLOC(function_decl, FunctionDecl);

    //TODO: replace this method with matchToken which accepts token type instead of string
    if ( !matchLiteral(context, OPEN_PAREN) ) return NULL;
    if ( !matchLiteral(context, CLOSE_PAREN) ) return NULL;
    if ( !matchLiteral(context, OP_ARROW) ) return NULL;

    if ( matchText(context, "int") )
    {
        if ( !matchLiteral(context, OPEN_BRACE ) ) return NULL;

        Binding* binding = NULL;

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
            if ( function_decl->first_binding == NULL )
            {
                function_decl->first_binding = temp_binding;
                binding = temp_binding;
            }
            else
            {
                binding->next = temp_binding;
                binding = binding->next;
            }
        }

        function_decl->last_binding = binding;
    }
    else
    {
        debugLog(context, "function's result is simple expression");
        //this is an expression in front of `funcName := () ->`
        ALLOC(binding, Binding);
        function_decl->last_binding = function_decl->first_binding = binding;
        function_decl->first_binding->expression = parseExpression(context);
        function_decl->first_binding->is_return = true;
    }

    return function_decl;
}

Binding* parseBinding(Context* context)
{
    ALLOC(binding, Binding);

    getNextToken(context, binding->lhs);
    if ( binding->lhs[0] == 0 ) return NULL;

    if ( getTokenKind(binding->lhs, NA) != IDENTIFIER )
    {
        errorLog("Invalid binding name: %s\n", binding->lhs);
    }

    debugLog(context, "Parsing binding: %s", binding->lhs);

    if ( matchLiteral(context, OP_COLON) )
    {
        getNextToken(context, binding->decl_type);
    }

    if ( !matchLiteral(context, OP_BIND) ) return NULL;

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
    Binding* binding;

    while ( 1 ) 
    {
        ALLOC(temp_binding, Binding);

        temp_binding = parseBinding(context);

        if ( temp_binding == NULL )
        {
            module->last_binding = binding;
            return module;
        }

        if ( module->first_binding == NULL )
        {
            module->first_binding = temp_binding;
            binding = temp_binding;
        }
        else
        {
            binding->next = temp_binding;
            binding = binding->next;
        }
    }

    module->last_binding = binding;
    return module;
}

