//
// Created by Marcin on 20.03.2021.
//
#pragma once
#ifndef ROC_LANG_EXTENSIONS_H
#define ROC_LANG_EXTENSIONS_H

#include <llvm/IR/Function.h>

#include <utility>
#include "Types.h"

class FunctionDeclarationTargetWrapper;

using namespace llvm;

class FunctionDeclarationTargetWrapper : public TargetFunctionCall {
public:
    FunctionDeclaration* functionDeclaration;

    explicit FunctionDeclarationTargetWrapper(FunctionDeclaration *functionDeclaration) : functionDeclaration(
            functionDeclaration) {}

    std::string getName() override {
        return functionDeclaration->getName()->getText();
    }

    std::vector<RocType *> getArgumentTypes() override {
        return ((RocFunctionContext*) functionDeclaration->getContextHolder("typeContext"))->parameterTypes;
    }

    RocType *getReturnType() override {
        return ((RocFunctionContext*) functionDeclaration->getContextHolder("typeContext"))->returnType;
    }
};

/**
 * Predefined functions i.e. calling toString() on Int32 type
 */
class PredefinedTargetMethodCall : public TargetFunctionCall {
public:
    std::unique_ptr<RocType> owner;
    std::string name;
    std::string realName;
    std::vector<std::unique_ptr<RocType>> argTypes;
    std::unique_ptr<RocType> returnType;

    PredefinedTargetMethodCall(RocType* owner,
                               std::string name,
                               std::string realName,
                               const std::vector<RocType*>& argTypes,
                               RocType* returnType) {

        this->owner = std::unique_ptr<RocType>(owner);
        this->name = std::move(name);
        this->realName = std::move(realName);
        for (auto& rc: argTypes) {
            this->argTypes.push_back(std::unique_ptr<RocType>(rc));
        }
        this->returnType = std::unique_ptr<RocType>(returnType);
    }

    std::string getName() override {
        return name;
    }

    std::string getRealName() override {
        return realName;
    }

    std::vector<RocType *> getArgumentTypes() override {
        std::vector<RocType*> result;
        for (auto& rc: argTypes) {
            result.push_back(rc.get());
        }
        return result;
    }

    RocType *getReturnType() override {
        return returnType.get();
    }
};

#endif //ROC_LANG_EXTENSIONS_H
