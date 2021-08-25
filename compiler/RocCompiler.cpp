//
// Created by Marcin on 19.02.2021.
//
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include <fstream>
#include <utility>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "RocCompiler.h"
#include "Types.h"
#include "LLVMBackend.h"
#include "Builtins.h"
#include "Extensions.h"
#include "../linking/API.h"
#include "../linking/Math.h"
#include "../passes/MemoryPass.h"

using namespace llvm;

CompilationContext::CompilationContext() {
    this->config = std::make_unique<Config>();
    this->builtinFunctionResolver = new BuiltinFunctionResolver();
};

CompilationNode *CompilationContext::currentCompilationNode() {
    if (this->compilationNodes.empty()) {
        return nullptr;
    } else {
        return this->compilationNodes.back();
    }
}

void CompilationContext::pushCompilationNode(CompilationNode *cn) {
    compilationNodes.push_back(cn);
    if (cn->isModule()) {
        moduleStack.push_back((ModuleDeclaration*) cn);
    }
}

TargetFunctionCall * CompilationContext::findFunctionByName(int typeId, const std::string& name, int args) {
    auto vec = this->targetFunctionsRegister.find(typeId);
    if (vec == this->targetFunctionsRegister.end()) {
        return nullptr;
    }
    for (auto& f: vec->second) {
        if (f->getName() == name && f->getArgumentTypes().size() == args) {
            return f;
        }
    }
    return nullptr;
}

void CompilationContext::insertTargetFunction(FunctionDeclarationTargetWrapper *target) {
    auto it = this->targetFunctionsRegister.find(-1);
    if (it != this->targetFunctionsRegister.end()) {
        it->second.push_back(target);
    } else {
        std::vector<TargetFunctionCall*> vec;
        vec.push_back(target);
        this->targetFunctionsRegister.insert({-1, vec});
    }
}

void CompilationContext::insertTargetFunction(PredefinedTargetMethodCall *target) {
    auto it = this->targetFunctionsRegister.find(target->owner->typeId());
    if (it != this->targetFunctionsRegister.end()) {
        it->second.push_back(target);
    } else {
        std::vector<TargetFunctionCall*> vec;
        vec.push_back(target);
        this->targetFunctionsRegister.insert({target->owner->typeId(), vec});
    }
}

void CompilationContext::reportProblem(const char *msg, ASTNode *toMark) {
    auto* problem = new CompileTypeException(msg, toMark, this);
    this->typeProblems.push_back(problem);
}

CompilationNode *CompilationContext::popCompilationNode() {
    if (this->compilationNodes.empty()) {
        return nullptr;
    } else {
        CompilationNode *cn = this->compilationNodes.back();
        if (cn->isModule()) {
            this->moduleStack.pop_back();
        }
        this->compilationNodes.pop_back();
        return cn;
    }
}

RocCompilationResult * RocCompiler::compile(const std::string& filePath) {
    Lexer lexer(filePath);
    ParseContext parseContext(&lexer);

    try {
        ModuleParser moduleParser(&parseContext);
        moduleParser.absolutePath = filePath;
        moduleParser.parse();
        if (!moduleParser.syntaxExceptions.empty()) {
            for (const auto &item : moduleParser.syntaxExceptions) item.printMessage();
            return nullptr;
        }
        auto md = moduleParser.parseContext->moduleDeclarations.back();
        auto ctx = std::make_unique<CompilationContext>();
        return RocCompiler::compile(std::move(md), ctx.get());
    } catch (SyntaxException &ex) {
        ex.printMessage();
        return nullptr;
    }
}

RocCompilationResult* RocCompiler::compile(const std::string& expr, const std::string& outputFileName) {
    std::ofstream f(outputFileName);
    f << expr;
    f.close();
    return compile(outputFileName);
}

RocCompilationResult* RocCompiler::compile(std::shared_ptr<ModuleDeclaration> moduleDeclaration,
                                           CompilationContext* compilationContext) {

    //ASTPrinter astPrinter;
    //moduleDeclaration->accept(&astPrinter);

    LiteralResolver literalResolver(compilationContext);
    literalResolver.visit(moduleDeclaration.get());

    FunctionSignatureResolver functionSignatureResolver(compilationContext);
    functionSignatureResolver.visit(moduleDeclaration.get());

    TypeResolver typeResolver(compilationContext);
    typeResolver.visit(moduleDeclaration.get());

    if (!compilationContext->typeProblems.empty()) {
        for (auto problem: compilationContext->typeProblems) {
            problem->printMessage();
        }
        return nullptr;
    }

    LLVMBackendProvider backendProvider;
    return backendProvider.compile(std::move(moduleDeclaration), compilationContext);
}

