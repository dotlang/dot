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


int parseLiteral(FILE* file, char* literal)
{
    ignoreWhitespace(file);
    for(int i=0;i<strlen(literal);i++)
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

void generate(Binding* b, FILE* out)
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
    bool isInvalid = LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    char* outData = LLVMPrintModuleToString(module);
    fputs(outData, out);
    LLVMDisposeMessage(outData);

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
}

int main(int argc, char** argv)
{
    FILE *file;
    Binding b;

    file = fopen(argv[1], "r");

    char* input_filename = basename(argv[1]);
    char* dot_place = strstr(input_filename, ".");

    char base_filename[1024];
    int base_len = dot_place - input_filename;
    strncpy(base_filename, input_filename, base_len);
    base_filename[base_len] = 0;
    printf("%s", base_filename);



    int result = parseModule(file, &b);
    printf("result is: %s\n", (result == FAIL)?"FAIL":"OK");
    fclose(file);

    char temp_file[256];
    sprintf(temp_file, "/tmp/%s.ll", base_filename);
    FILE* llvm_output = fopen(temp_file, "w");
    if ( !llvm_output )
    {
        printf("Cannot open mkstemp\n");
        return -1;
    }

    //generate LLVM intermediate representation of the source code file
    generate(&b, llvm_output);
    fclose(llvm_output);


    //compile llvm output to object file
    char clang_command[1024];
    sprintf(clang_command, "clang -x ir -o ./build/%s %s", base_filename, temp_file);
    system(clang_command);
}
