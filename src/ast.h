#ifndef __AST_H__
#define __AST_H__

#include <stdio.h>
#include <stdlib.h>

#include "llvm-c/Core.h"

#define OK   1
#define FAIL -1

#define ALLOC(V, T)  T* V = (T*)calloc(1, sizeof(T))
#define PARSE(R, F) R = F(context); if ( R == NULL ) return NULL

#define SAVE_POSITION long initial_position = ftell(context->input_file)
#define RESTORE_POSITION fseek(context->input_file, initial_position, SEEK_SET)
#define CHECK_FAIL(x) if ( x == FAIL ) return NULL
#define CHECK_NULL(x) if ( x == NULL ) return NULL

#define OP_NOP 0
#define OP_AND 1
#define OP_OR  2
#define OP_XOR 3
#define OP_EQ  4
#define OP_NE  5
#define OP_GT  6
#define OP_LT  7
#define OP_GE  8
#define OP_LE  9
#define OP_SHR 10
#define OP_SHL 11
#define OP_POW 12
#define OP_ADD 13
#define OP_SUB 14
#define OP_MUL 15
#define OP_DIV 16
#define OP_REM 17
#define OP_DVT 18  //divisibility test
#define OP_NOT 19
#define OP_NEG 20
#define OP_DOT 21
#define OP_BRC 22  //braces a[1]

typedef struct
{
    char* input_file_path;
    char  input_file_name[1024];
    FILE* input_file;
    int   debug_mode;
    char  output_file_path[1024];

    char llvmir_dir[1024];
    char llvmir_file_path[1024];

    LLVMModuleRef module;
    LLVMBuilderRef builder;

} Context;


typedef struct Binding Binding;
typedef struct FunctionDecl FunctionDecl;
typedef struct CodeBlock CodeBlock;
typedef struct Expression Expression;
typedef struct EqExpression EqExpression;
typedef struct CmpExpression CmpExpression;
typedef struct ShiftExpression ShiftExpression;
typedef struct AddExpression AddExpression;
typedef struct MulExpression MulExpression;
typedef struct UnaryExpression UnaryExpression;
typedef struct PrimaryExpression PrimaryExpression;
typedef struct BasicExpression BasicExpression;
typedef struct TermExpression TermExpression;

typedef struct Module
{
    struct ModuleElement
    {
        Binding* binding;
        struct ModuleElement* next;
    } *first_element, *last_element;
} Module;

typedef struct Binding
{
    char lhs[32];
    FunctionDecl* function_decl;
} Binding;

typedef struct FunctionDecl
{
    CodeBlock* code_block;
    Expression* expression;
} FunctionDecl;

typedef struct CodeBlock
{
    struct CodeBlockElement
    {
        Binding* binding;
        Expression* return_expression;

        struct CodeBlockElement* next;
    } *first_element, *last_element;

} CodeBlock;

typedef struct Expression
{
    struct ExpressionElement
    {
        int op;
        EqExpression* eq_expression;
        struct ExpressionElement* next;
    } *first_element, *last_element;
} Expression;

typedef struct EqExpression
{
    struct EqExpressionElement
    {
        int op;
        CmpExpression* cmp_expression;
        struct EqExpressionElement* next;
    } *first_element, *last_element;
} EqExpression;

typedef struct CmpExpression
{
    struct CmpExpressionElement
    {
        int op;
        ShiftExpression* shift_expression;
        struct CmpExpressionElement* next;
    } *first_element, *last_element;
} CmpExpression;

typedef struct ShiftExpression
{
    struct ShiftExpressionElement
    {
        int op;
        AddExpression* add_expression;
        struct ShiftExpressionElement* next;
    } *first_element, *last_element;
} ShiftExpression;

typedef struct AddExpression
{
    struct AddExpressionElement
    {
        int op;
        MulExpression* mul_expression;
        struct AddExpressionElement* next;
    } *first_element, *last_element;
} AddExpression;

typedef struct MulExpression
{
    struct MulExpressionElement
    {
        int op;
        UnaryExpression* unary_expression;
        struct MulExpressionElement* next;
    } *first_element, *last_element;
} MulExpression;

typedef struct UnaryExpression
{
    int op;
    PrimaryExpression* primary_expression;
} UnaryExpression;

typedef struct PrimaryExpression
{
    BasicExpression* basic_expression;
    struct PrimaryExpressionElement
    {
        int op;
        TermExpression* term_expression;
        struct PrimaryExpressionElement* next;
    } *first_element, *last_element;

} PrimaryExpression;

typedef struct BasicExpression
{
    char binding_name[32];
    Expression* expression;
    int number;
} BasicExpression;

typedef struct TermExpression
{
    Expression* expression;
    char binding_name[32];
} TermExpression;

#endif
