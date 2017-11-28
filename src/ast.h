#ifndef __AST_H__
#define __AST_H__

#include <stdio.h>
#include <stdlib.h>

#include "llvm-c/Core.h"

#define OK   1
#define FAIL -1

#define ALLOC(T)  (T*)calloc(1, sizeof(T))
#define SAVE_POSITION long initial_position = ftell(context->input_file)
#define RESTORE_POSITION fseek(context->input_file, initial_position, SEEK_SET)

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

typedef struct ModuleItem ModuleItem;
typedef struct StaticBinding StaticBinding;
typedef struct Expression Expression;
typedef struct MathExpression MathExpression;
typedef struct FunctionDecl FunctionDecl;
typedef struct MathFactor MathFactor;

typedef struct
{
    ModuleItem* items_head;
    ModuleItem* items_tail;
} Module;

typedef struct ModuleItem
{
    //whichever is not NULL is valid
    StaticBinding* static_binding;
    //NamedType* named_type
    
    struct _ModuleItem* next;
    struct _ModuleItem* prev;
} ModuleItem;

typedef struct StaticBinding
{
    char lhs[32];
    //ImportBinding* import_binding
    //ModuleLiteral* module_literal
    FunctionDecl* function_decl;
} StaticBinding;

typedef struct FunctionDecl
{
    //CodeBlockItem* code_block_head
    Expression* expression;
} FunctionDecl;

typedef struct Expression
{
    MathExpression* math_expression;
} Expression;

typedef struct MathExpression
{
    MathFactor* factor;
    struct MathExpression* expression;
    //can be "+"|"-"|"*"|"/"|"%"|"%%"
    //or 0 if its a simple number
    char        op;
} MathExpression;

typedef struct MathFactor
{
    Expression* expression;
    int number;
} MathFactor;


#endif
