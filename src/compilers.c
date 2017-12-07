#include "compilers.h"

void compileBinding(Context* context, Binding* binding);

LLVMTypeRef expressionTypeToLLVMType(ExpressionType type)
{
    switch ( type )
    {
        case INT: return LLVMInt64Type();
        case CHAR: return LLVMInt8Type();
        case BOOL: return LLVMInt1Type();
        case FLOAT: return LLVMDoubleType();
        default:
            abort();
    }
}

void compileFunctionDecl(Context* context, LLVMValueRef function, FunctionDecl* function_decl)
{
    ArgDef* arg = function_decl->first_arg;

    //first we need to allocate function inputs
    for(int i=0;i<function_decl->arg_count;i++)
    {
        LLVMValueRef arg_ref = LLVMGetParam(function, i);
        printf("adding %s arg as %d\n", arg->name, (int)arg_ref);

        LLVMTypeRef arg_type = expressionTypeToLLVMType(arg->type);
        LLVMValueRef alloc_ref = LLVMBuildAlloca(context->builder, arg_type, arg->name);
        LLVMBuildStore(context->builder, arg_ref, alloc_ref);
        ht_set(context->function_bindings, arg->name, alloc_ref);

        arg = arg->next;
    }

    Binding* binding = function_decl->first_binding;
    while ( binding != NULL )
    {
        compileBinding(context, binding);
        binding = binding->next;
    }

}

LLVMTypeRef getFunctionType(FunctionDecl* function_decl)
{
    ALLOC_ARRAY(args, function_decl->arg_count, LLVMTypeRef);

    ArgDef* arg = function_decl->first_arg;
    for(int i=0;i<function_decl->arg_count;i++)
    {
        args[i] = expressionTypeToLLVMType(arg->type);
        arg = arg->next;
    }

    switch ( function_decl->output_type )
    {
        case FLOAT:
            return LLVMFunctionType(LLVMDoubleType(), args, function_decl->arg_count, 0);
        case CHAR:
            return LLVMFunctionType(LLVMInt8Type(), args, function_decl->arg_count, 0);
        case BOOL:
            return LLVMFunctionType(LLVMInt1Type(), args, function_decl->arg_count, 0);
        default:
            return LLVMFunctionType(LLVMInt64Type(), args, function_decl->arg_count, 0);
    }
}

LLVMTypeRef getBindingType(Binding* binding)
{
    //TODO: some places we store char* some places ExpressionType, unify and simplify
    if ( !strcmp(binding->decl_type, "bool") ) return LLVMInt1Type();
    if ( !strcmp(binding->decl_type, "float") ) return LLVMDoubleType();
    if ( !strcmp(binding->decl_type, "char") ) return LLVMInt8Type();

    return LLVMInt64Type();
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

//TODO: simplify this. can we add raw llvm ir?
void addPredefinedFunctions(Context* context)
{
    LLVMTypeRef input_types[] = { LLVMInt1Type() };

    LLVMTypeRef func_type = LLVMFunctionType(LLVMInt64Type(), input_types, 1, 0);
    LLVMValueRef main_func = LLVMAddFunction(context->module, "bool_to_int", func_type);

    /* LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry"); */
    /* LLVMPositionBuilderAtEnd(context->builder, entry); */
    /* LLVMValueRef arg = LLVMGetFirstParam(main_func); */
    /* LLVMTypeRef int1Type = LLVMInt1Type(); */
    /* LLVMValueRef true_val = LLVMConstInt(int1Type, 1, true); */

    /* LLVMTypeRef intType = LLVMInt64Type(); */
    /* LLVMValueRef one = LLVMConstInt(intType, 1, true); */
    /* LLVMValueRef zero = LLVMConstInt(intType, 0, true); */

    /* LLVMValueRef is_true =  LLVMBuildICmp(context->builder, LLVMIntEQ, arg, true_val, "is_true"); */
    /* LLVMValueRef r_value = LLVMBuildSelect(context->builder, is_true, one, zero, "bool_to_int"); */
    /* LLVMBuildRet(context->builder, r_value); */

    //Cast float to int
    LLVMTypeRef input_types2[] = { LLVMDoubleType() };
    func_type = LLVMFunctionType(LLVMInt64Type(), input_types2, 1, 0);
    main_func = LLVMAddFunction(context->module, "float_to_int", func_type);

    /* entry = LLVMAppendBasicBlock(main_func, "entry"); */
    /* LLVMPositionBuilderAtEnd(context->builder, entry); */
    /* arg = LLVMGetFirstParam(main_func); */
    /* r_value = LLVMBuildFPToSI(context->builder, arg, LLVMInt64Type(), "float_to_int"); */
    /* LLVMBuildRet(context->builder, r_value); */

    //cast char to int
    LLVMTypeRef input_types3[] = { LLVMInt8Type() };
    func_type = LLVMFunctionType(LLVMInt64Type(), input_types3, 1, 0);
    main_func = LLVMAddFunction(context->module, "char_to_int", func_type);

    /* entry = LLVMAppendBasicBlock(main_func, "entry"); */
    /* LLVMPositionBuilderAtEnd(context->builder, entry); */
    /* arg = LLVMGetFirstParam(main_func); */
    /* r_value = LLVMBuildZExt(context->builder, arg, LLVMInt64Type(), "char_to_int"); */
    /* LLVMBuildRet(context->builder, r_value); */


    //assert function
    LLVMTypeRef input_types4[] = { LLVMInt1Type() };
    func_type = LLVMFunctionType(LLVMInt64Type(), input_types4, 1, 0);
    main_func = LLVMAddFunction(context->module, "assert", func_type);

    /* entry = LLVMAppendBasicBlock(main_func, "entry"); */
    /* LLVMPositionBuilderAtEnd(context->builder, entry); */
    /* arg = LLVMGetFirstParam(main_func); */
    /* true_val = LLVMConstInt(int1Type, 1, true); */
    /* LLVMValueRef assert_result =  LLVMBuildICmp(context->builder, LLVMIntEQ, arg, true_val, "assert_result"); */
    /* LLVMBasicBlockRef  assert_success_block = LLVMAppendBasicBlock(main_func, "assert_successful"); */
    /* LLVMBasicBlockRef  assert_failed_block = LLVMAppendBasicBlock(main_func, "assert_failed"); */
    /* r_value = LLVMBuildCondBr(context->builder, assert_result, assert_success_block, assert_failed_block); */


    /* LLVMPositionBuilderAtEnd(context->builder, assert_success_block); */
    /* LLVMBuildRet(context->builder, one); */


    /* LLVMPositionBuilderAtEnd(context->builder, assert_failed_block); */
    /* LLVMBuildRet(context->builder, zero); */

    //abort function
    /* func_type = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0); */
    /* LLVMAddFunction(context->module, "abort", func_type); */


/* > FunctionType *AbortFTy = FunctionType::get(Type::getVoidTy(C), false); */
/* > Function *AbortF = Function::Create(AbortFTy, */
    /*     > GlobalValue::ExternalLinkage, "abort", M); */
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

