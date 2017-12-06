#include "compilers.h"

void compileBinding(Context* context, Binding* binding);

void compileFunctionDecl(Context* context, FunctionDecl* function_decl)
{
    Binding* binding = function_decl->first_binding;
    while ( binding != NULL )
    {
        compileBinding(context, binding);
        binding = binding->next;
    }

}

LLVMTypeRef getFunctionType(FunctionDecl* function_decl)
{
    return LLVMFunctionType(LLVMInt64Type(), NULL, 0, 0);
}

LLVMTypeRef getBindingType(Binding* binding)
{
    return LLVMIntType(64);
}

void compileBinding(Context* context, Binding* binding)
{
    if ( binding->is_return )
    {
        LLVMValueRef r_value = compileExpression(context, binding->expression);
        LLVMBuildRet(context->builder, r_value);
    }
    else if ( binding->function_decl != NULL )
    {
        LLVMValueRef mainfunc = LLVMGetNamedFunction(context->module, binding->lhs);

        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
        LLVMPositionBuilderAtEnd(context->builder, entry);

        compileFunctionDecl(context, binding->function_decl);
    }
    else
    {
        //we have a named binding which is not returned value
        LLVMValueRef r_value = compileExpression(context, binding->expression);
        LLVMTypeRef binding_type = getBindingType(binding);
        LLVMValueRef alloc_ref = LLVMBuildAlloca(context->builder, binding_type, binding->lhs);
        LLVMBuildStore(context->builder, r_value, alloc_ref);

        ht_set(context->function_bindings, binding->lhs, alloc_ref);
    }
}

void declareBinding(Context* context, Binding* binding)
{
    if ( binding->function_decl != NULL )
    {
        LLVMTypeRef func_type = getFunctionType(binding->function_decl);
        LLVMAddFunction(context->module, binding->lhs, func_type);
    }
}

void compileModule(Context* context, Module* m)
{
    context->module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(context->module, "");
    LLVMSetTarget(context->module, LLVMGetDefaultTargetTriple());
    context->builder = LLVMCreateBuilder();

    Binding* binding = m->first_binding;
    while ( binding != NULL )
    {
        declareBinding(context, binding);
        binding = binding->next;
    }

    binding = m->first_binding;
    while ( binding != NULL )
    {
        //for module level bindings we dont need a storage as we can lookup functions using LLVM
        compileBinding(context, binding);
        binding = binding->next;
    }

    char *error = NULL;
    bool is_invalid = LLVMVerifyModule(context->module, LLVMAbortProcessAction, &error);
    if ( is_invalid )
    {
        printf("%s\n", error);
        return;
    }

    LLVMPrintModuleToFile(context->module, context->llvmir_file_path, &error);
    LLVMDisposeMessage(error);
}

void disposeLlvm(Context* context)
{
    LLVMDisposeBuilder(context->builder);
    LLVMDisposeModule(context->module);
}

