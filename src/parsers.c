#include <string.h>


#include "parsers.h"
#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"

/* int parseExpression(Context*, Expression*); */

/* //MathFactor         = "(" Expression ")" | NUMBER */
/* int parseMathFactor(Context* context, MathFactor* factor) */
/* { */
/*     int result = parseLiteral(context, "("); */
/*     if ( result != FAIL ) */
/*     { */
/*         factor->expression = ALLOC(Expression); */
/*         result = parseExpression(context, factor->expression); */
/*         if ( result == FAIL ) return FAIL; */

/*         result = parseLiteral(context, ")"); */
/*         if ( result == FAIL ) return FAIL; */

/*         return OK; */
/*     } */

/*     char num1[16]; */
/*     result = parseNumber(context, num1); */
/*     if ( result != FAIL ) */
/*     { */
/*         factor->number = atoi(num1); */
/*         return OK; */
/*     } */

/*     return OK; */
/* } */

/* //MathExpression     = MathFactor ("+"|"-"|"*"|"/"|"%"|"%%") MathExpression | MathFactor */
/* int parseMathExpression(Context* context, MathExpression* exp) */
/* { */
/*     exp->factor = ALLOC(MathFactor); */
/*     int result = parseMathFactor(context, exp->factor); */
/*     if ( result == FAIL ) return FAIL; */

/*     const char* ops[] = { "%%", "+", "-", "*", "/", "%" }; */
/*     result = parseMultipleChoiceLiteral(context, 6, ops); */
/*     debugLog(context, "result of operator check: %d", result); */

/*     if ( result == FAIL ) { */
/*         exp->op = OP_NOP; */
/*         return OK; */
/*     } */
/*     exp->op = result; */
    
/*     exp->expression = ALLOC(MathExpression); */
/*     result = parseMathExpression(context, exp->expression); */
/*     if ( result == FAIL ) return FAIL; */

/*     return OK; */
/* } */

int parseBasicExpression(Context* context, BasicExpression* basic_expression)
{
    char num[16];
    int result = parseNumber(context, num);
    if ( result != FAIL )
    {
        basic_expression->number = atoi(num);
        return OK;
    }

    //TODO: complete this part
    basic_expression->number = 19;
    return OK;

}

int parsePrimaryExpression(Context* context, PrimaryExpression* primary_expression)
{
    primary_expression->basic_expression = ALLOC(BasicExpression);
    int result = parseBasicExpression(context, primary_expression->basic_expression);

    if ( result == FAIL ) return FAIL;

    struct PrimaryExpressionElement* element = ALLOC(struct PrimaryExpressionElement);
    element->op = OP_NOP;
    //TODO: scan for ( or dot or [
    //
    
    return OK;
}

int parseUnaryExpression(Context* context, UnaryExpression* unary_expression)
{
    unary_expression->op = OP_NOP;
    unary_expression->primary_expression = ALLOC(PrimaryExpression);

    return parsePrimaryExpression(context, unary_expression->primary_expression);
    //TODO: scan for operators
}

int parseMulExpression(Context* context, MulExpression* mul_expression)
{
    struct MulExpressionElement* element = ALLOC(struct MulExpressionElement);

    mul_expression->first_element = mul_expression->last_element = element;
    element->unary_expression = ALLOC(UnaryExpression);

    element->op = OP_NOP;
    return parseUnaryExpression(context, element->unary_expression);
    //TODO: scan for operators
}

int parseAddExpression(Context* context, AddExpression* add_expression)
{
    struct AddExpressionElement* element = ALLOC(struct AddExpressionElement);

    add_expression->first_element = add_expression->last_element = element;
    element->mul_expression = ALLOC(MulExpression);

    element->op = OP_NOP;
    return parseMulExpression(context, element->mul_expression);
    //TODO: scan for operators
}

int parseShiftExpression(Context* context, ShiftExpression* shift_expression)
{
    struct ShiftExpressionElement* element = ALLOC(struct ShiftExpressionElement);

    shift_expression->first_element = shift_expression->last_element = element;
    element->add_expression = ALLOC(AddExpression);

    element->op = OP_NOP;
    return parseAddExpression(context, element->add_expression);
    //TODO: scan for operators
}

int parseCmpExpression(Context* context, CmpExpression* cmp_expression)
{
    struct CmpExpressionElement* element = ALLOC(struct CmpExpressionElement);

    cmp_expression->first_element = cmp_expression->last_element = element;
    element->shift_expression = ALLOC(ShiftExpression);

    element->op = OP_NOP;
    return parseShiftExpression(context, element->shift_expression);
    //TODO: scan for operators
}

int parseEqExpression(Context* context, EqExpression* eq_expression)
{
    struct EqExpressionElement* element = ALLOC(struct EqExpressionElement);

    eq_expression->first_element = eq_expression->last_element = element;
    element->cmp_expression = ALLOC(CmpExpression);

    element->op = OP_NOP;
    return parseCmpExpression(context, element->cmp_expression);
    //TODO: scan for operators
}

//Expression         = MathExpression 
int parseExpression(Context* context, Expression* exp)
{
    struct ExpressionElement* element = ALLOC(struct ExpressionElement);

    exp->first_element = exp->last_element = element;
    element->eq_expression = ALLOC(EqExpression);

    element->op = OP_NOP;
    return parseEqExpression(context, element->eq_expression);
    //TODO: scan for and/or/xor/...
}

int parseFunctionDecl(Context* context, FunctionDecl* function_decl)
{
    int result = parseLiteral(context, "()");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(context, "->");
    if ( result == FAIL ) return FAIL;

    function_decl->expression = ALLOC(Expression);

    return parseExpression(context, function_decl->expression);
}

//StaticBinding  = BindingLhs+ ":=" ( FunctionDecl )
//FunctionDecl   = "(" ") ->" Expression
int parseBinding(Context* context, Binding* b)
{
    char token[256];
    int result = parseIdentifier(context, token);
    if ( result == FAIL ) return FAIL;
    strcpy(b->lhs, token);

    result = parseLiteral(context, ":=");
    if ( result == FAIL ) return FAIL;

    b->function_decl = ALLOC(FunctionDecl);
    return parseFunctionDecl(context, b->function_decl);
}

//Module            = { ( StaticBinding ) }
int parseModule(Context* context, Module* module)
{
    struct ModuleElement* element = ALLOC(struct ModuleElement);
    element->binding = ALLOC(Binding);

    module->first_element = module->last_element = element;

    return parseBinding(context, element->binding);
}

