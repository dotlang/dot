#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>

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

    char out_command[1024];
    sprintf(out_command, "as -o %s.o", base_filename);

	FILE *pipe_fp;
	if (( pipe_fp = popen(out_command, "w")) == NULL)
	{
		perror("popen");
		exit(1);
	}

    fprintf(pipe_fp, "\t.global _start\n");
    fprintf(pipe_fp, "\t.text\n");
    fprintf(pipe_fp, "\t_start:\n");
    fprintf(pipe_fp, "\t\tmov $%d, %%rdi\n", b.number);
    fprintf(pipe_fp, "\t\tmov $60, %%rax\n");
    fprintf(pipe_fp, "\t\tsyscall\n");

    fclose(pipe_fp);


    char ld_command[1024];
    sprintf(ld_command, "ld -o %s %s.o", base_filename, base_filename);
	system(ld_command);

	char rm_command[2014];
	sprintf(rm_command, "rm %s.o", base_filename);

	system(rm_command);
}
