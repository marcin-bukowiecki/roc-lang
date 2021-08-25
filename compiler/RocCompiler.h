//
// Created by Marcin Bukowiecki on 15.02.2021.
//
#pragma once
#ifndef ROC_LANG_ROCCOMPILER_H
#define ROC_LANG_ROCCOMPILER_H

#include <vector>
#include "../parser/AST.h"

class RocTypeNodeContext;
class RocCompiler;
class CompilationContext;
class BuiltinFunctionResolver;
class RocCompilationResult;
class TargetFunctionCall;
class FunctionDeclarationTargetWrapper;
class PredefinedTargetMethodCall;
class CompileTypeException;

namespace llvm {
    class Function;
    class ExecutionEngine;
};

class LiteralResolver : public ASTVisitor {
public:
    CompilationContext* compilationContext;
    ModuleDeclaration* moduleDeclaration{};

    explicit LiteralResolver(CompilationContext* compilationContext) {
        this->compilationContext = compilationContext;
    }

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(ModuleDeclaration *moduleNode) override;

    void visit(LiteralExpr *) override;
};

enum RocBackendType {
    rustB,
    llvmB
};

class BackendProvider {
public:
    RocBackendType backendType;

    explicit BackendProvider(RocBackendType backendType);

    virtual RocCompilationResult *compile(std::shared_ptr<ModuleDeclaration> moduleDeclaration,
                                          CompilationContext *compilationContext) = 0;

    virtual RocTypeNodeContext *getFunctionContext() = 0;
};

class LLVMBackendProvider : public BackendProvider {
public:
    LLVMBackendProvider();

    RocCompilationResult *compile(std::shared_ptr<ModuleDeclaration> moduleDeclaration,
                                  CompilationContext *compilationContext) override;

    RocTypeNodeContext *getFunctionContext() override;
};

/**
 * Main context to hold compilation information
 */
class CompilationContext {
private:
    int functionIdCounter = 1000;

public:
    std::list<CompilationNode *> compilationNodes;
    std::vector<ModuleDeclaration *> moduleStack;
    std::string stringAccumulator;
    bool mainInitialized = false;
    std::unique_ptr<Config> config;
    BuiltinFunctionResolver *builtinFunctionResolver;
    std::map<int /* typeId */, std::vector<TargetFunctionCall*>> targetFunctionsRegister{};
    std::vector<CompileTypeException*> typeProblems;

    CompilationContext();

    CompilationNode *currentCompilationNode();

    CompilationNode *popCompilationNode();

    void pushCompilationNode(CompilationNode *);

    TargetFunctionCall* findFunctionByName(int typeId, const std::string& name, int args);

    void insertTargetFunction(FunctionDeclarationTargetWrapper *target);

    void insertTargetFunction(PredefinedTargetMethodCall *target);

    void reportProblem(const char *message, ASTNode* toMark);
};

/**
 * Main class for compilation
 */
class RocCompiler {
public:

    static RocCompilationResult *compile(const std::string& filePath);

    static RocCompilationResult *compile(const std::string& expr, const std::string& outputFileName);

    static RocCompilationResult *compile(std::shared_ptr<ModuleDeclaration> moduleDeclaration,
                                         CompilationContext *compilationContext);
};

class RocCompilationResult {
public:
    uint64_t mainFunctionPtr;
    llvm::Function *mainFunction;
    llvm::ExecutionEngine *EE;
};

#endif //ROC_LANG_ROCCOMPILER_H
