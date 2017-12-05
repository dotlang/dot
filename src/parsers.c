#include <string.h>
#include <ctype.h>

#include "parsers.h"
#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"

Expression* parseExpression(Context*);
Binding* parseBinding(Context*);

/* BasicExpression* parseBasicExpression(Context* context) */
/* { */
/*     ALLOC(basic_expression, BasicExpression); */

/*     char num[16]; */
/*     if ( matchNumber(context, num) ) */
/*     { */
/*         basic_expression->number = atoi(num); */
/*         return basic_expression; */
/*     } */

/*     IF_MATCH("(") */
/*     { */
/*         PARSE(basic_expression->expression, parseExpression); */
/*         IF_MATCH(")") return basic_expression; */
/*     } */
/*     //it might be an identifier */
/*     char token[256]; */
/*     int result = parseIdentifier(context, token); */
/*     if ( result == OK ) */ 
/*     { */
/*         strcpy(basic_expression->binding_name, token); */
/*         return basic_expression; */
/*     } */

/*     return NULL; */
/* } */

/* TermExpression* parseTermExpression(Context* context) */
/* { */
/*     IF_MATCH("(") */
/*     { */
/*         IF_MATCH(")") */
/*         { */
/*             ALLOC(term_expression, TermExpression); */
/*             term_expression->op = OP_CAL; */
/*             return term_expression; */
/*         } */
/*     } */

/*     return NULL; */
/* } */

/* PrimaryExpression* parsePrimaryExpression(Context* context) */
/* { */
/*     ALLOC(primary_expression, PrimaryExpression); */
/*     PARSE(primary_expression->basic_expression, parseBasicExpression); */

/*     struct PrimaryExpressionElement* element; */

/*     while ( 1 ) */
/*     { */
/*         ALLOC(element_next, struct PrimaryExpressionElement); */

/*         PARSE_ELSE(element_next->term_expression, parseTermExpression) */
/*         { */
/*             primary_expression->last_element = element; */
/*             return primary_expression; */
/*         } */

/*         if ( primary_expression->first_element == NULL ) */
/*         { */
/*             primary_expression->first_element  = element_next; */
/*             element = element_next; */
/*         } */
/*         else */
/*         { */
/*             element->next = element_next; */
/*             element = element->next; */
/*         } */
/*     } */

/*     return primary_expression; */
/* } */

/* UnaryExpression* parseUnaryExpression(Context* context) */
/* { */
/*     ALLOC(unary_expression, UnaryExpression); */
/*     PARSE(unary_expression->primary_expression, parsePrimaryExpression); */

/*     return unary_expression; */
/* } */

/* MulExpression* parseMulExpression(Context* context) */
/* { */
/*     ALLOC(mul_expression, MulExpression); */
/*     ALLOC(element, struct MulExpressionElement); */

/*     mul_expression->first_element = mul_expression->last_element = element; */
/*     PARSE(element->unary_expression, parseUnaryExpression); */

/*     element->op = OP_NOP; */

/*     const char* ops[] = { "*", "/", "%%", "%" }; */
/*     const char* op = matchLiterals(context, ops, 4); */

/*     while ( op != NULL ) */
/*     { */
/*         ALLOC(element_next, struct MulExpressionElement); */
/*         element->next = element_next; */
/*         element = element->next; */

/*         element->op = strToOp(op); */

/*         PARSE(element->unary_expression, parseUnaryExpression); */

/*         op = matchLiterals(context, ops, 4); */
/*     } */

/*     mul_expression->last_element = element; */

/*     //TODO: scan for operators */
/*     return mul_expression; */
/* } */

/* AddExpression* parseAddExpression(Context* context) */
/* { */
/*     ALLOC(add_expression, AddExpression); */
/*     ALLOC(element, struct AddExpressionElement); */

/*     add_expression->first_element = add_expression->last_element = element; */
/*     PARSE(element->mul_expression, parseMulExpression); */

/*     //for the first element there is no op */
/*     element->op = OP_NOP; */

/*     const char* ops[] = { "+", "-" }; */
/*     const char* op = matchLiterals(context, ops, 2); */

/*     //it's fine if we no longer see operators */
/*     while ( op != NULL ) */ 
/*     { */
/*         ALLOC(element_next, struct AddExpressionElement); */
/*         element->next = element_next; */
/*         element = element->next; */

/*         element->op = strToOp(op); */

/*         PARSE(element->mul_expression, parseMulExpression); */

/*         op = matchLiterals(context, ops, 2); */
/*     } */
/*     add_expression->last_element = element; */

/*     return add_expression; */
/* } */

/* ShiftExpression* parseShiftExpression(Context* context) */
/* { */
/*     ALLOC(shift_expression, ShiftExpression); */
/*     ALLOC(element, struct ShiftExpressionElement); */

/*     shift_expression->first_element = shift_expression->last_element = element; */

/*     element->op = OP_NOP; */
/*     PARSE(element->add_expression, parseAddExpression); */
/*     //TODO: scan for operators */
/*     // */
/*     return shift_expression; */
/* } */

/* CmpExpression* parseCmpExpression(Context* context) */
/* { */
/*     ALLOC(cmp_expression, CmpExpression); */
/*     ALLOC(element, struct CmpExpressionElement); */

/*     cmp_expression->first_element = cmp_expression->last_element = element; */

/*     element->op = OP_NOP; */
/*     PARSE(element->shift_expression, parseShiftExpression); */
/*     //TODO: scan for operators */
/*     return cmp_expression; */
/* } */

/* EqExpression* parseEqExpression(Context* context) */
/* { */
/*     ALLOC(eq_expression, EqExpression); */
/*     ALLOC(element, struct EqExpressionElement); */

/*     eq_expression->first_element = eq_expression->last_element = element; */

