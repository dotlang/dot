//This is the minimum code te generate LLVM code which just outputs a fixed value as return value.
//To run this example:
//1. clang++ -std=c11 `llvm-config --cflags` -x c test.c `llvm-config --ldflags --libs core analysis native bitwriter --system-libs` -lm -o test
//2. Find a file named `test` and execute it
//3. Find a file named `out.ll` and execute it using lli: `lli out.ll`
//4. `echo $?` the result should be the value specified here
#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/TargetMachine.h"

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int main() {

	LLVMModuleRef module = LLVMModuleCreateWithName("test");
	LLVMSetDataLayout(module, "");
	LLVMSetTarget(module, LLVMGetDefaultTargetTriple());

	LLVMBuilderRef builder = LLVMCreateBuilder();
	LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
	LLVMValueRef mainfunc = LLVMAddFunction(module, "main", funcType);
	LLVMBasicBlockRef entry = LLVMAppendBasicBlock(mainfunc, "entry");
	LLVMPositionBuilderAtEnd(builder, entry);

    LLVMTypeRef intType = LLVMIntType(32);
    LLVMValueRef val = LLVMConstInt(intType, 196, true);
    LLVMBuildRet(builder, val);

	char *error = NULL;
	bool isInvalid = LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
	LLVMDisposeMessage(error);

    FILE* out = fopen("out.ll", "w");

    char* outData = LLVMPrintModuleToString(module);
    fputs(outData, out);
    LLVMDisposeMessage(outData);

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);

    return 0;
}
