//
// Created by Marcin Bukowiecki on 19.03.2021.
//
#pragma once
#ifndef ROC_LANG_BUILTINS_H
#define ROC_LANG_BUILTINS_H

#include "../mir/MIR.h"
#include <map>
#include <string>

class RocLLVMContext;

void definePrintln(RocLLVMContext *rocLlvmContext);
void definePrint(RocLLVMContext *rocLlvmContext);

void defineInt32ToStringRaw(RocLLVMContext *rocLlvmContext);

void defineStringRawToStringRaw(RocLLVMContext *rocLlvmContext);
void defineStringRawTypeId(RocLLVMContext *rocLlvmContext);

void defineAnyType(RocLLVMContext *rocLlvmContext);
void defineStringRawType(RocLLVMContext *rocLlvmContext);
void defineStringType(RocLLVMContext *rocLlvmContext);
void defineIntType(RocLLVMContext *rocLlvmContext);
void createFunctionDispatcher(RocLLVMContext *rocLlvmContext);

void createAnyToString(RocLLVMContext *rocLlvmContext);

class BuiltinFunction : public TargetFunctionCall {
public:
    std::string name;
    std::vector<std::unique_ptr<RocType>> typeParameters;
    std::vector<std::unique_ptr<RocType>> parameters;
    std::unique_ptr<RocType> returnType;
    bool varArgs = false;

    explicit BuiltinFunction(std::string name) {
        this->name = std::move(name);
        this->returnType = std::make_unique<UnitRocType>();
    }

    explicit BuiltinFunction(std::string name,
                             const std::vector<RocType*>& parameters,
                             RocType* returnType,
                             bool varArgs = false) {
        this->name = std::move(name);
        for (auto& p: parameters) {
            this->parameters.push_back(std::unique_ptr<RocType>(p));
        }
        this->returnType = std::unique_ptr<RocType>(returnType);
        this->varArgs = varArgs;
    }

    explicit BuiltinFunction(std::string name,
                             const std::vector<RocType*>& typeParameters,
                             const std::vector<RocType*>& parameters,
                             RocType* returnType) {
        this->name = std::move(name);
        for (auto& p: typeParameters) {
            this->typeParameters.push_back(std::unique_ptr<RocType>(p));
        }
        for (auto& p: parameters) {
            this->parameters.push_back(std::unique_ptr<RocType>(p));
        }
        this->returnType = std::unique_ptr<RocType>(returnType);
    }

    RocType * getReturnType() override {
        return this->returnType.get();
    }

    std::string getName() override {
        return name;
    }

    std::vector<RocType *> getArgumentTypes() override {
        std::vector<RocType*> result;
        for (auto& p : this->parameters) {
            result.push_back(p.get());
        }
        return result;
    }
};

/**
 * Visitor for resolving builtins functions i.e. len, println etc.
 */
class BuiltinFunctionResolver : public MIRVisitor {
public:
    std::map<std::string, std::vector<std::unique_ptr<BuiltinFunction>>> functions;

    BuiltinFunctionResolver();

    void addFunction(BuiltinFunction* function) {
        auto it = functions.find(function->name);
        if (it == functions.end()) {
            std::vector<std::unique_ptr<BuiltinFunction>> f;
            f.push_back(std::unique_ptr<BuiltinFunction>(function));
            functions.insert({function->name, std::move(f)});
        } else {
            it->second.push_back(std::unique_ptr<BuiltinFunction>(function));
        }
    }
};

#endif //ROC_LANG_BUILTINS_H
