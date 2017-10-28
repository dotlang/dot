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
#define FAIL 0

typedef struct
{
    char name[256];
    int  number;

} Binding;

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


int parseLiteral(FILE* file, const char* literal)
{
    ignoreWhitespace(file);
    for(size_t i=0;i<strlen(literal);i++)
    {
        char c = (char)fgetc(file);
        if ( c != literal[i] ) return FAIL;
    }

    return OK;
}

int parseNumber(FILE* file, char* token)
{
    ignoreWhitespace(file);
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

    token[token_len] = 0;
    return token_len;
}

int parseIdentifier(FILE* file, char* token)
{
    ignoreWhitespace(file);

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

    token[token_len] = 0;
    return token_len;
}


int parseBinding(FILE* file, Binding* b)
{
    char token[256];
    int result = parseIdentifier(file, token);
    if ( result == FAIL ) return FAIL;
    printf("Token: %s\n", token);
    strcpy(b->name, token);

    result = parseLiteral(file, ":=");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "()");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "->");
    if ( result == FAIL ) return FAIL;

    result = parseNumber(file, token);
    if ( result == FAIL ) return FAIL;
    printf("Number: %s\n", token);
    b->number = atoi(token);

    return OK;
}

int parseModule(FILE* file, Binding* b)
{
    int result = parseBinding(file, b);

    return result;
}

void generate(Binding* b, char* output_file)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("test");
    LLVMSetDataLayout(module, "");
    LLVMSetTarget(module, LLVMGetDefaultTargetTriple());

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef mainfunc = LLVMAddFunction(module, "main", funcType);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMTypeRef intType = LLVMIntType(32);
    LLVMValueRef val = LLVMConstInt(intType, b->number, true);
    LLVMBuildRet(builder, val);

    char *error = NULL;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMPrintModuleToFile(module, output_file, &error);

    LLVMDisposeMessage(error);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
}

int main(int argc, char** argv)
{
    Binding b;

    FILE *input_file = fopen(argv[1], "r");

    int result = parseModule(input_file, &b);
    printf("parse result is: %s\n", (result == FAIL)?"FAIL":"OK");
    fclose(input_file);


    //generate LLVM intermediate representation of the source code file
    char temp_dir[256];
    strcpy(temp_dir, "/tmp/dot_temp_XXXXXX");
    mkdtemp(temp_dir);
    printf("temp dir %s created\n", temp_dir);

    char base_filename[1024];
    char* input_filename = basename(argv[1]);
    char* dot_place = strstr(input_filename, ".");
    int base_len = dot_place - input_filename;
    strncpy(base_filename, input_filename, base_len);
    base_filename[base_len] = 0;
    printf("base filename = %s\n", base_filename);

    char temp_filename[256];
    sprintf(temp_filename, "%s/%s.ll", temp_dir, base_filename);
    printf("intermediate ll stored at %s\n", temp_filename);
    generate(&b, temp_filename);


    //compile llvm output to object file
    char clang_command[1024];
    sprintf(clang_command, "clang -x ir -o %s %s", base_filename, temp_filename);
    printf("running %s\n", clang_command);
    system(clang_command);

    char cleanup_cmd[1024];
    sprintf(cleanup_cmd, "rm -rf %s", temp_dir);
    system(cleanup_cmd);
}
