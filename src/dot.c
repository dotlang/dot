#include <stdio.h>
#include <string.h>

enum TOKEN_ID
{
    INCOMPLETE,
    OTHER
};

int getTokenId(char* token, char next_char)
{
    return INCOMPLETE;
}

int main(int argc, char** argv)
{
	FILE *file;

    file = fopen(argv[1], "r");
    char token[256];
    int token_length = 0;
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

	  int token_id = getTokenId(token, c);
	  printf("%c", c);
    }

    fclose(file);

    return 219;
}
