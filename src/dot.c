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

#define OK   1
#define FAIL -1

typedef struct
{
    char *input_file_path;
    char input_file_name[1024];
    FILE* input_file;
    int  debug_mode;

    char output_dir[1024];
    char output_file_path[1024];

} CompilationContext;


typedef struct MathExpression
{
    //+, -, * or / or 0 when expression is a single number
    //if op is 0, then we have a single number
    char op;
    union
    {
        struct
        {
            struct MathExpression* lhs;
            struct MathExpression* rhs;
        };
        int number;
    };
} MathExpression;

typedef struct
{
    char name[256];
    MathExpression exp;

} Binding;

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

#define SAVE_POSITION long initial_position = ftell(file)
#define RESTORE_POSITION fseek(file, initial_position, SEEK_SET)

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

int parseIdentifier(FILE* file, char* token)
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
    return OK;
}

int parseMathExpression(FILE* file, MathExpression* exp)
{
    char num1[16];
    int result = parseNumber(file, num1);
    if ( result == FAIL ) 
    {
        result = parseLiteral(file, "(");
        if ( result != FAIL )
        {
            result = parseMathExpression(file, exp);
            if ( result == FAIL ) return FAIL;
        }
        result = parseLiteral(file, ")");
        if ( result == FAIL ) return FAIL;

        return OK;
    }

    const char* operators="+-*/";

    result = parseMultipleChoiceLiteral(file, operators);
    if ( result == FAIL ) 
    {
        exp->op = 0;
        exp->number = atoi(num1);
        return OK;
    }

    exp->lhs = malloc(sizeof(MathExpression));
    exp->lhs->op = 0;
    exp->lhs->number = atoi(num1);

    exp->op = operators[result];
    
    exp->rhs = malloc(sizeof(MathExpression));
    result = parseMathExpression(file, exp->rhs);
    if ( result == FAIL ) return FAIL;

    return OK;
}

int parseBinding(FILE* file, Binding* b)
{
    char token[256];
    int result = parseIdentifier(file, token);
    if ( result == FAIL ) return FAIL;
    strcpy(b->name, token);

    result = parseLiteral(file, ":=");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "()");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "->");
    if ( result == FAIL ) return FAIL;

    MathExpression me;
    result = parseMathExpression(file, &me);
    if ( result == FAIL ) return FAIL;
    b->exp = me;

    return OK;
}

int parseModule(FILE* file, Binding* b)
{
    int result = parseBinding(file, b);

    return result;
}

LLVMValueRef generateMathExpression(CompilationContext* context, MathExpression* exp, LLVMBuilderRef builder)
{
    debugLog(context, "Generating code for math exp of type %c", exp->op);

    if ( exp->op == 0 )
    {
        LLVMTypeRef intType = LLVMIntType(32);
        return LLVMConstInt(intType, exp->number, true);
    }
    else
    {
        LLVMValueRef val1 = generateMathExpression(context, exp->lhs, builder);
        LLVMValueRef val2 = generateMathExpression(context, exp->rhs, builder);

        if ( exp->op == '+' )
        {
            return LLVMBuildAdd(builder, val1, val2, "temp");
        }
        else if ( exp->op == '-' )
        {
            return LLVMBuildSub(builder, val1, val2, "temp");
        }
        else if ( exp->op == '*' )
        {
            return LLVMBuildMul(builder, val1, val2, "temp");
        }
        else if ( exp->op == '/' )
        {
            return LLVMBuildSDiv(builder, val1, val2, "temp");
        }
    }

    abort();
}

void generate(CompilationContext* context, Binding* b, char* output_file)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(module, "");
    LLVMSetTarget(module, LLVMGetDefaultTargetTriple());

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef mainfunc = LLVMAddFunction(module, "main", funcType);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMBuildRet(builder, generateMathExpression(context, &b->exp, builder));

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
        printf("Usage: dot <input_file> [-d]");
        return 0;
    }


	CompilationContext context;
    context.debug_mode = 0;

    for(int i=1;i<argc;i++)
    {
        char* arg = argv[i];
        if ( strcmp(arg, "-d") == 0 ) 
        {
            context.debug_mode = 1;
            debugLog(&context, "debug mode enabled\n");
        }
        else
        {
            context.input_file_path = argv[i];
        }
    }

    context.input_file = fopen(context.input_file_path, "r");

    Binding b;
    int result = parseModule(context.input_file, &b);
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
    debugLog(&context, "Will store intermediate code at %s", context.output_file_path);
    generate(&context, &b, context.output_file_path);
    debugLog(&context, "Code generation finished.");

    //compile llvm output to object file
    debugLog(&context, "Compiling to native executable...");
    char clang_command[1024];
    sprintf(clang_command, "clang -x ir -o %s %s", base_filename, context.output_file_path);
    debugLog(&context, "Compilation command: %s", clang_command);
    system(clang_command);
    debugLog(&context, "Compilation finished.");

    debugLog(&context, "Cleaning up temp files...");
    char cleanup_cmd[1024];
    sprintf(cleanup_cmd, "rm -rf %s", context.output_dir);
    system(cleanup_cmd);
    debugLog(&context, "Cleanup finished.");
}

