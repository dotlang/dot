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

    FILE* out = fopen("out", "w");

    char* outData = LLVMPrintModuleToString(module);
    fputs(outData, out);
    LLVMDisposeMessage(outData);

    return 0;
}
