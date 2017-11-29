#ifndef __BASIC_PARSERS_H__
#define __BASIC_PARSERS_H__

#include "ast.h"

void ignoreWhitespace(Context* context);
int parseMultipleChoiceLiteral(Context* context, const int, const char** choices);
int parseLiteral(Context* context, const char* literal);
int parseNumber(Context* context, char* token);
int parseIdentifier(Context* context, char* token);
int strToOp(const char* str);
const char* opToStr(int op);

#endif
