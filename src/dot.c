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
#define SAVE_POSITION long initial_position = ftell(file)
#define RESTORE_POSITION fseek(file, initial_position, SEEK_SET)


int parseExpression(FILE*, Expression*);

typedef struct
{
    char *input_file_path;
    char input_file_name[1024];
    FILE* input_file;
    int  debug_mode;

    char output_dir[1024];
    char output_file_path[1024];

} CompilationContext;

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

void ignoreWhitespace(FILE* file)
{
    char c = (char)fgetc(file);
    char temp[1024];

    while ( isspace(c) || c == '#' ) 
    {
        if ( c == EOF ) break;
        if ( c == '#' ) fgets(temp, 1024, file);

        c = (char)fgetc(file);
    }

    ungetc(c, file);
}

//Try to read any of given characters. Return index of matching character
int parseMultipleChoiceLiteral(FILE* file, const char* choices)
{
    ignoreWhitespace(file);

    SAVE_POSITION;
    char c = (char)fgetc(file);
    int choice_count = strlen(choices);

    for(int i=0;i<choice_count;i++)
    {
        if ( c == choices[i] ) return i;
    }

    RESTORE_POSITION;
    return FAIL;
}

int parseLiteral(FILE* file, const char* literal)
{
    ignoreWhitespace(file);

    SAVE_POSITION;
    for(size_t i=0;i<strlen(literal);i++)
    {
        char c = (char)fgetc(file);
        if ( c != literal[i] ) 
        {
            RESTORE_POSITION;
            return FAIL;
        }
    }

    return OK;
}

int parseNumber(FILE* file, char* token)
{
    ignoreWhitespace(file);
    SAVE_POSITION;

    int token_len = 0;

    char c = (char)fgetc(file);
    if ( isdigit(c) )
    {
        while ( c != EOF && isdigit(c) ) 
        {
            token[token_len++] = c;
            c = (char)fgetc(file);
        }
        ungetc(c, file);
    }
    else
    {
        RESTORE_POSITION;
        return FAIL;
    }

    token[token_len] = 0;
    return OK;
}

int parseIdentifier(CompilationContext* context, FILE* file, char* token)
{
    ignoreWhitespace(file);

    SAVE_POSITION;
    char c = (char)fgetc(file);
    int token_len = 0;

    if ( isalpha(c) )
    {
        while ( c != EOF && isalpha(c) )
        {
            token[token_len++] = c;
            c = (char)fgetc(file);
        }
        ungetc(c, file);
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

int parseMathExpression(FILE* file, MathExpression* exp)
{
    exp->lhs = ALLOC(Expression);

    int result = parseExpression(file, exp->lhs);
    if ( result == FAIL ) return FAIL;

    const char* operators="+-*/";

    result = parseMultipleChoiceLiteral(file, operators);
    if ( result == FAIL ) return FAIL;

    exp->op = operators[result];
    
    exp->rhs = ALLOC(Expression);
    result = parseExpression(file, exp->rhs);
    if ( result == FAIL ) return FAIL;

    return OK;
}

int parseExpression(FILE* file, Expression* exp)
{
    int result = parseLiteral(file, "(");
    if ( result != FAIL )
    {
        result = parseExpression(file, exp);
        if ( result == FAIL ) return FAIL;

        result = parseLiteral(file, ")");
        if ( result == FAIL ) return FAIL;

        return OK;
    }

    exp->math_expression = ALLOC(MathExpression);
    result = parseMathExpression(file, exp->math_expression);

    if ( result == OK ) return OK;
    exp->math_expression = NULL;

    char num1[16];
    result = parseNumber(file, num1);
    if ( result != FAIL )
    {
        exp->number = atoi(num1);
        return OK;
    }


    return OK;
}

int parseBinding(CompilationContext* context, FILE* file, StaticBinding* b)
{
    char token[256];
    int result = parseIdentifier(context, file, token);
    if ( result == FAIL ) return FAIL;
    strcpy(b->lhs, token);

    result = parseLiteral(file, ":=");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "()");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "->");
    if ( result == FAIL ) return FAIL;

    b->function_decl = ALLOC(FunctionDecl);
    b->function_decl->expression = ALLOC(Expression);

    result = parseExpression(file, b->function_decl->expression);
    if ( result == FAIL ) return FAIL;

    return OK;
}

int parseModule(CompilationContext* context, FILE* file, Module* module)
{
    StaticBinding* binding = ALLOC(StaticBinding);
    _ModuleItem* item = ALLOC(_ModuleItem);
    item->static_binding = binding;

    module->items_head = module->items_tail = item;
    int result = parseBinding(context, file, binding);

    return result;
}

LLVMValueRef generateExpression(CompilationContext* context, Expression* exp, LLVMBuilderRef builder)
{

    if ( exp->math_expression == NULL )
    {
        debugLog(context, "Generating code for simple exp: %d", exp->number);
        LLVMTypeRef intType = LLVMIntType(32);
        return LLVMConstInt(intType, exp->number, true);
    }
    else
    {
        debugLog(context, "Generating code for math exp");

        LLVMValueRef val1 = generateExpression(context, exp->math_expression->lhs, builder);
        LLVMValueRef val2 = generateExpression(context, exp->math_expression->rhs, builder);

        char op = exp->math_expression->op;

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

    LLVMBuildRet(builder, generateExpression(context, m->items_head->static_binding->function_decl->expression, builder));

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
    int result = parseModule(&context, context.input_file, &module);
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

