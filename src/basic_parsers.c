#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <stdbool.h>

#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"

#define SAVE_POSITION long initial_position = ftell(context->input_file)
#define RESTORE_POSITION fseek(context->input_file, initial_position, SEEK_SET)

void ignoreWhitespace(Context* context)
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

//Try to read any of given characters. Return index of matching choice
const char* matchLiterals(Context* context, const char** choices, const int choice_count)
{
    for(int i=0;i<choice_count;i++)
    {
        if ( matchLiteral(context, choices[i]) ) return choices[i];
    }

    return NULL;
}

bool matchLiteral(Context* context, const char* literal)
{
    ignoreWhitespace(context);

    SAVE_POSITION;
    for(size_t i=0;i<strlen(literal);i++)
    {
        char c = (char)fgetc(context->input_file);
        if ( c != literal[i] ) 
        {
            RESTORE_POSITION;
            return false;
        }
    }

    return true;
}

bool matchNumber(Context* context, char* token)
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
        return false;
    }

    token[token_len] = 0;
    return true;
}

int parseIdentifier(Context* context, char* token)
{
    ignoreWhitespace(context);

    SAVE_POSITION;
    char c = (char)fgetc(context->input_file);
    int token_len = 0;

    if ( isalpha(c) )
    {
        while ( c != EOF && isalnum(c) )
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

int strToOp(const char* str)
{
    if ( !strcmp(str, "+") ) return OP_ADD;
    if ( !strcmp(str, "-") ) return OP_SUB;
    if ( !strcmp(str, "*") ) return OP_MUL;
    if ( !strcmp(str, "/") ) return OP_DIV;
    if ( !strcmp(str, "%") ) return OP_REM;
    if ( !strcmp(str, "%%") ) return OP_DVT;

    return OP_NOP;
}

const char* opToStr(int op)
{
    switch ( op )
    {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_REM: return "%";
        case OP_DVT: return "%%";
    }

    return "N/A";

}
