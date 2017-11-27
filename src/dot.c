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

#define OK   1
#define FAIL -1

#define ALLOC(T)  (T*)calloc(1, sizeof(T))
#define SAVE_POSITION long initial_position = ftell(context->input_file)
#define RESTORE_POSITION fseek(context->input_file, initial_position, SEEK_SET)



typedef struct
{
    char *input_file_path;
    char input_file_name[1024];
    FILE* input_file;
    int  debug_mode;

    char output_dir[1024];
    char output_file_path[1024];

} CompilationContext;

int parseExpression(CompilationContext*, Expression*);

void debugLog(CompilationContext* context, const char* format, ...)
{
    if ( context->debug_mode == 0 ) return;
	char result[1024];

    /* Declare a va_list type variable */
    va_list myargs;

    /* Initialise the va_list variable with the ... after fmt */
    va_start(myargs, format);

    /* Forward the '...' to vprintf */
    vsprintf(result, format, myargs);

    /* Clean up the va_list */
    va_end(myargs);

	printf("%s\n", result);
}

void ignoreWhitespace(CompilationContext* context)
{
    char c = (char)fgetc(context->input_file);
    char temp[1024];

    while ( isspace(c) || c == '#' ) 
    {
        if ( c == EOF ) break;
        if ( c == '#' ) fgets(temp, 1024, context->input_file);

        c = (char)fgetc(context->input_file);
    }

    ungetc(c, context->input_file);
}

//Try to read any of given characters. Return index of matching character
int parseMultipleChoiceLiteral(CompilationContext* context, const char* choices)
{
    ignoreWhitespace(context);

    SAVE_POSITION;
    char c = (char)fgetc(context->input_file);
    int choice_count = strlen(choices);

    for(int i=0;i<choice_count;i++)
    {
        if ( c == choices[i] ) return i;
    }

    RESTORE_POSITION;
    return FAIL;
}

int parseLiteral(CompilationContext* context, const char* literal)
{
    ignoreWhitespace(context);

    SAVE_POSITION;
    for(size_t i=0;i<strlen(literal);i++)
    {
        char c = (char)fgetc(context->input_file);
        if ( c != literal[i] ) 
        {
            RESTORE_POSITION;
            return FAIL;
        }
    }

    return OK;
}

int parseNumber(CompilationContext* context, char* token)
{
    ignoreWhitespace(context);
    SAVE_POSITION;

    int token_len = 0;

    char c = (char)fgetc(context->input_file);
    if ( isdigit(c) )
    {
        while ( c != EOF && isdigit(c) ) 
        {
            token[token_len++] = c;
            c = (char)fgetc(context->input_file);
        }
        ungetc(c, context->input_file);
    }
    else
    {
        RESTORE_POSITION;
        return FAIL;
    }

    token[token_len] = 0;
    return OK;
}

int parseIdentifier(CompilationContext* context, char* token)
{
    ignoreWhitespace(context);

    SAVE_POSITION;
    char c = (char)fgetc(context->input_file);
    int token_len = 0;

    if ( isalpha(c) )
    {
        while ( c != EOF && isalpha(c) )
        {
            token[token_len++] = c;
            c = (char)fgetc(context->input_file);
        }
        ungetc(c, context->input_file);
    }
    else
    {
        RESTORE_POSITION;
        return FAIL;
    }

    token[token_len] = 0;
    debugLog(context, "Parsed an identifier: %s", token);
        
    return OK;
}

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

