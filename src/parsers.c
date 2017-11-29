#include <string.h>


#include "parsers.h"
#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"

BasicExpression* parseBasicExpression(Context* context)
{
    BasicExpression* basic_expression = ALLOC(BasicExpression);

    char num[16];
    int result = parseNumber(context, num);
    if ( result != FAIL )
    {
        basic_expression->number = atoi(num);
        return basic_expression;
    }

    //TODO: complete this part
    basic_expression->number = 19;
    return basic_expression;

}

PrimaryExpression* parsePrimaryExpression(Context* context)
{
    PrimaryExpression* primary_expression = ALLOC(PrimaryExpression);
    PARSE(primary_expression->basic_expression, parseBasicExpression);

    struct PrimaryExpressionElement* element = ALLOC(struct PrimaryExpressionElement);
    element->op = OP_NOP;
    //TODO: scan for ( or dot or [
    //
    
    return primary_expression;
}

UnaryExpression* parseUnaryExpression(Context* context)
{
    UnaryExpression* unary_expression = ALLOC(UnaryExpression);

    unary_expression->op = OP_NOP;

    unary_expression->primary_expression = parsePrimaryExpression(context);
    //TODO: scan for operators
    return unary_expression;
}

MulExpression* parseMulExpression(Context* context)
{
    MulExpression* mul_expression = ALLOC(MulExpression);
    struct MulExpressionElement* element = ALLOC(struct MulExpressionElement);

    mul_expression->first_element = mul_expression->last_element = element;

    element->op = OP_NOP;
    element->unary_expression =  parseUnaryExpression(context);

    //TODO: scan for operators
    return mul_expression;
}

AddExpression* parseAddExpression(Context* context)
{
    AddExpression* add_expression = ALLOC(AddExpression);

    struct AddExpressionElement* element = ALLOC(struct AddExpressionElement);

    add_expression->first_element = add_expression->last_element = element;
    element->mul_expression = parseMulExpression(context);

    //for the first element there is no op
    element->op = OP_NOP;

    const char* ops[] = { "+", "-" };
    int result = parseMultipleChoiceLiteral(context, 2, ops);

    //it's fine if we no longer see operators
    while ( result == OK ) 
    {
        element->next = ALLOC(struct AddExpressionElement);
        element = element->next;

        element->op = (result==0) ? OP_ADD:OP_SUB;
        element->mul_expression = parseMulExpression(context);

        CHECK_FAIL(result);

        add_expression->last_element = element;

        result = parseMultipleChoiceLiteral(context, 2, ops);
    }

    //TODO: scan for operators
    return add_expression;
}

ShiftExpression* parseShiftExpression(Context* context)
{
    ShiftExpression* shift_expression = ALLOC(ShiftExpression);

    struct ShiftExpressionElement* element = ALLOC(struct ShiftExpressionElement);

    shift_expression->first_element = shift_expression->last_element = element;

    element->op = OP_NOP;
    element->add_expression = parseAddExpression(context);
    //TODO: scan for operators
    //
    return shift_expression;
}

CmpExpression* parseCmpExpression(Context* context)
{
    CmpExpression* cmp_expression = ALLOC(CmpExpression);

    struct CmpExpressionElement* element = ALLOC(struct CmpExpressionElement);

    cmp_expression->first_element = cmp_expression->last_element = element;

    element->op = OP_NOP;
    element->shift_expression = parseShiftExpression(context);
    CHECK_NULL(element->shift_expression);
    //TODO: scan for operators
    return cmp_expression;
}

EqExpression* parseEqExpression(Context* context)
{
    EqExpression* eq_expression = ALLOC(EqExpression);

    struct EqExpressionElement* element = ALLOC(struct EqExpressionElement);

    eq_expression->first_element = eq_expression->last_element = element;

    element->op = OP_NOP;
    element->cmp_expression = parseCmpExpression(context);
    CHECK_NULL(element->cmp_expression);
    //TODO: scan for operators
    
    return eq_expression;
}

//Expression         = MathExpression 
Expression* parseExpression(Context* context)
{
    Expression* exp = ALLOC(Expression);
    struct ExpressionElement* element = ALLOC(struct ExpressionElement);

    exp->first_element = exp->last_element = element;

    element->op = OP_NOP;
    element->eq_expression = parseEqExpression(context);
    CHECK_NULL(element->eq_expression);
    //TODO: scan for and/or/xor/...
    
    return exp;
}

FunctionDecl* parseFunctionDecl(Context* context)
{
    FunctionDecl* function_decl = ALLOC(FunctionDecl);

    int result = parseLiteral(context, "()");
    CHECK_FAIL(result);

    result = parseLiteral(context, "->");
    CHECK_FAIL(result);

    function_decl->expression = parseExpression(context);
    CHECK_NULL(function_decl->expression);

    return function_decl;
}

//StaticBinding  = BindingLhs+ ":=" ( FunctionDecl )
//FunctionDecl   = "(" ") ->" Expression
Binding* parseBinding(Context* context)
{
    Binding* binding = ALLOC(Binding);

    char token[256];
    int result = parseIdentifier(context, token);
    if ( result == FAIL ) return NULL;
    strcpy(binding->lhs, token);

    result = parseLiteral(context, ":=");
    CHECK_FAIL(result);

    binding->function_decl = parseFunctionDecl(context);
    CHECK_NULL(binding->function_decl);

    return binding;
}

//Module            = { ( StaticBinding ) }
Module* parseModule(Context* context)
{
    Module* module = ALLOC(Module);
    struct ModuleElement* element = ALLOC(struct ModuleElement);
    module->first_element = module->last_element = element;

    element->binding = parseBinding(context);
    CHECK_NULL(element->binding);

    return module;
}

