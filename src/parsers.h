#ifndef __PARSERS_H__
#define __PARSERS_H__

#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"

int parseExpression(Context*, Expression*);

//MathFactor         = "(" Expression ")" | NUMBER
int parseMathFactor(Context* context, MathFactor* factor)
{
    int result = parseLiteral(context, "(");
    if ( result != FAIL )
    {
        factor->expression = ALLOC(Expression);
        result = parseExpression(context, factor->expression);
        if ( result == FAIL ) return FAIL;

        result = parseLiteral(context, ")");
        if ( result == FAIL ) return FAIL;

        return OK;
    }

    char num1[16];
    result = parseNumber(context, num1);
    if ( result != FAIL )
    {
        factor->number = atoi(num1);
        return OK;
    }

    return OK;
}

//MathExpression     = MathFactor ("+"|"-"|"*"|"/"|"%"|"%%") MathExpression | MathFactor
int parseMathExpression(Context* context, MathExpression* exp)
{
    exp->factor = ALLOC(MathFactor);
    int result = parseMathFactor(context, exp->factor);
    if ( result == FAIL ) return FAIL;

    const char* ops="+-*/";
    result = parseMultipleChoiceLiteral(context, ops);

    if ( result == FAIL ) {
        exp->op = 0;
        return OK;
    }

    exp->op = ops [result];
    
    exp->expression = ALLOC(MathExpression);
    result = parseMathExpression(context, exp->expression);
    if ( result == FAIL ) return FAIL;

    return OK;
}

//Expression         = MathExpression 
int parseExpression(Context* context, Expression* exp)
{
    exp->math_expression = ALLOC(MathExpression);
    int result = parseMathExpression(context, exp->math_expression);

    if ( result == FAIL ) return FAIL;
    debugExpression(context, exp);
    debugLog(context,"");

    return OK;
}

//StaticBinding  = BindingLhs+ ":=" ( FunctionDecl )
//FunctionDecl   = "(" ") ->" Expression
int parseStaticBinding(Context* context, StaticBinding* b)
{
    char token[256];
    int result = parseIdentifier(context, token);
    if ( result == FAIL ) return FAIL;
    strcpy(b->lhs, token);

    result = parseLiteral(context, ":=");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(context, "()");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(context, "->");
    if ( result == FAIL ) return FAIL;

    b->function_decl = ALLOC(FunctionDecl);
    b->function_decl->expression = ALLOC(Expression);

    result = parseExpression(context, b->function_decl->expression);
    if ( result == FAIL ) return FAIL;

    return OK;
}

//Module            = { ( StaticBinding ) }
int parseModule(Context* context, Module* module)
{
    StaticBinding* binding = ALLOC(StaticBinding);
    ModuleItem* item = ALLOC(ModuleItem);
    item->static_binding = binding;

    module->items_head = module->items_tail = item;
    int result = parseStaticBinding(context, binding);

    return result;
}



#endif
