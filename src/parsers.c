#include <string.h>


#include "parsers.h"
#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"


Expression* parseExpression(Context* context);


BasicExpression* parseBasicExpression(Context* context)
{
    ALLOC(basic_expression, BasicExpression);

    char num[16];
    int result = parseNumber(context, num);
    if ( result != FAIL )
    {
        basic_expression->number = atoi(num);
        return basic_expression;
    }

    if ( parseLiteral(context, "(") == OK )
    {
        PARSE(basic_expression->expression, parseExpression);
        if ( parseLiteral(context, ")") == FAIL ) return NULL;
        return basic_expression;
    }

    return NULL;
}

PrimaryExpression* parsePrimaryExpression(Context* context)
{
    ALLOC(primary_expression, PrimaryExpression);
    PARSE(primary_expression->basic_expression, parseBasicExpression);

    ALLOC(element, struct PrimaryExpressionElement);
    element->op = OP_NOP;
    //TODO: scan for ( or dot or [
    //
    
    return primary_expression;
}

UnaryExpression* parseUnaryExpression(Context* context)
{
    ALLOC(unary_expression, UnaryExpression);

    unary_expression->op = OP_NOP;

    PARSE(unary_expression->primary_expression, parsePrimaryExpression);
    //TODO: scan for operators
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
    const char* op = parseMultipleChoiceLiteral(context, 4, ops);

    while ( op != NULL )
    {
        ALLOC(element_next, struct MulExpressionElement);
        element->next = element_next;
        element = element->next;

        element->op = strToOp(op);

        PARSE(element->unary_expression, parseUnaryExpression);

        op = parseMultipleChoiceLiteral(context, 4, ops);
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
    const char* op = parseMultipleChoiceLiteral(context, 2, ops);

    //it's fine if we no longer see operators
    while ( op != NULL ) 
    {
        ALLOC(element_next, struct AddExpressionElement);
        element->next = element_next;
        element = element->next;

        element->op = strToOp(op);

        PARSE(element->mul_expression, parseMulExpression);

        op = parseMultipleChoiceLiteral(context, 2, ops);
    }
    add_expression->last_element = element;

    //TODO: scan for operators
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
    ALLOC(element, struct CodeBlockElement);

    int result = parseLiteral(context, "::");
    if ( result == OK )
    {
        PARSE(element->return_expression, parseExpression);
    }
    else
    {
        //for now we only should have one return inside the code block
        abort();
        /* element->binding = parseBinding(context); */
        /* if ( element->binding == NULL ) return code_block; */
    }
    code_block->first_element = code_block->last_element = element;

    return code_block;
}

FunctionDecl* parseFunctionDecl(Context* context)
{
    ALLOC(function_decl, FunctionDecl);

    int result = parseLiteral(context, "()");
    CHECK_FAIL(result);

    result = parseLiteral(context, "->");
    CHECK_FAIL(result);

    result = parseLiteral(context, "int");
    if ( result == OK )
    {
        result = parseLiteral(context, "{");
        if ( result == OK )
        {
            PARSE(function_decl->code_block, parseCodeBlock);
            result = parseLiteral(context, "}");
            if ( result == FAIL ) return NULL;
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

    result = parseLiteral(context, ":=");
    CHECK_FAIL(result);

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

