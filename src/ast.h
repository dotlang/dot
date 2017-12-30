#ifndef __AST_H__
#define __AST_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "llvm-c/Core.h"
#include "hash.h"

#define ALLOC(V, T)  T* V = (T*)calloc(1, sizeof(T))
#define ALLOC_ARRAY(V, C, T)  T* V = (T*)calloc(C, sizeof(T))
#define ALLOC_NODE(V, TOKEN, KIND )\
                ALLOC(V, ExpressionNode);\
                strcpy(V->token, TOKEN);\
                V->kind = kind
#define CHAIN_LIST(FIELD, PREV, CURR) \
                if ( FIELD == NULL ) FIELD = CURR;\
                if ( PREV != NULL ) PREV->next = CURR;\
                PREV = CURR

typedef struct
{
    //TODO: why Module* is not part of context?
    //TODO: bundle everything related to compilation in one struct
    //and everything about a single compilation module into another
    char* input_file_path;
    char  input_file_name[1024];
    FILE* input_file;
    int   debug_mode;
    char  output_file_path[1024];

    char llvmir_dir[1024];
    char llvmir_file_path[1024];

    LLVMModuleRef module;
    LLVMBuilderRef builder;
    //List of bindings defined inside current function
    hashtable_t* function_bindings;
} Context;

typedef struct Binding Binding;

typedef enum
{
    NA_TYPE,
    INT,
    FLOAT,
    BOOL,
    CHAR
} ExpressionType;

typedef enum
{
    NA,
    OP_EQUALS,
    INT_LITERAL,
    FLOAT_LITERAL,
    BOOL_LITERAL,
    CHAR_LITERAL,
    IDENTIFIER,
    OPEN_PAREN,
    CLOSE_PAREN,
    OP_ADD,
    OP_SUB,
    OP_NEG, //unary -
    OP_POS, //unary +
    OP_SHR,
    OP_SHL,
    OP_MUL,
    OP_DIV,
    OP_REM,
    OP_DVT,
    OPEN_BRACE,
    CLOSE_BRACE,
    COMMA,
    FN_CALL,
    FN_CALL_SIMPLE, //function call without arg
    OP_BIND,
    OP_ARROW,
    OP_RETURN,
    OP_COLON
} TokenKind;

typedef struct ExpressionNode
{
    char token[32];
    TokenKind kind;
    struct ExpressionNode* next;

    //extra fields - to be updated during validation
    ExpressionType ex_type;
} ExpressionNode;

typedef struct Expression
{
    ExpressionNode *first_node;

    //extra fields - to be updated during validation
    ExpressionType ex_type;
} Expression;

typedef struct 
{
    Binding *first_binding;
} Module;

typedef struct ArgDef
{
    char name[32];
    char type[32];

    struct ArgDef* next;
} ArgDef;

typedef struct 
{
    Binding *first_binding;
    ArgDef *first_arg;
    char output_type[32];
} FunctionDecl;

typedef struct Binding
{
    char lhs[32];

    //true, if this is a binding inside a function_decl and is prefixed with `::`
    bool is_return;

    //A binding can be either an expression or a function declaration
    FunctionDecl* function_decl;
    Expression* expression;

    //declared type `x:int := 10`
    char decl_type[64];

    struct Binding* next;

    //extra fields - to be updated during validation
    ExpressionType ex_type;
} Binding;


typedef struct FunctionArgList
{
    struct FunctionArg
    {
        LLVMValueRef arg;
        struct FunctionArg* next;
    } *first_arg;
} FunctionArgList;


#endif
