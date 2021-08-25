//
// Created by Marcin Bukowiecki on 13.03.2021.
//
#pragma once
#ifndef ROC_LANG_LLVMBACKEND_H
#define ROC_LANG_LLVMBACKEND_H

#include <utility>

#include "llvm/IR/Module.h"
#include "Types.h"
#include "Constants.h"
#include "../mir/MIR.h"

using namespace llvm;

class RocRawStringType;
class UnitRocType;
class RocLLVMContext;

class VTableEntry {
public:
    virtual std::string getFunctionName() = 0;

    virtual FunctionCallee getFunctionCallee() = 0;

    virtual long long getFunctionId() = 0;
};

class BuiltinVTableEntry : public VTableEntry {
private:
    std::string functionName;
    FunctionCallee functionCallee;

public:
    explicit BuiltinVTableEntry(std::string functionName,
                                FunctionCallee functionCallee) : functionCallee(functionCallee) {
        this->functionName = std::move(functionName);
    }

    FunctionCallee getFunctionCallee() override {
        return functionCallee;
    }

    std::string getFunctionName() override {
        return functionName;
    }

    long long getFunctionId() override {
        return roc::methods::getMethodId(getFunctionName());
    }
};

class RocLLVMContext {
public:
    LLVMContext* llvmContext;
    Module* module;

    Type* voidType{};

    Type* boolType{};

    Type* int8Type{};
    Type* int32Type{};
    Type* int64Type{};

    Type* float32Type{};
    Type* float64Type{};

    StructType* stringRawType{};
    StructType* stringType{};

    Type* functionEntryType{};

    StructType* int32StructType{};
    StructType* anyTypeStructType{};

    std::map<std::string, FunctionCallee> definedLLVMFunctions;

    RocLLVMContext(LLVMContext *llvmContext, Module *module);

    FunctionCallee findFunction(const std::string& functionName);

};

class ToLLVMVisitor : public MIRVisitor {
public:
    Module* module = nullptr;
    LLVMContext* llvmContext = nullptr;
    RocLLVMContext* rocLLVMContext = nullptr;
    std::vector<MIRFunction*> compilationFunctionStack{};
    BasicBlock* currentBlock{};
    std::vector<Value*> valueStack;
    Function* mainFunction{};
    std::map<TypeEnum, Type*> definedTypesMap{};

    ToLLVMVisitor(LLVMContext* llvmContext, Module *module);

    Value* popLast() {
        auto result = valueStack.back();
        valueStack.pop_back();
        return result;
    }

    Type* createBaseType();

    Type* defineBuiltinStruct(TypeEnum typeEnum) const;

    void visit(MIRModule *mirModule) override;

    void visit(MIRFunction *mirFunction) override;

    void visit(MIRBlock *block) override;

    void visit(MIRCCall *mircCall) override;

    void visit(MIRFunctionCall *mirFunctionCall) override;

    void visit(MIRFunctionInstanceCall *mirFunctionCall) override;

    void visit(MIRRawString *mirRawString) override;

    void visit(MIRConstantInt *mirConstantInt) override;

    void visit(MIRReturnValue *mirReturnValue) override;

    void visit(MIRReturnVoidValue *mirReturnValue) override;

    void visit(MIRInt64Add *mirInt64Add) override;

    void visit(MIRInt32Add *mirInt64Add) override;

    void visit(MIRInt64Sub *mirInt64Add) override;

    void visit(MIRInt32Sub *mirInt64Add) override;

    void visit(MIRInt64Div *mirInt64Div) override;

    void visit(MIRInt32Div *mirInt32Div) override;

    void visit(MIRInt64Mul *mirInt64Mul) override;

    void visit(MIRInt32Mul *mirInt32Mul) override;

    void visit(MIRLocalVariableAccess *localAccess) override;

    void visit(MIRStringToRaw *mirStringToRaw) override;

    void visit(MIRCastTo *mirCastTo) override;

    void visit(MIRToWrapper *mirToWrapper) override;

    void visit(MIRInt32Array *mirInt32Array) override;

    void visit(MIRInt32ArraySet *mirInt32ArraySet) override;

    void visit(MIRInt32ArrayGet *mirInt32ArrayGet) override;

    void visit(MIRAnd *mirAnd) override;

    void visit(MIROr *mirOr) override;

    void visit(MIRInt32Eq *mirAnd) override;

    void visit(MIRInt32NotEq *mirAnd) override;

    void visit(MIRInt32Le *mirAnd) override;

    void visit(MIRInt32Ge *mirAnd) override;

    void visit(MIRInt32Gt *mirAnd) override;

    void visit(MIRInt32Lt *mirAnd) override;

    void visitTrue(MIRTrue *mirTrue) override;

    void visitFalse(MIRFalse *mirFalse) override;

    void visitIf(MIRIf *mirIf) override;

    void visit(MIRToPtr *mirToPtr) override;
};

/**
 * Visitor which transform Roc type into LLVM type
 */
class ToLLVMTypeVisitor : public RocTypeVisitor {
public:
    Type* result{};
    LLVMContext* llvmContext = nullptr;
    RocLLVMContext* rocLLVMContext = nullptr;

    explicit ToLLVMTypeVisitor(RocLLVMContext *llvmContext);

    /**
     * Transform to LLVM array of i8 type [length * i8]
     *
     * @param type given roc type
     */
    void visit(RocRawStringType *type) override;

    void visit(RocStringType *type) override;

    /**
     * Transform to LLVM void type
     *
     * @param type given roc type
     */
    void visit(UnitRocType *type) override;

    /**
     * Transform to LLVM Int32 type
     *
     * @param type given roc type
     */
    void visit(RocInt32Type *type) override;

    /**
     * Transform to LLVM Int32 type
     *
     * @param type given roc type
     */
    void visit(RocInt64Type *type) override;

    void visit(RocFloat64Type *type) override;


    void visit(RocAnyType *type) override;
};

#endif //ROC_LANG_LLVMBACKEND_H


