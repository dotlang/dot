#include "compile_helper.h"



ExpressionType getType(LLVMValueRef value)
{
    LLVMTypeRef type_ref = LLVMTypeOf(value);
    char* type_str = LLVMPrintTypeToString(type_ref);

    if ( !strcmp(type_str, "double") ) return FLOAT;
    if ( !strcmp(type_str, "i1") ) return BOOL;
    if ( !strcmp(type_str, "i64") ) return INT;

    errorLog("Invliad type to get string representation: %s", type_str);
    abort();
}
