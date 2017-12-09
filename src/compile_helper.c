#include "compile_helper.h"



ExpressionType getType(LLVMValueRef value)
{
    LLVMTypeRef type_ref = LLVMTypeOf(value);
    char* type_str = LLVMPrintTypeToString(type_ref);

    if ( !strcmp(type_str, "double") ) return FLOAT;
    if ( !strcmp(type_str, "i1") ) return BOOL;
    if ( !strcmp(type_str, "i64") ) return INT;
    if ( !strcmp(type_str, "i8") ) return CHAR;

    errorLog("Invliad type to get string representation: %s", type_str);
    abort();
}

bool isLiteralKind(TokenKind kind)
{
    return ( kind == INT_LITERAL || kind == BOOL_LITERAL || 
            kind == FLOAT_LITERAL || kind == CHAR_LITERAL );
}

LLVMTypeRef expressionTypeToLLVMType(char* type)
{
    if ( !strcmp(type, "int") ) return LLVMInt64Type();
    if ( !strcmp(type, "float") ) return LLVMDoubleType();
    if ( !strcmp(type, "char") ) return LLVMInt8Type();
    if ( !strcmp(type, "bool") ) return LLVMInt1Type();

    return LLVMInt64Type();
}

LLVMTypeRef getFunctionType(Context* context, FunctionDecl* function_decl)
{
    ALLOC_ARRAY(args, function_decl->arg_count, LLVMTypeRef);

    ArgDef* arg = function_decl->first_arg;
    for(int i=0;i<function_decl->arg_count;i++)
    {
        args[i] = expressionTypeToLLVMType(arg->type);
        arg = arg->next;
    }

    return LLVMFunctionType(expressionTypeToLLVMType(function_decl->output_type), args, function_decl->arg_count, 0);
}

LLVMTypeRef getBindingType(Binding* binding)
{
    //TODO: some places we store char* some places ExpressionType, unify and simplify
    if ( !strcmp(binding->decl_type, "bool") ) return LLVMInt1Type();
    if ( !strcmp(binding->decl_type, "float") ) return LLVMDoubleType();
    if ( !strcmp(binding->decl_type, "char") ) return LLVMInt8Type();

    return LLVMInt64Type();
}

