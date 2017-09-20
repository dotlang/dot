#include <stdio.h>
#include <string.h>
#include "re.h"

enum TOKEN_ID
{
    UNKNOWN,
    WHITESPACE,
    ARROW,
    BIND,
    LEFT_PAREN,
    RIGHT_PAREN,
    NUMBER,
    IDENTIFIER
};

typedef struct
{
    char pattern[32];
    re_t compiled_pattern;
    int token_type;
} TokenSpec;

int token_count = 7;
TokenSpec tokens[] = {
    {.pattern = "[\\s]+",   .token_type = WHITESPACE},
    {.pattern = "->",      .token_type = ARROW},
    {.pattern = "(",       .token_type = LEFT_PAREN},
    {.pattern = ")",       .token_type = RIGHT_PAREN},
    {.pattern = ":=",      .token_type = BIND},
    {.pattern = "[\\d]+",   .token_type = NUMBER},
    {.pattern = "[\\w]+",   .token_type = IDENTIFIER},
};

char* tokenTypeToString(int type)
{
    switch(type)
    {
        case WHITESPACE: return "WHITESPACE";
        case ARROW: return "ARROW";
        case LEFT_PAREN: return "LEFT_PAREN";
        case RIGHT_PAREN: return "RIGHT_PAREN";
        case BIND: return "BIND";
        case NUMBER: return "NUMBER";
        case IDENTIFIER: return "IDENTIFIER";
    }

    return "UNKNOWN";
}

int getTokenType(char* token, char next_char)
{
    int len = strlen(token);
    token[len] = next_char;
    token[len+1] = 0;
    int i =0;

    for(i=0;i<token_count;i++)
    {
        if ( re_matchp(tokens[i].compiled_pattern, token) == 0 ) break;
    }

    token[len=0;

    if ( i < token_count ) return tokens[i].token_type;

    return UNKNOWN;
}

int main(int argc, char** argv)
{
    for(int i=0;i<token_count;i++)
    {
        tokens[i].compiled_pattern = re_compile(tokens[i].pattern);
    }

	FILE *file;

    file = fopen(argv[1], "r");
    char token[256];
    int token_length = 0;
    int token_type = UNKNOWN;
    int last_token_type = UNKNOWN;
    memset(token, 0, 256);

    for(;;) 
    {
      char c = (char)fgetc(file);
	  if ( c == EOF ) break;
	  if ( c == '#' )
      {
          char temp = c;
          while ( temp != EOF && temp != '\n' ) temp = (char)fgetc(file);
          continue;
      }

	  int token_type = getTokenType(token, c);

	  if ( token_type != last_token_type ) 
      {
          printf(tokenTypeToString(last_token_type))
      }

	  printf("%c", c);
    }

    fclose(file);

    return 219;
}
