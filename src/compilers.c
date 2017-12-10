#include "compilers.h"

void compileBinding(Context* context, Binding* binding);

void compileFunctionDecl(Context* context, LLVMValueRef function, FunctionDecl* function_decl)
{
    ArgDef* current_arg = function_decl->first_arg;

    //clear list of function bindings to prevent name clash between functions
	context->function_bindings = ht_create(100);

    //first we need to allocate function inputs
    int i = 0;
    while ( current_arg != NULL )
    {
        LLVMValueRef arg_ref = LLVMGetParam(function, i);

        LLVMTypeRef arg_type = expressionTypeToLLVMType(current_arg->type);
        LLVMValueRef alloc_ref = LLVMBuildAlloca(context->builder, arg_type, current_arg->name);
        LLVMBuildStore(context->builder, arg_ref, alloc_ref);
        ht_set(context->function_bindings, current_arg->name, alloc_ref);

        current_arg = current_arg->next;
        i++;
    }

    Binding* binding = function_decl->first_binding;
    while ( binding != NULL )
    {
        compileBinding(context, binding);
        binding = binding->next;
    }

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

        compileFunctionDecl(context, mainfunc, binding->function_decl);
    }
    else
    {
        //we have a named binding which is not returned value
        LLVMValueRef r_value = compileExpression(context, binding->expression);
        LLVMTypeRef binding_type = expressionTypeToLLVMType(binding->decl_type);
        LLVMValueRef alloc_ref = LLVMBuildAlloca(context->builder, binding_type, binding->lhs);
        LLVMBuildStore(context->builder, r_value, alloc_ref);

        ht_set(context->function_bindings, binding->lhs, alloc_ref);
    }
}

void declareBinding(Context* context, Binding* binding)
{
    if ( binding->function_decl != NULL )
    {
        LLVMTypeRef func_type = getFunctionType(context, binding->function_decl);
        LLVMAddFunction(context->module, binding->lhs, func_type);
    }
}

void addPredefinedFunctions(Context* context)
{
    LLVMTypeRef input_types[] = { LLVMInt1Type() };

    LLVMTypeRef func_type = LLVMFunctionType(LLVMInt64Type(), input_types, 1, 0);
    LLVMValueRef main_func = LLVMAddFunction(context->module, "bool_to_int", func_type);

    //Cast float to int
    LLVMTypeRef input_types2[] = { LLVMDoubleType() };
    func_type = LLVMFunctionType(LLVMInt64Type(), input_types2, 1, 0);
    main_func = LLVMAddFunction(context->module, "float_to_int", func_type);

    //cast char to int
    LLVMTypeRef input_types3[] = { LLVMInt8Type() };
    func_type = LLVMFunctionType(LLVMInt64Type(), input_types3, 1, 0);
    main_func = LLVMAddFunction(context->module, "char_to_int", func_type);

    //assert function
    LLVMTypeRef input_types4[] = { LLVMInt1Type() };
    func_type = LLVMFunctionType(LLVMInt64Type(), input_types4, 1, 0);
    main_func = LLVMAddFunction(context->module, "assert", func_type);

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

    addPredefinedFunctions(context);

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

