#ifndef __BASIC_COMPILERS_H__
#define __BASIC_COMPILERS_H__

#include "ast.h"

void compileModule(Context*, Module*);
void disposeLlvm(Context*);

#endif
