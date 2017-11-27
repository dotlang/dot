#ifndef __AST_H__
#define __AST_H__

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
    MathFactor* lhs;
    struct MathExpression* rhs;
    //can be "+"|"-"|"*"|"/"|"%"|"%%"
    char        op;
} MathExpression;

typedef struct MathFactor
{
    Expression* expression;
    int number;
} MathFactor;





#endif
