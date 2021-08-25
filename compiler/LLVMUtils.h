//
// Created by Marcin Bukowiecki on 28.03.2021.
//
#pragma once
#ifndef ROC_LANG_LLVMUTILS_H
#define ROC_LANG_LLVMUTILS_H

namespace llvm {
    class LLVMContext;
    class Module;
    class BasicBlock;
    class Value;
}

class RocType;
class RocLLVMContext;
class ToLLVMVisitor;

llvm::Value* newRocRawStruct(ToLLVMVisitor *visitor, llvm::Value* rawString, int length);
llvm::Value* newRocInt32Struct(ToLLVMVisitor *visitor, llvm::Value* value);

void createPuts(llvm::LLVMContext *llvmContext,
                llvm::Module *module,
                const std::string& text,
                llvm::BasicBlock *place);

void createFunctionDispatcherForAny(RocLLVMContext *rocLLVMContext);

void createStringRawVTable(RocLLVMContext *rocLlvmContext);
void createInt32VTable(RocLLVMContext *rocLlvmContext);

llvm::Value* callMalloc(llvm::LLVMContext *llvmContext,
                        llvm::Module *module,
                        long long size,
                        llvm::BasicBlock *place,
                        const std::string& name = "");

llvm::Value* castTo(llvm::Value* from, llvm::Type *to, llvm::BasicBlock *place);

llvm::Value* getPointerTo(RocLLVMContext *rocLLVMContext, llvm::Value* from, llvm::BasicBlock *place, int index);

llvm::Value* storeValue(llvm::Value* value, llvm::Value *to, llvm::BasicBlock *place);

void createGetTypeIdFunction(RocLLVMContext *rocLlvmContext, RocType *rocType);

#endif //ROC_LANG_LLVMUTILS_H
