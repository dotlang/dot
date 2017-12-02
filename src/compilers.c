#include "compilers.h"

void compileBinding(Context* context, hashtable_t*, Binding* binding);

void compileCodeBlock(Context* context, CodeBlock* code_block)
{
    struct CodeBlockElement* element = code_block->first_element;

    while ( element != NULL )
    {
        if ( element->return_expression != NULL )
        {
            LLVMValueRef return_exp = compileExpression(context, code_block->last_element->return_expression);
            LLVMBuildRet(context->builder, return_exp);
        }
        else
        {
            compileBinding(context, context->function_bindings, element->binding);
        }

        element = element->next;
    }
}

void compileFunctionDecl(Context* context, FunctionDecl* function_decl)
{
    if ( function_decl->expression != NULL )
    {
        LLVMValueRef return_exp = compileExpression(context, function_decl->expression);
        LLVMBuildRet(context->builder, return_exp);
    }
    else
    {
        compileCodeBlock(context, function_decl->code_block);
    }
}

void compileBinding(Context* context, hashtable_t* storage, Binding* binding)
{
    if ( binding->function_decl != NULL )
    {
        LLVMValueRef mainfunc = LLVMGetNamedFunction(context->module, binding->lhs);
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
        LLVMPositionBuilderAtEnd(context->builder, entry);

        compileFunctionDecl(context, binding->function_decl);
    }
    else
    {
        LLVMValueRef r_value = compileExpression(context, binding->expression);
        LLVMTypeRef int_type = LLVMIntType(32);
        LLVMValueRef alloc_ref = LLVMBuildAlloca(context->builder, int_type, binding->lhs);
        LLVMBuildStore(context->builder, r_value, alloc_ref);

        ht_set(storage, binding->lhs, alloc_ref);
    }
}

void declareFunctionBinding(Context* context, Binding* binding)
{
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMAddFunction(context->module, binding->lhs, funcType);
}

void compileModule(Context* context, Module* m)
{
    context->module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(context->module, "");
    LLVMSetTarget(context->module, LLVMGetDefaultTargetTriple());
    context->builder = LLVMCreateBuilder();

    struct ModuleElement* element = m->first_element;
    while ( element != NULL && element->binding->function_decl != NULL )
    {
        declareFunctionBinding(context, element->binding);
        element = element->next;
    }

    element = m->first_element;
    while ( element != NULL )
    {
        //for module level bindings we dont need a storage as we can lookup functions using LLVM
        compileBinding(context, NULL, element->binding);
        element = element->next;
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

