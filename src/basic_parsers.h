#ifndef __BASIC_PARSERS_H__
#define __BASIC_PARSERS_H__

#include "ast.h"


#define SAVE_POSITION long initial_position = ftell(context->input_file)
#define RESTORE_POSITION fseek(context->input_file, initial_position, SEEK_SET)


void getNextToken(Context* context, char* token);
bool newLineAhead(Context* context);
TokenKind getTokenKind(char* token, TokenKind prev_kind);
int getOperatorPrecedence(TokenKind kind);
bool isLeftAssociative(TokenKind kind);
bool matchLiteral(Context* context, int kind);
//TODO: temporary to match with type names
bool matchText(Context* context, const char* text);

#endif
