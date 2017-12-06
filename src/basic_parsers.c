#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#include "ast.h"
#include "basic_parsers.h"
#include "debug_helpers.h"

#define SAVE_POSITION long initial_position = ftell(context->input_file)
#define RESTORE_POSITION fseek(context->input_file, initial_position, SEEK_SET)

char getChar(Context* context)
{
    char c= (char)fgetc(context->input_file);

    return c;
}

void undoChar(Context* context, char c)
{
    ungetc(c, context->input_file);
}

bool newLineAhead(Context* context)
{
    while ( 1 ) 
    {
        char c = getChar(context);

        if ( c == '#' )
        {
            char temp[1024];
            fgets(temp, 1024, context->input_file);
            return true;
        }
        else if ( isspace(c) )
        {
            if ( c == '\n' ) return true;
        }
        else
        {
            undoChar(context, c);
            return false;
        }
    }
}

TokenKind getTokenKind(char* token)
{
    if ( !strcmp(token, "(") ) return OPEN_PAREN;
    if ( !strcmp(token, ")") ) return CLOSE_PAREN;
    if ( !strcmp(token, "+") ) return OP_ADD;
    if ( !strcmp(token, "-") ) return OP_SUB;
    if ( !strcmp(token, "*") ) return OP_MUL;
    if ( !strcmp(token, "/") ) return OP_DIV;
    if ( !strcmp(token, "%") ) return OP_REM;
    if ( !strcmp(token, "%%") ) return OP_DVT;
    if ( !strcmp(token, "{") ) return OPEN_BRACE;
    if ( !strcmp(token, "}") ) return CLOSE_BRACE;
    if ( !strcmp(token, ",") ) return COMMA;

    if ( isdigit(token[0]) ) return INT_LITERAL;

    return IDENTIFIER;
}

int getOperatorPrecedence(TokenKind kind)
{
    switch ( kind )
    {
        case OP_ADD: return 2;
        case OP_SUB: return 2;
        case OP_MUL: return 3;
        case OP_DIV: return 3;
        case OP_REM: return 3;
        case OP_DVT: return 3;
        default: abort();
    }

    abort();
}
            
bool isLeftAssociative(TokenKind kind)
{
    return true;
}

//TODO: make these efficient by caching next token in peek token instead of changing file ptr
void peekNextToken(Context* context, char* token)
{
    SAVE_POSITION;

    getNextToken(context, token);

    RESTORE_POSITION;
}

void getNextToken(Context* context, char* token)
{
    while ( 1 ) 
    {
        char c = getChar(context);

        //check for whitespace and EOF
        if ( c == EOF ) return;
        if ( isspace(c) ) continue;

        //ignore comments
        if ( c == '#' ) 
        {
            char temp[1024];
            fgets(temp, 1024, context->input_file);
            continue;
        }

        //check for one-two character tokens
        if ( c == '%' )
        {
            c = getChar(context);
            if ( c == '%' )
            {
                token[0] = token[1] = '%';
                token[2]=0;
                return;
            }
            undoChar(context, c);
            token[0] = '%';
            token[1] = 0;
            return;
        }

        if ( c == ':' )
        {
            c = getChar(context);
            if ( c == ':' || c == '=' )
            {
                token[0] = ':';
                token[1] = c;
                token[2] = 0;
                return;
            }
            undoChar(context, c);
            token[0] = ':';
            token[1] = 0;
            return;
        }

        if ( c == '-' )
        {
            c = getChar(context);
            if ( c == '>' )
            {
                token[0] = '-';
                token[1] = '>';
                token[2] = 0;
                return;
            }
            undoChar(context, c);
            token[0] = '-';
            token[1] = 0;
            return;
        }
        
        //check for single character tokens
        if ( strchr("+-*/{}[]().,", c) != NULL )
        {
            token[0] = c;
            token[1] = 0;
            return;
        }

        //check for number literals
        if ( isdigit(c) )
        {
            int len = 0;
            while ( isdigit(c) )
            {
                token[len++] = c;
                c = getChar(context);
            }
            undoChar(context, c);
            token[len] = 0;
            return;
        }

        if ( isalpha(c) )
        {
            int len = 0;
            while ( c != EOF && isalnum(c) )
            {
                token[len++] = c;
                c = getChar(context);
            }
            undoChar(context, c);
            token[len] = 0;

            return;
        }
    }
}
