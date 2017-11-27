#ifndef __AST_H__
#define __AST_H__

/* typedef struct MathExpression */
/* { */
/*     //+, -, * or / or 0 when expression is a single number */
/*     //if op is 0, then we have a single number */
/*     char op; */
/*     union */
/*     { */
/*         struct */
/*         { */
/*             struct MathExpression* lhs; */
/*             struct MathExpression* rhs; */
/*         }; */
/*         int number; */
/*     }; */
/* } MathExpression; */

/* typedef struct */
/* { */
/*     char name[256]; */
/*     MathExpression exp; */

/* } Binding; */

typedef struct Expression Expression;

typedef struct 
{
    Expression* expression;
    int number;
} MathFactor;

typedef struct MathExpression
{
    MathFactor* lhs;
    struct MathExpression* rhs;
    //can be "+"|"-"|"*"|"/"|"%"|"%%"
    char        op;
} MathExpression;

typedef struct Expression
{
    MathExpression* math_expression;
} Expression;

typedef struct 
{
    //CodeBlockItem* code_block_head
    Expression* expression;
} FunctionDecl;

typedef struct
{
    char lhs[32];
    //ImportBinding* import_binding
    //ModuleLiteral* module_literal
    FunctionDecl* function_decl;
} StaticBinding;

typedef struct _ModuleItem
{
    //whichever is not NULL is valid
    StaticBinding* static_binding;
    //NamedType* named_type
    
    struct _ModuleItem* next;
    struct _ModuleItem* prev;
} _ModuleItem;

typedef struct
{
    _ModuleItem* items_head;
    _ModuleItem* items_tail;
} Module;




#endif
