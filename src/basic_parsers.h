#ifndef __BASIC_PARSERS_H__
#define __BASIC_PARSERS_H__

#include "ast.h"
#include "debug_helpers.h"

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

//Try to read any of given characters. Return index of matching character
int parseMultipleChoiceLiteral(Context* context, const char* choices)
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

int parseLiteral(Context* context, const char* literal)
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

int parseNumber(Context* context, char* token)
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

int parseIdentifier(Context* context, char* token)
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

#endif
