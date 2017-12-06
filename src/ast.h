#ifndef __AST_H__
#define __AST_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "llvm-c/Core.h"
#include "hash.h"

#define ALLOC(V, T)  T* V = (T*)calloc(1, sizeof(T))
#define ALLOC_NODE(V, TOKEN, KIND )\
                ALLOC(V, ExpressionNode);\
                strcpy(V->token, TOKEN);\
                V->kind = kind
/* #define PARSE(R, F) R = F(context); if ( R == NULL ) return NULL */
/* #define PARSE_ELSE(R, F) R = F(context); if ( R == NULL ) */

/* #define EXPECT(x)     if ( matchLiteral(context, x) == false ) return NULL */
/* #define IF_MATCH(x)   if ( matchLiteral(context, x) == true ) */

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
    //This is a map of non-function bindings defined at module level
    /* hashtable_t* module_value_bindings; */
    //List of bindings defined inside current function
    hashtable_t* function_bindings;
    char token_cache[256];

} Context;

typedef struct Binding Binding;

typedef enum
{
    NA,
    INT_LITERAL,
    IDENTIFIER,
    OPEN_PAREN,
    CLOSE_PAREN,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_REM,
    OP_DVT,
    OPEN_BRACE,
    CLOSE_BRACE,
    OP_COMMA,
    COMMA,
    FN_CALL
} TokenKind;

typedef struct ExpressionNode
{
    char token[32];
    TokenKind kind;
    int arg_count;

    struct ExpressionNode* next;
} ExpressionNode;

typedef struct Expression
{
    ExpressionNode *first_node, *last_node;
} Expression;

typedef struct 
{
    Binding* first_binding, *last_binding;
} Module, FunctionDecl;

typedef struct Binding
{
    char lhs[32];

    //true, if this is a binding inside a function_decl and is prefixed with `::`
    bool is_return;

    //A binding can be either an expression or a function declaration
    FunctionDecl* function_decl;
    Expression* expression;

    struct Binding* next;
} Binding;


#endif
