#include <stdio.h>
#include <string.h>
#include "re.h"

//represents a binding:
//example 1: x := 12
//example 2: f := () -> 19.1
//example 3: g := (x:int) -> x*2
typedef struct 
{
    char name[32];      //whatever comes before :=
    char body_raw[256]; //whatever comes after :=
} Binding;

//first phase: lex-read-binding-name

//ignores rest of the current line in the file. Returns 1 if file is finished
char ignoreLine(FILE* file)
{
    char temp = '#';
    while ( temp != EOF && temp != '\n' ) temp = (char)fgetc(file);
    ungetc(temp, file);

    return temp;
}


/*
 * Steps:
 * 1. Pre-process file (ignore whitespace and comments) to extract functions
 * 1.1. Validate spacing, naming, ...
 * 2. Complete parts of the function structure little by little
 * 3. Do optimizations that are possible (make lambdas normal functions, replace chain operator with normal function call, 
 *      decompose expressions into simple exps, replace .() with normal code, replace casting with core function calls, ...)
 * 4. Process inside body (extract expressions)
 * 5. Add required statements like dispose
 * 6. Generate code
 * 7. Further opt.
 * 8. Cache function code and dependencies for use in incremental compilation
 * See http://lindseykuper.livejournal.com/307725.html
 */

re_t re_identifier;

int readToken(FILE* file, re_t pattern, char* token)
{
    char c = (char)fgetc(file);
    if ( c == EOF ) return 0;
    while ( c == '#' ) 
    {
        ignoreLine(file);
        c = (char)fgetc(file);
        while(isspace(c)) c = (char)fgetc(file);
    }

    token[0] = c;
    token[1] = 0;

    int token_length = 1;

    while( c != EOF && re_matchp(pattern, token) == 0 )
    {
        c = (char) fgetc(file);
        token[token_length] = c;
        token_length++;
        token[token_length]=0;
    }

    if ( token_length == 1 )
    {
        //this means that even the first character did not match with the given regex
        ungetc(c, file);
        return 0;
    }

    ungetc(c, file);
    token_length--;
    token[token_length] = 0;

    return 1;
}

int main(int argc, char** argv)
{
    re_identifier = re_compile("^[\\w]+$");

	FILE *file;

    file = fopen(argv[1], "r");

    for(;;) 
    {
        char token[32];

        int result = readToken(file, re_identifier, token);
        if (result != 1) break;

        printf("found token: %s", token);
        printf("\n");

        ignoreLine(file);
    }

    fclose(file);

    return 219;
}
