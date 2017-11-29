#ifndef __DEBUG_HELPERS_H__
#define __DEBUG_HELPERS_H__

#include "ast.h"

void debugLog(Context*, const char*, ...);
void dumpExpression(Context*, Expression*);

#endif

