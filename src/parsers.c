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
    ALLOC(expression, Expression);

    //TODO: there are a lot of checks that can be done here to make sure exp has correct syntax
    ExpressionNode* prev_node = NULL;

    //TODO: remove
    TokenKind kind = NA;
    //TODO: replace using prev node
    TokenKind prev_kind = NA;
    //TODO: remove
    char token[32];
    Stack* fn_stack = new_stack();
    Stack* op_stack = new_stack();

    while ( 1 )
    {
        bool add_node = true;

        //ignore newLine if it's the first thing we see
        if ( prev_node != NULL && newLineAhead(context) )
        {
            if ( kind == IDENTIFIER ||
                 kind == INT_LITERAL ||
                 kind == FLOAT_LITERAL ||
                 kind == CHAR_LITERAL ||
                 kind == BOOL_LITERAL ||
                 kind == CLOSE_PAREN )
            {
                break;
            }
        }

        getNextToken(context, token);
        if ( token[0] == 0 ) break;

        kind = getTokenKind(token, prev_kind);
        ALLOC_NODE(temp_node, token, kind);

        if ( kind == INT_LITERAL || kind == BOOL_LITERAL || kind == FLOAT_LITERAL || kind == CHAR_LITERAL )
        {
            //if token is literal or identifier, just move it to output
        }
        else if ( kind == IDENTIFIER )
        {
            if ( matchLiteral(context, OPEN_PAREN) )
            {
                //add it to stack
                temp_node->kind = FN_CALL;

                if ( matchLiteral(context, CLOSE_PAREN) )
                {
                    //this is a function call without arg
                    //do not add it to the stack but add to the output
                }
                else
                {
                    //the function call has at least one arg
                    temp_node->arg_count=1;
                    push(op_stack, temp_node);
                    push(fn_stack, temp_node);
                    add_node = false;
                }
            }
            else
            {
                //this is a binding name - add to output
            }
        }
        else if ( kind == COMMA )
        {
            ((ExpressionNode*)peek(fn_stack))->arg_count++;
            add_node=false;
        }
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

                prev_node->next = op_stack_top;
                //TODO: use chain_list
                prev_node = prev_node->next;
                pop(op_stack);
                op_stack_top = (ExpressionNode*)peek(op_stack);
            }

            //pop the open_paren or fn_call from op_stack
            pop(op_stack);

            if ( op_stack_top->kind == FN_CALL )
            {
                pop(fn_stack);

                //TODO: use chain_list
                prev_node->next = op_stack_top;
                prev_node = prev_node->next;
            }
            add_node=false;
        }
        else //if we see a normal operator
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
                //TODO: use chain list macro
                prev_node->next = op_stack_top;
                prev_node = prev_node->next;

                op_stack_top = (ExpressionNode*)peek(op_stack);
            }

            ALLOC_NODE(opr, token, kind);
            push(op_stack, opr);
            add_node=false;
        }

        if ( add_node ) 
        {
            if ( prev_node != NULL ) prev_node->next = temp_node;
            SET_IF_NULL(expression->first_node, temp_node);
            prev_node = temp_node;
        }

        prev_kind = kind;
    }

    //at the end, pop from stack and move to output queue
    ExpressionNode* op_stack_top = (ExpressionNode*)pop(op_stack);
    while ( op_stack_top != NULL )
    {
        //TODO: use chain list 
        prev_node->next = op_stack_top;
        prev_node = prev_node->next;

        op_stack_top = (ExpressionNode*)pop(op_stack);
    }

    dumpExpression(context, expression);

    return expression;
}

ExpressionType readTypeDecl(Context* context)
{
    SAVE_POSITION;

    char token[32];
    getNextToken(context, token);

    if ( !strcmp(token, "int") ) return INT;;
    if ( !strcmp(token, "float") ) return FLOAT;
    if ( !strcmp(token, "char") ) return CHAR;;
    if ( !strcmp(token, "bool") ) return BOOL;;

    RESTORE_POSITION;
    return NA_TYPE;
}

FunctionDecl* parseFunctionDecl(Context* context)
{
    ALLOC(function_decl, FunctionDecl);

    if ( !matchLiteral(context, OPEN_PAREN) ) return NULL;
    if ( !matchLiteral(context, CLOSE_PAREN) ) 
    {
        ArgDef* prev_arg_def = NULL;
        //function decl contains some inputs
        //in the form of name:type
        while ( 1 )
        {
            ALLOC(temp_arg_def, ArgDef);

            if ( prev_arg_def != NULL ) prev_arg_def->next = temp_arg_def;

            getNextToken(context, temp_arg_def->name);
            if ( !matchLiteral(context, OP_COLON) ) return NULL;
            getNextToken(context, temp_arg_def->type);

            SET_IF_NULL(function_decl->first_arg, temp_arg_def);
            function_decl->arg_count++;
            prev_arg_def = temp_arg_def;

            if ( matchLiteral(context, CLOSE_PAREN) ) break;
            if ( !matchLiteral(context, COMMA) ) return NULL;
        }
    }

    if ( !matchLiteral(context, OP_ARROW) ) return NULL;

    ExpressionType output_type;
    if ( (output_type = readTypeDecl(context)) != NA_TYPE )
    {
        function_decl->output_type = output_type;

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
    ALLOC(binding, Binding);
    long start_position = ftell(context->input_file);

    getNextToken(context, binding->lhs);
    if ( binding->lhs[0] == 0 ) return NULL;

    if ( getTokenKind(binding->lhs, NA) != IDENTIFIER )
    {
        errorLog("Invalid binding name: %s\n", binding->lhs);
    }

    debugLog(context, "Parsing binding: %s", binding->lhs);

    //for implicit bindings which result is ignored, we dont have normal pattern (e.g. assert(x) )
    bool is_explicit = false;

    if ( matchLiteral(context, OP_COLON) )
    {
        getNextToken(context, binding->decl_type);
        is_explicit = true;
    }

    if ( !matchLiteral(context, OP_BIND) )
    {
        //if it's an explicit binding, throw error
        if ( is_explicit ) errorLog("Invalid binding definition: %s", binding->lhs);
        //else, it's an explicit binding, restart from beginning and parse it
        fseek(context->input_file, start_position, SEEK_SET);
        strcpy(binding->lhs, "_");
    }

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
        if ( prev_binding != NULL ) prev_binding->next = temp_binding;

        SET_IF_NULL(module->first_binding, temp_binding);
        prev_binding = temp_binding;
    }

    return module;
}

