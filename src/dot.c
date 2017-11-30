#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <stdbool.h>

#include "ast.h"
#include "debug_helpers.h"
#include "parsers.h"
#include "compilers.h"

void printUsage()
{
    printf("dotLang compiler v0.1.0 - Copyright Mahdi Mohammadinasab\n");
    printf("Usage: dot <input_file>\n");
    printf("If DOT_VERBOSE_LOG is set to one, verbose output will be shown\n");
}

void checkDebugMode(Context* context)
{
    const char* s = getenv("DOT_VERBOSE_LOG");

    if ( s != NULL && strcmp(s, "1") == 0 ) 
    {
        context->debug_mode = 1;
    }

    context->debug_mode = 0;
}

int openInputFile(Context* context, char* arg)
{
    context->input_file_path = arg;
    debugLog(context, "compiling %s...", context->input_file_path);

    context->input_file = fopen(context->input_file_path, "r");

    if ( context->input_file == NULL )
    {
        printf("Could not open input file: %s\n", context->input_file_path);
        return FAIL;
    }

    return OK;
}

void prepareOutputLocation(Context* context)
{
    //create temp dir
    strcpy(context->llvmir_dir, "/tmp/dot_temp_XXXXXX");
    mkdtemp(context->llvmir_dir);
    debugLog(context, "temp dir %s created", context->llvmir_dir);

    //create parse base file name
    char* input_filename = basename(context->input_file_path);
    char* dot_place = strstr(input_filename, ".");
    int base_len = dot_place - input_filename;
    strncpy(context->output_file_path, input_filename, base_len);
    context->output_file_path[base_len] = 0;
    debugLog(context, "base filename = %s", context->output_file_path);

    //set output path
    sprintf(context->llvmir_file_path, "%s/%s.ll", context->llvmir_dir, context->output_file_path);
    debugLog(context, "will store intermediate code at %s", context->llvmir_file_path);
}

void generateExecutable(Context* context)
{
    //compile llvm output to object file
    debugLog(context, "compiling to native executable...");
    char clang_command[1024];
    sprintf(clang_command, "clang -x ir -o %s %s", context->output_file_path, context->llvmir_file_path);
    debugLog(context, "compilation command: %s", clang_command);
    system(clang_command);
    debugLog(context, "compilation finished.");
}

void cleanupTemps(Context* context)
{
    debugLog(context, "cleaning up temp files...");
    char cleanup_cmd[1024];
    sprintf(cleanup_cmd, "rm -rf %s", context->llvmir_dir);
    /* system(cleanup_cmd); */
    debugLog(context, "cleanup finished.");
}

int main(int argc, char** argv)
{
    if ( argc == 0 )
    {
        printUsage();
        return 1;
    }

	ALLOC(context, Context);
    checkDebugMode(context);
    
    int error_code = openInputFile(context, argv[1]);
    if ( error_code == FAIL ) return 1;

    Module* module = parseModule(context);
    fclose(context->input_file);

    prepareOutputLocation(context);

    compileModule(context, module);
    disposeLlvm(context);

    debugLog(context, "llvm compilation finished.");

    generateExecutable(context);

    cleanupTemps(context);
}