void LiteralResolver::visit(LiteralExpr *literalExpr) {
    if (this->compilationContext->currentCompilationNode()->isFunctionDeclaration()) {
        auto fd = (FunctionDeclaration*) this->compilationContext->currentCompilationNode();
        auto localSymbols = fd->localSymbols;
        auto it = localSymbols.find(literalExpr->literal->getText());
        if (it != localSymbols.end()) {
            auto localAccess = std::make_unique<LocalAccess>(std::move(literalExpr->literal));
            localAccess->localVariableRef = it->second;
            localAccess->setParent(literalExpr->getParent());
            literalExpr->getParent()->replaceChild(literalExpr, std::move(localAccess));
            return;
        }
    }
    throw SyntaxException(("Unknown Symbol: " + literalExpr->getText()).c_str(),
                          literalExpr->literal.get(),
                          moduleDeclaration->absolutePath);
}

void LiteralResolver::visit(FunctionDeclaration *functionDeclaration) {
    this->compilationContext->pushCompilationNode(functionDeclaration);
    int i=0;
    for (auto& p: functionDeclaration->parameterList->parameters) {
        auto lv = std::make_unique<LocalVariableRef>();
        lv->name = p->name->getText();
        lv->index = i;
        lv->parameter = p;
        functionDeclaration->localSymbols.insert({p->getText(), lv.get()});
        functionDeclaration->locals.push_back(std::move(lv));
        i++;
    }
    ASTVisitor::visit(functionDeclaration);
    this->compilationContext->popCompilationNode();
}

void LiteralResolver::visit(ModuleDeclaration *moduleNode) {
    this->moduleDeclaration = moduleNode;
    this->compilationContext->pushCompilationNode(moduleNode);
    moduleNode->staticBlock->accept(this);
    for (auto& f: moduleNode->functions) {
        f->accept(this);
    }
    this->compilationContext->popCompilationNode();
}

BackendProvider::BackendProvider(RocBackendType backendType) : backendType(backendType) {}

LLVMBackendProvider::LLVMBackendProvider() : BackendProvider(RocBackendType::llvmB) {}

int verifyModule1(Module* M) {
    errs() << "verifying... ";
    if (verifyModule(*M)) {
        errs() << ": Error constructing function!\n";
        return 1;
    }
    return 0;
}


void test() {
    std::cout << "TestTest";
}

RocCompilationResult* LLVMBackendProvider::compile(std::shared_ptr<ModuleDeclaration> moduleDeclaration,
                                                   CompilationContext *compilationContext) {


    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    LLVMContext Context;
    std::unique_ptr<Module> Owner(new Module(moduleDeclaration->moduleName, Context));
    Module *M = Owner.get();

    ToMIRVisitor toMirVisitor;
    toMirVisitor.visit(moduleDeclaration.get());
    toMirVisitor.mirModule->moduleDeclaration = std::move(moduleDeclaration);
    toMirVisitor.mirModule->visit(compilationContext->builtinFunctionResolver);

    LabelResolver labelResolver;
    toMirVisitor.mirModule->visit(&labelResolver);

    SmartTypeCaster smartTypeCaster;
    toMirVisitor.mirModule->visit(&smartTypeCaster);

    HeapAllocatorLifter heapAllocatorLifter;
    toMirVisitor.mirModule->visit(&heapAllocatorLifter);

    ToLLVMVisitor visitor(&Context, M);
    toMirVisitor.mirModule->visit(&visitor);
    verifyModule1(M);

    auto cr = new RocCompilationResult();
    auto TargetTriple = sys::getDefaultTargetTriple();
    M->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        errs() << Error;
        return cr;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto TheTargetMachine =
            Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    M->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = "output.s";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();
        return cr;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_AssemblyFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return cr;
    }

    pass.run(*M);
    dest.flush();

    std::string errStr;
    auto *EE =
            EngineBuilder(std::move(Owner))
                    .setErrorStr(&errStr)
                    .create();
    EE->addGlobalMapping("myIntToString", (uint64_t) myIntToString);
    EE->addGlobalMapping("myVTableFactory", (uint64_t) myVTableFactory);
    EE->addGlobalMapping("myGetFunctionPointer", (uint64_t) myGetFunctionPointer);
    EE->addGlobalMapping("myPrintln", (uint64_t) myPrintln);
    EE->addGlobalMapping("myPrint", (uint64_t) myPrint);
    EE->addGlobalMapping("myInt32ToString", (uint64_t) myInt32ToString);
    EE->addGlobalMapping("addVTableMapping", (uint64_t) addVTableMapping);
    EE->addGlobalMapping("myStringRawRTypeToString", (uint64_t) myStringRawRTypeToString);

    EE->addGlobalMapping("myInitRawString", (uint64_t) myInitRawString);
    EE->addGlobalMapping("myInitInt32", (uint64_t) myInitInt32);

    EE->addGlobalMapping("Sqrt", (uint64_t) Sqrt);


    auto fa = EE->getFunctionAddress("main");

    cr->mainFunction = visitor.mainFunction;
    cr->mainFunctionPtr = fa;
    cr->EE = EE;

    return cr;
}

RocTypeNodeContext * LLVMBackendProvider::getFunctionContext() {
    return new RocFunctionContext();
}