/*     element->op = OP_NOP; */
/*     PARSE(element->cmp_expression, parseCmpExpression); */
/*     //TODO: scan for operators */
    
/*     return eq_expression; */
/* } */

TokenKind getTokenKind(char* token)
{
    if ( !strcmp(token, "(") ) return LEFT_PAREN;
    if ( !strcmp(token, ")") ) return RIGHT_PAREN;
    if ( !strcmp(token, "+") ) return OP_ADD;
    if ( !strcmp(token, "-") ) return OP_SUB;
    if ( !strcmp(token, "*") ) return OP_MUL;
    if ( !strcmp(token, "/") ) return OP_DIV;
    if ( !strcmp(token, "%") ) return OP_REM;
    if ( !strcmp(token, "%%") ) return OP_DVT;
    if ( !strcmp(token, "{") ) return LEFT_BRACE;
    if ( !strcmp(token, "}") ) return RIGHT_BRACE;

    if ( isdigit(token[0]) ) return INT_LITERAL;

    return IDENTIFIER;
}

int getOperatorPrecedence(TokenKind kind)
{
    switch ( kind )
    {
        case OP_ADD: return 2;
        case OP_SUB: return 2;
        case OP_MUL: return 3;
        case OP_DIV: return 3;
        case OP_REM: return 3;
        case OP_DVT: return 3;
        default: abort();
    }

    abort();
}
            
bool isLeftAssociative(TokenKind kind)
{
    return true;
}

Expression* parseExpression(Context* context)
{
    //TODO: there are a lot of checks that can be done here to make sure exp has correct syntax
    //TODO: remove lpar and rpar from token kinds
    ALLOC(expression, Expression);

    ExpressionNode* node = NULL;
    ExpressionNode* stack[100];
    TokenKind kind = NA;
    char token[32];
    int stack_ptr = 0;

    while ( 1 )
    {
        //ignore newLine if it's the first thing we see
        if ( node != NULL && newLineAhead(context) )
        {
            if ( kind == IDENTIFIER ||
                 kind == INT_LITERAL ||
                 kind == RIGHT_PAREN )
            {
                break;
            }
        }

        int token_len = getNextToken(context, token);
        if ( token_len == 0 ) break;

        kind = getTokenKind(token);

        if ( kind == INT_LITERAL || kind == IDENTIFIER )
        {
            //if token is number or identifier, just move it to output
            ALLOC(temp_node, ExpressionNode);
            strcpy(temp_node->token, token);
            temp_node->kind = kind;

            if ( node == NULL ) { node = expression->first_node = temp_node; }
            else { node->next = temp_node; node = node->next; } 
        }
        else if ( !strcmp(token, "(") )
        {
            //if token is "(" push it into stack
            ALLOC(lpar, ExpressionNode);
            strcpy(lpar->token, "(");
            lpar->kind = LEFT_PAREN;

            stack[stack_ptr] = lpar;
            stack_ptr++;
        }
        else if ( !strcmp(token, ")") )
        {
            //if token is ")" pop everything except "(" from stack and push to output, then pop "("
            while ( stack_ptr > 0 && stack[stack_ptr-1]->kind != LEFT_PAREN )
            {
                stack_ptr--;
                node->next = stack[stack_ptr];
                node = node->next;
            }

            stack_ptr--;
        }
        else
        {
            int prec = getOperatorPrecedence(kind);

            //if token is opreator first check precedence then push to stack
            while ( stack_ptr > 0 &&
                    stack[stack_ptr-1]->kind != LEFT_PAREN &&
                    ( ( stack[stack_ptr-1]->prec > prec ) ||
                      ( stack[stack_ptr-1]->prec == prec && isLeftAssociative(stack[stack_ptr-1]->kind) )
                    ) )
            {
                stack_ptr--;
                node->next = stack[stack_ptr];
                node = node->next;
            }

            ALLOC(opr, ExpressionNode);
            strcpy(opr->token, token);
            opr->kind = kind;
            opr->prec = prec;

            stack[stack_ptr++] = opr;
        }
    }

    //at the end, pop from stack and move to output queue
    while ( stack_ptr > 0 ) 
    {
        stack_ptr--;
        node->next = stack[stack_ptr];
        node = node->next;
    }

    expression->last_node = node;

    dumpExpression(context, expression);

    return expression;
}

FunctionDecl* parseFunctionDecl(Context* context)
{
    ALLOC(function_decl, FunctionDecl);

    char token[256];
    getNextToken(context, token);
    if ( strcmp(token, "(") ) return NULL;
    getNextToken(context, token);
    if ( strcmp(token, ")") ) return NULL;
    getNextToken(context, token);
    if ( strcmp(token, "->") ) return NULL;

    peekNextToken(context, token);
    if ( !strcmp(token, "int") )
    {
        getNextToken(context, token);
        getNextToken(context, token);
        if ( strcmp(token, "{") ) return NULL;

        Binding* binding = NULL;

        while ( 1 ) 
        {
            peekNextToken(context, token);
            if ( !strcmp(token, "}") ) 
            {
                getNextToken(context, token);
                break;
            }

            Binding* temp_binding = NULL;
            if ( !strcmp(token, "::") )
            {
                getNextToken(context, token);
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

    char token[256];
    int len = getNextToken(context, token);
    if ( len == 0 ) return NULL;
    
    strcpy(binding->lhs, token);
    debugLog(context, "Parsing binding: %s", binding->lhs);

    len = getNextToken(context, token);
    if ( strcmp(token, ":=") ) return NULL;

    peekNextToken(context, token);

    if ( !strcmp(token, "(") )
    {
        //parse a code block
        binding->function_decl = parseFunctionDecl(context);
    }
    else
    {
        binding->expression = parseExpression(context);
    }

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

