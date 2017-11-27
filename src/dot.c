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
#include "basic_parsers.h"
#include "debug_helpers.h"

int parseExpression(CompilationContext*, Expression*);

//MathFactor         = "(" Expression ")" | NUMBER
int parseMathFactor(CompilationContext* context, MathFactor* factor)
{
    int result = parseLiteral(context, "(");
    if ( result != FAIL )
    {
        factor->expression = ALLOC(Expression);
        result = parseExpression(context, factor->expression);
        if ( result == FAIL ) return FAIL;

        result = parseLiteral(context, ")");
        if ( result == FAIL ) return FAIL;

        return OK;
    }

    char num1[16];
    result = parseNumber(context, num1);
    if ( result != FAIL )
    {
        factor->number = atoi(num1);
        return OK;
    }

    return OK;
}

//MathExpression     = MathFactor ("+"|"-"|"*"|"/"|"%"|"%%") MathExpression | MathFactor
int parseMathExpression(CompilationContext* context, MathExpression* exp)
{
    exp->lhs = ALLOC(MathFactor);

    int result = parseMathFactor(context, exp->lhs);
    if ( result == FAIL ) return FAIL;

    const char* ops="+-*/";

    result = parseMultipleChoiceLiteral(context, ops);
    if ( result == FAIL ) {
        exp->op = 0;
        return OK;
    }

    exp->op = ops [result];
    
    exp->rhs = ALLOC(MathExpression);
    result = parseMathExpression(context, exp->rhs);
    if ( result == FAIL ) return FAIL;

    return OK;
}

//Expression         = BINDING_NAME | FunctionDecl | FnCall | ExpressionLiteral | StructAccess |
//                     OperatorExpression | MathExpression | SequenceMapReadOp | BoolExpression
int parseExpression(CompilationContext* context, Expression* exp)
{
    exp->math_expression = ALLOC(MathExpression);
    int result = parseMathExpression(context, exp->math_expression);

    if ( result == FAIL ) return FAIL;

    return OK;
}

int parseBinding(CompilationContext* context, StaticBinding* b)
{
    char token[256];
    int result = parseIdentifier(context, token);
    if ( result == FAIL ) return FAIL;
    strcpy(b->lhs, token);

    result = parseLiteral(context, ":=");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(context, "()");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(context, "->");
    if ( result == FAIL ) return FAIL;

    b->function_decl = ALLOC(FunctionDecl);
    b->function_decl->expression = ALLOC(Expression);

    result = parseExpression(context, b->function_decl->expression);
    if ( result == FAIL ) return FAIL;

    return OK;
}

int parseModule(CompilationContext* context, Module* module)
{
    StaticBinding* binding = ALLOC(StaticBinding);
    ModuleItem* item = ALLOC(ModuleItem);
    item->static_binding = binding;

    module->items_head = module->items_tail = item;
    int result = parseBinding(context, binding);

    return result;
}

LLVMValueRef generateMathExpression(CompilationContext* context, MathExpression* exp, LLVMBuilderRef builder);

LLVMValueRef generateMathFactor(CompilationContext* context, MathFactor* factor, LLVMBuilderRef builder)
{
    if ( factor->expression != NULL ) return generateMathExpression(context, factor->expression->math_expression, builder);

    debugLog(context, "Generating code for simple exp: %d", factor->number);
    LLVMTypeRef intType = LLVMIntType(32);
    return LLVMConstInt(intType, factor->number, true);
}

LLVMValueRef generateMathExpression(CompilationContext* context, MathExpression* exp, LLVMBuilderRef builder)
{
    debugLog(context, "Generating code for math exp");

    LLVMValueRef val1 = generateMathFactor(context, exp->lhs, builder);

    char op = exp->op;

    if ( op == 0 ) 
    {
        return val1;
    }

    LLVMValueRef val2 = generateMathExpression(context, exp->rhs, builder);

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

void generate(CompilationContext* context, Module* m, char* output_file)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(module, "");
    LLVMSetTarget(module, LLVMGetDefaultTargetTriple());

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef mainfunc = LLVMAddFunction(module, "main", funcType);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMBuildRet(builder, generateMathExpression(context, m->items_head->static_binding->function_decl->expression->math_expression, builder));

    char *error = NULL;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMPrintModuleToFile(module, output_file, &error);

    LLVMDisposeMessage(error);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
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

    generate(&context, &module, context.output_file_path);

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
    system(cleanup_cmd);
    debugLog(&context, "cleanup finished.");
}

