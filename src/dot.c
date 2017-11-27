#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <stdbool.h>

#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/TargetMachine.h"

#include "ast.h"
#include "debug_helpers.h"
#include "parsers.h"

LLVMValueRef generateMathExpression(CompilationContext* context, MathExpression* exp, LLVMBuilderRef builder);

LLVMValueRef generateMathFactor(CompilationContext* context, MathFactor* factor, LLVMBuilderRef builder)
{
    if ( factor->expression != NULL ) return generateMathExpression(context, factor->expression->math_expression, builder);

    LLVMTypeRef intType = LLVMIntType(32);
    return LLVMConstInt(intType, factor->number, true);
}

LLVMValueRef generateMathExpression(CompilationContext* context, MathExpression* exp, LLVMBuilderRef builder)
{
    LLVMValueRef val1 = generateMathFactor(context, exp->factor, builder);

    char op = exp->op;

    if ( op == 0 ) 
    {
        return val1;
    }

    LLVMValueRef val2 = generateMathExpression(context, exp->expression, builder);

    if ( op == '+' )
    {
        return LLVMBuildAdd(builder, val1, val2, "temp");
    }
    else if ( op == '-' )
    {
        return LLVMBuildSub(builder, val1, val2, "temp");
    }
    else if ( op == '*' )
    {
        return LLVMBuildMul(builder, val1, val2, "temp");
    }
    else if ( op == '/' )
    {
        return LLVMBuildSDiv(builder, val1, val2, "temp");
    }

    abort();
}

void generateFunction(CompilationContext* context, FunctionDecl* function_decl)
{
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef mainfunc = LLVMAddFunction(context->module, "main", funcType);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
    LLVMPositionBuilderAtEnd(context->builder, entry);

    LLVMBuildRet(context->builder, generateMathExpression(context, function_decl->expression->math_expression, context->builder));
}

void generateModule(CompilationContext* context, Module* m)
{
    context->module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(context->module, "");
    LLVMSetTarget(context->module, LLVMGetDefaultTargetTriple());
    context->builder = LLVMCreateBuilder();

    generateFunction(context, m->items_head->static_binding->function_decl);
}

void compileModule(CompilationContext* context)
{
    char* output_file = context->output_file_path;

    char *error = NULL;
    LLVMVerifyModule(context->module, LLVMAbortProcessAction, &error);
    LLVMPrintModuleToFile(context->module, output_file, &error);

    LLVMDisposeMessage(error);
    LLVMDisposeBuilder(context->builder);
    LLVMDisposeModule(context->module);
}

int main(int argc, char** argv)
{
    if ( argc == 0 )
    {
        printf("Usage: dot <input_file>");
        printf("If DOT_VERBOSE_COMPILE is set to one, verbose output will be shown");
        return 0;
    }

	CompilationContext context;
    context.debug_mode = 0;

    const char* s = getenv("DOT_VERBOSE_COMPILE");
    if ( s != NULL && strcmp(s, "1") == 0 ) 
    {
        context.debug_mode = 1;
        debugLog(&context, "debug mode enabled");
    }

    context.input_file_path = argv[1];
    debugLog(&context, "compiling %s...", context.input_file_path);

    context.input_file = fopen(context.input_file_path, "r");

    Module module;
    int result = parseModule(&context, &module);
    fclose(context.input_file);
    debugLog(&context, "parse result is: %s", (result == FAIL)?"FAIL":"OK");

    //generate LLVM intermediate representation of the source code file
    strcpy(context.output_dir, "/tmp/dot_temp_XXXXXX");
    mkdtemp(context.output_dir);
    debugLog(&context, "temp dir %s created", context.output_dir);

    char base_filename[1024];
    char* input_filename = basename(context.input_file_path);
    char* dot_place = strstr(input_filename, ".");
    int base_len = dot_place - input_filename;
    strncpy(base_filename, input_filename, base_len);
    base_filename[base_len] = 0;
    debugLog(&context, "base filename = %s", base_filename);


    sprintf(context.output_file_path, "%s/%s.ll", context.output_dir, base_filename);
    debugLog(&context, "will store intermediate code at %s", context.output_file_path);

    generateModule(&context, &module);
    compileModule(&context);

    debugLog(&context, "code generation finished.");

    //compile llvm output to object file
    debugLog(&context, "compiling to native executable...");
    char clang_command[1024];
    sprintf(clang_command, "clang -x ir -o %s %s", base_filename, context.output_file_path);
    debugLog(&context, "compilation command: %s", clang_command);
    system(clang_command);
    debugLog(&context, "compilation finished.");

    debugLog(&context, "cleaning up temp files...");
    char cleanup_cmd[1024];
    sprintf(cleanup_cmd, "rm -rf %s", context.output_dir);
    /* system(cleanup_cmd); */
    debugLog(&context, "cleanup finished.");
}

