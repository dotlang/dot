#include <string.h>

#include "parsers.h"
#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"


Expression* parseExpression(Context* context);
Binding* parseBinding(Context* context);


BasicExpression* parseBasicExpression(Context* context)
{
    ALLOC(basic_expression, BasicExpression);

    char num[16];
    if ( matchNumber(context, num) )
    {
        basic_expression->number = atoi(num);
        return basic_expression;
    }

    IF_MATCH("(")
    {
        PARSE(basic_expression->expression, parseExpression);
        IF_MATCH(")") return basic_expression;
    }

    return NULL;
}

PrimaryExpression* parsePrimaryExpression(Context* context)
{
    ALLOC(primary_expression, PrimaryExpression);
    PARSE(primary_expression->basic_expression, parseBasicExpression);

    return primary_expression;
}

UnaryExpression* parseUnaryExpression(Context* context)
{
    ALLOC(unary_expression, UnaryExpression);
    PARSE(unary_expression->primary_expression, parsePrimaryExpression);

    return unary_expression;
}

MulExpression* parseMulExpression(Context* context)
{
    ALLOC(mul_expression, MulExpression);
    ALLOC(element, struct MulExpressionElement);

    mul_expression->first_element = mul_expression->last_element = element;
    PARSE(element->unary_expression, parseUnaryExpression);

    element->op = OP_NOP;

    const char* ops[] = { "*", "/", "%%", "%" };
    const char* op = matchLiterals(context, ops, 4);

    while ( op != NULL )
    {
        ALLOC(element_next, struct MulExpressionElement);
        element->next = element_next;
        element = element->next;

        element->op = strToOp(op);

        PARSE(element->unary_expression, parseUnaryExpression);

        op = matchLiterals(context, ops, 4);
    }

    mul_expression->last_element = element;

    //TODO: scan for operators
    return mul_expression;
}

AddExpression* parseAddExpression(Context* context)
{
    ALLOC(add_expression, AddExpression);
    ALLOC(element, struct AddExpressionElement);

    add_expression->first_element = add_expression->last_element = element;
    PARSE(element->mul_expression, parseMulExpression);

    //for the first element there is no op
    element->op = OP_NOP;

    const char* ops[] = { "+", "-" };
    const char* op = matchLiterals(context, ops, 2);

    //it's fine if we no longer see operators
    while ( op != NULL ) 
    {
        ALLOC(element_next, struct AddExpressionElement);
        element->next = element_next;
        element = element->next;

        element->op = strToOp(op);

        PARSE(element->mul_expression, parseMulExpression);

        op = matchLiterals(context, ops, 2);
    }
    add_expression->last_element = element;

    return add_expression;
}

ShiftExpression* parseShiftExpression(Context* context)
{
    ALLOC(shift_expression, ShiftExpression);
    ALLOC(element, struct ShiftExpressionElement);

    shift_expression->first_element = shift_expression->last_element = element;

    element->op = OP_NOP;
    PARSE(element->add_expression, parseAddExpression);
    //TODO: scan for operators
    //
    return shift_expression;
}

CmpExpression* parseCmpExpression(Context* context)
{
    ALLOC(cmp_expression, CmpExpression);
    ALLOC(element, struct CmpExpressionElement);

    cmp_expression->first_element = cmp_expression->last_element = element;

    element->op = OP_NOP;
    PARSE(element->shift_expression, parseShiftExpression);
    //TODO: scan for operators
    return cmp_expression;
}

EqExpression* parseEqExpression(Context* context)
{
    ALLOC(eq_expression, EqExpression);
    ALLOC(element, struct EqExpressionElement);

    eq_expression->first_element = eq_expression->last_element = element;

    element->op = OP_NOP;
    PARSE(element->cmp_expression, parseCmpExpression);
    //TODO: scan for operators
    
    return eq_expression;
}

//Expression         = MathExpression 
Expression* parseExpression(Context* context)
{
    ALLOC(exp, Expression);
    ALLOC(element, struct ExpressionElement);

    exp->first_element = exp->last_element = element;

    element->op = OP_NOP;
    PARSE(element->eq_expression, parseEqExpression);
    //TODO: scan for and/or/xor/...
    
    return exp;
}

CodeBlock* parseCodeBlock(Context* context)
{
    ALLOC(code_block, CodeBlock);
    struct CodeBlockElement* element = NULL;

    while ( 1 )
    {
        IF_MATCH("}")
        {
            code_block->last_element = element;
            return code_block;
        }

        ALLOC(temp_element, struct CodeBlockElement);
        if ( code_block->first_element == NULL )
        {
            code_block->first_element = temp_element;
            element = temp_element;
        }
        else
        {
            element->next = temp_element;
            element = element->next;
        }

        IF_MATCH("::")
        {
            PARSE(element->return_expression, parseExpression);
        }
        else
        {
            PARSE(element->binding, parseBinding);
        }
    }
}

FunctionDecl* parseFunctionDecl(Context* context)
{
    ALLOC(function_decl, FunctionDecl);

    EXPECT("()");
    EXPECT("->");

    IF_MATCH("int")
    {
        IF_MATCH("{")
        {
            PARSE(function_decl->code_block, parseCodeBlock);
        }
    }
    else
    {
        PARSE(function_decl->expression, parseExpression);
    }

    return function_decl;
}

//StaticBinding  = BindingLhs+ ":=" ( FunctionDecl )
//FunctionDecl   = "(" ") ->" Expression
Binding* parseBinding(Context* context)
{
    ALLOC(binding, Binding);

    char token[256];
    int result = parseIdentifier(context, token);
    if ( result == FAIL ) return NULL;
    strcpy(binding->lhs, token);

    EXPECT(":=");
    PARSE(binding->function_decl, parseFunctionDecl);

    return binding;
}

//Module            = { ( StaticBinding ) }
Module* parseModule(Context* context)
{
    ALLOC(module, Module);
    ALLOC(element, struct ModuleElement);
    module->first_element = module->last_element = element;

    PARSE(element->binding, parseBinding);

    return module;
}

