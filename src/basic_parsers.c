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

bool isBinaryOp(TokenKind kind)
{
    return (kind == OP_ADD) || (kind == OP_SUB) || (kind == OP_MUL) ||
        (kind == OP_DIV) || (kind == OP_REM) || (kind == OP_DVT) || (kind == OP_SHR) ||
        (kind == OP_SHL);
}

TokenKind getTokenKind(char* token, TokenKind prev_kind)
{
    //if we have op_plus or op_minus and it's the first item, 
    //after another binary op or after `(` then it is unary
    //convert it to op_neg and op_pos
    //handle these in algorithm, precedemce and associativity
    
    int len = strlen(token);
    if ( len == 1 && token[0] == '(') return OPEN_PAREN;
    if ( len == 1 && token[0] == ':') return OP_COLON;;
    if ( len == 1 && token[0] == ')') return CLOSE_PAREN;
    if ( len == 1 && token[0] == '+') 
    {
        if ( prev_kind == NA || prev_kind == OPEN_PAREN || prev_kind == OPEN_BRACE ) return OP_POS;
        if ( isBinaryOp(prev_kind) ) return OP_POS;
        return OP_ADD;
    }

    if ( len == 1 && token[0] == '-') 
    {
        if ( prev_kind == NA || prev_kind == OPEN_PAREN || prev_kind == OPEN_BRACE ) return OP_NEG;
        if ( isBinaryOp(prev_kind) ) return OP_NEG;
        return OP_SUB;
    }

    if ( len == 1 && token[0] == '*') return OP_MUL;
    if ( len == 1 && token[0] == '/') return OP_DIV;
    if ( len == 1 && token[0] == '%') return OP_REM;
    if ( len == 1 && token[0] == '{') return OPEN_BRACE;
    if ( len == 1 && token[0] == '}') return CLOSE_BRACE;
    if ( len == 1 && token[0] == ',') return COMMA;
    if ( len == 2 && token[0] == '%' && token[1] == '%' )  return OP_DVT;
    if ( len == 2 && token[0] == ':' && token[1] == '=' )  return OP_BIND;
    if ( len == 2 && token[0] == ':' && token[1] == ':' )  return OP_RETURN;
    if ( len == 2 && token[0] == '-' && token[1] == '>' )  return OP_ARROW;
    if ( len == 2 && token[0] == '>' && token[1] == '>' )  return OP_SHR;
    if ( len == 2 && token[0] == '<' && token[1] == '<' )  return OP_SHL;
    if ( isdigit(token[0]) ) return INT_LITERAL;

    if ( !strcmp(token, "true") || !strcmp(token, "false") ) return BOOL_LITERAL;

    return IDENTIFIER;
}

int getOperatorPrecedence(TokenKind kind)
{
    //numbers are based on C language operator precedence
    //http://en.cppreference.com/w/c/language/operator_precedence
    //numbers are reversed as they are 1-n in link above
    switch ( kind )
    {
        case OP_ADD: return 11;
        case OP_SUB: return 11;
        case OP_MUL: return 12;
        case OP_DIV: return 12;
        case OP_REM: return 12;
        case OP_DVT: return 12;
        case OP_POS: return 13;
        case OP_NEG: return 13;
        case OP_SHR: return 10;
        case OP_SHL: return 10;
        default: { errorLog("Aborting! Invalid operator: %d\n", kind); abort(); }
    }
}
            
bool isLeftAssociative(TokenKind kind)
{
    if ( kind == OP_POS || kind == OP_NEG ) return false;
    return true;
}

bool matchText(Context* context, const char* text)
{
    char token[256];

    SAVE_POSITION;

    getNextToken(context, token);

    if ( token[0] == 0 ) return false;

    bool result = (strcmp(token, text) == 0);

    if ( !result )
    {
        RESTORE_POSITION;
    }

    return result;
}

bool matchLiteral(Context* context, int kind)
{
    char token[256];

    SAVE_POSITION;

    getNextToken(context, token);

    if ( token[0] == 0 ) return false;

    bool result = (getTokenKind(token, NA) == kind);

    if ( !result )
    {
        RESTORE_POSITION;
    }

    return result;
}

void getNextToken(Context* context, char* token)
{
    const char* complex_ops[] = { "%%", ":=", "::", "->", ">>", "<<"};
    const char* simple_ops = "<>=+-*/()[].,{}:%";

    while ( 1 ) 
    {
        char c = getChar(context);

        //check for whitespace and EOF
        if ( c == EOF ) 
        {
            token[0] = 0;
            return;
        }
        if ( isspace(c) ) continue;

        //ignore comments
        if ( c == '#' ) 
        {
            char temp[1024];
            fgets(temp, 1024, context->input_file);
            continue;
        }

        if ( strchr(simple_ops, c) != NULL )
        {
            token[0] = c;

            char next_c = getChar(context);

            for(size_t i=0;i<sizeof(complex_ops)/sizeof(complex_ops[0]);i++)
            {
                if ( complex_ops[i][0] == c && complex_ops[i][1] == next_c )
                {
                    token[1] = next_c;
                    token[2] = 0;
                    return;
                }
            }
            undoChar(context, next_c);
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
