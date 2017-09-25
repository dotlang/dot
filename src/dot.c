#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "re.h"

#define OK 1
#define FAIL 0

/*
 * Grammar of the language:
 * MODULE := BINDING
 * BINDING := IDENTIFIER ':=' '()->' NUMBER
 * IDENTIFIER := \w+
 * NMUBER := \d+
 */

void ignoreLine(FILE* file)
{
    char temp[1024];
    fgets(temp, 1024, file);
}

void ignoreWhitespace(FILE* file)
{
    char c = (char)fgetc(file);
    while ( isspace(c) || c == '#' ) 
    {
        if ( c == EOF ) break;
        if ( c == '#' ) ignoreLine(file);

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

int parseNumber(FILE* file)
{
    ignoreWhitespace(file);
    char c = (char)fgetc(file);
    if ( !isdigit(c) ) return FAIL;
    while ( c != EOF && isdigit(c) ) 
    {
        printf("%c\n", c);
        c = (char)fgetc(file);
    }
    printf("%c\n", c);

    return OK;
}

int parseIdentifier(FILE* file) 
{
    ignoreWhitespace(file);

    char c = (char)fgetc(file);
    if ( !isalpha(c) ) return FAIL;
    while ( c != EOF && isalpha(c) ) c = (char)fgetc(file);

    return OK;

}


int parseBinding(FILE* file)
{
    int result = parseIdentifier(file);
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, ":=");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "()");
    if ( result == FAIL ) return FAIL;

    result = parseLiteral(file, "->");
    if ( result == FAIL ) return FAIL;

    result = parseNumber(file);
    if ( result == FAIL ) return FAIL;

    return OK;
}

int parseModule(FILE* file)
{
    return parseBinding(file);
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
