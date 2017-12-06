#ifndef __BASIC_PARSERS_H__
#define __BASIC_PARSERS_H__

#include "ast.h"

void getNextToken(Context* context, char* token);
bool newLineAhead(Context* context);
void peekNextToken(Context* context, char* token);
TokenKind getTokenKind(char* token);
int getOperatorPrecedence(TokenKind kind);
bool isLeftAssociative(TokenKind kind);

#endif
