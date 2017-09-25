#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "re.h"

#define OK   1
#define FAIL 0


//TODO: extract values of tokens (binding name and number)
//TODO: generate output code with llvm API
//
/*
 * Grammar of the language:
 * MODULE := BINDING
 * BINDING := IDENTIFIER ':=' '()->' NUMBER
 * IDENTIFIER := \w+
 * NMUBER := \d+
 */

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

int parseModule(FILE* file)
{
    Binding b;
    int result = parseBinding(file, &b);

    printf("number is %d\n", b.number);
    return result;
}

int main(int argc, char** argv)
{
	FILE *file;

    file = fopen(argv[1], "r");

    int result = parseModule(file);
    printf("result is: %s\n", (result == FAIL)?"FAIL":"OK");

    fclose(file);

    return 219;
}
