#ifndef __COMPILERS_H__
#define __COMPILERS_H__

#include "ast.h"

void compileModule(Context*, Module*);
void disposeLlvm(Context*);

#endif
