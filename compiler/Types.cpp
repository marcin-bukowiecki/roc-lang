//
// Created by Marcin Bukowiecki on 04.03.2021.
//
#include "Types.h"
#include "Extensions.h"
#include "Builtins.h"
#include "LLVMBackend.h"

RocPtrType * RocType::getPtrType() {
    return new RocPtrType(this);
}

static RocTypeNodeContext* createTypeContext(TypeNode* typeNode) {
    return (RocTypeNodeContext*) typeNode->addContextHolder(TYPE_CONTEXT, new RocTypeNodeContext());
}

static RocTypeNodeContext* getTypeContext(TypeNode* typeNode) {
    return (RocTypeNodeContext*) typeNode->getContextHolder(TYPE_CONTEXT);
}

static RocType* getReturnType(TypeNode* typeNode) {
    return getTypeContext(typeNode)->getGivenType();
}

static void setType(TypeNode* typeNode, RocType* rocType) {
    getTypeContext(typeNode)->setGivenType(rocType);
}

static void setType(Expression* expression, RocType* rocType) {
    ((RocTypeNodeContext*) expression->getContextHolder(TYPE_CONTEXT))->setGivenType(rocType);
}

static RocType* createFromEnum(TypeEnum typeEnum) {
    switch (typeEnum) {
        case int64Type:
            return new RocInt64Type();
        default:
            throw "Unsupported type enum: " + std::to_string(typeEnum);
    }
}

static TargetFunctionCall* findTargetCall(FunctionCallNode *functionCallNode,
                           std::vector<RocType*> callTypes,
                           CompilationContext *compilationContext);

void TypeResolver::visit(ReferenceExpression *referenceExpression) {
    referenceExpression->reference->accept(this);
    referenceExpression->furtherExpression->accept(this);
    createTypeContext(referenceExpression)->setGivenType(getReturnType(referenceExpression->furtherExpression));
}

void TypeResolver::visit(ModuleDeclaration *moduleNode) {
    this->compilationContext->moduleStack.push_back(moduleNode);
    moduleNode->staticBlock->accept(this);
    for (auto& f : moduleNode->functions) {
        f->accept(this);
    }
    this->compilationContext->moduleStack.pop_back();
}

void TypeResolver::visit(ArrayCreateExpr *arrayCreateExpr) {
    auto ctx = createTypeContext(arrayCreateExpr);
    ASTVisitor::visit(arrayCreateExpr);
    ctx->setGivenType(ArrayTypeResolver::resolve(arrayCreateExpr, this->compilationContext));
}

void TypeResolver::visit(StaticBlock *staticBlock) {
    for (auto& expr : staticBlock->expressions) {
        expr->accept(this);
    }
}

void TypeResolver::visit(Parameter *parameter) {
    parameter->typeNode->accept(this);
}

void TypeResolver::visit(StringNode *stringNode) {
    createTypeContext(stringNode)->setGivenType(new RocRawStringType(stringNode->getAsStringValue().length()));
}

void TypeResolver::visit(FunctionCallNode *node) {
    auto fc = (RocFunctionCallContext*) node->addContextHolder(TYPE_CONTEXT, new RocFunctionCallContext());

    for (const auto &item : node->literalExpr->typeVariables) item->accept(this);

    if (node->getParent()->getNodeType() == ElementType::referenceExpression) {
        auto ref = ((ReferenceExpression*) node->getParent())->reference;
        auto refType = getTypeContext(ref)->getGivenType();
        auto methods = refType->getMethods();

        TargetFunctionCall* tc = nullptr;
        for (auto& m: methods) {
            if (m->getName() == node->functionName) {
                tc = m;
                break;
            }
        }
        for (auto& m: methods) {
            if (m != tc) {
                delete m;
            }
        }

        if (tc) {
            fc->targetFunctionCall = tc;
            fc->setGivenType(tc->getReturnType());
            return;
        }
        auto restMsg = "(" + refType->prettyName() + " has no method " + node->getName() + ")";
        throw CompileFunctionException(("Could not find method to call " + restMsg).c_str(),
                                       node,
                                       this->compilationContext->moduleStack.back()->absolutePath);
    }
    std::vector<RocType*> callTypes;
    for (auto& arg : node->argumentList->arguments) {
        arg->accept(this);
        callTypes.push_back(getReturnType(arg->expression.get()));
    }
    auto tc = findTargetCall(node, callTypes, this->compilationContext);
    if (tc == nullptr) {
        throw CompileFunctionException("Could not find target function call",
                                       node,
                                       this->compilationContext->moduleStack.back()->absolutePath);
    }
    fc->targetFunctionCall = tc;
    fc->setGivenType(tc->getReturnType());
}

void TypeResolver::visit(FunctionVoidReturnTypeNode *functionVoidReturnTypeNode) {
    ((RocTypeNodeContext*) functionVoidReturnTypeNode->typeNode->getContextHolder(TYPE_CONTEXT))->setGivenType(new UnitRocType());
}

void TypeResolver::visit(FunctionNonVoidReturnTypeNode *functionNonVoidReturnTypeNode) {
    functionNonVoidReturnTypeNode->typeNode->accept(this);
    ((RocTypeNodeContext*) functionNonVoidReturnTypeNode->typeNode
    ->getContextHolder(TYPE_CONTEXT))->setGivenType(getReturnType(functionNonVoidReturnTypeNode->typeNode)->clone());
}

void TypeResolver::visit(AddExpr *addExpr) {
    ASTVisitor::visit(addExpr);
    createTypeContext(addExpr);
    auto leftType = getReturnType(addExpr->left.get());
    auto rightType = getReturnType(addExpr->right.get());
    if (leftType->isString() && rightType->isString()) {
        setType(addExpr, leftType->clone());
        return;
    }
    if (leftType->isString() && !rightType->isString()) {
        throw CompileFunctionException("Invalid operation", addExpr, compilationContext);
    }
    if (!leftType->isString() && rightType->isString()) {
        throw CompileFunctionException("Invalid operation", addExpr, compilationContext);
    }

    if (leftType->size() > rightType->size()) {
        setType(addExpr, leftType->clone());
    } else {
        setType(addExpr, rightType->clone());
    }
}

void TypeResolver::visit(SubExpr *subExpr) {
    ASTVisitor::visit(subExpr);
    createTypeContext(subExpr);
    auto leftType = getReturnType(subExpr->left.get());
    auto rightType = getReturnType(subExpr->right.get());
    if (leftType->size() > rightType->size()) {
        setType(subExpr,leftType->clone());
    } else {
        setType(subExpr, rightType->clone());
    }
}

void TypeResolver::visit(DivExpr *divExpr) {
    ASTVisitor::visit(divExpr);
    createTypeContext(divExpr);
    setType(divExpr, new RocFloat64Type());
}

void TypeResolver::visit(MulExpr *mulExpr) {
    ASTVisitor::visit(mulExpr);
    createTypeContext(mulExpr);
    auto leftType = getReturnType(mulExpr->left.get());
    auto rightType = getReturnType(mulExpr->right.get());
    if (leftType->size() > rightType->size()) {
        setType(mulExpr, leftType->clone());
    } else {
        setType(mulExpr, rightType->clone());
    }
}

void TypeResolver::visit(ModExpr *modExpr) {
    ASTVisitor::visit(modExpr);
    createTypeContext(modExpr);
    auto leftType = getReturnType(modExpr->left.get());
    auto rightType = getReturnType(modExpr->right.get());
    if (leftType->size() > rightType->size()) {
        setType(modExpr, leftType->clone());
    } else {
        setType(modExpr, rightType->clone());
    }
}

void TypeResolver::visit(SingleTypeNode *node) {
    createTypeContext(node);
    auto text = node->getText();
    if (text == "i64") {
        setType(node, new RocInt64Type());
    } else if (text == "i32" || text == "int") {
        setType(node, new RocInt32Type());
    } else if (text == "f64") {
        setType(node, new RocFloat64Type());
    } else if (text == "Bool") {
        setType(node, new RocBoolType());
    } else if (text == "Any") {
        setType(node, new RocAnyType());
    } else if (text == "String") {
        setType(node, new RocStringType());
    } else {
        throw CompileTypeException("Could not resolve given type", node, this->compilationContext);
    }
}

void TypeResolver::visit(ArrayTypeNode *arrayTypeNode) {
    auto ctx = createTypeContext(arrayTypeNode);
    arrayTypeNode->singleTypeNode->accept(this);
    auto inner = getReturnType(arrayTypeNode->singleTypeNode.get());
    ctx->setGivenType(new RocArrayType(inner->clone()));
}

void TypeResolver::visit(IntNode *intNode) {
    auto ctx = createTypeContext(intNode);
    ctx->setGivenType(new RocInt32Type());
}

void TypeResolver::visit(LocalAccess *node) {
    createTypeContext(node);
    getTypeContext(node)->setGivenType(getReturnType(node->localVariableRef->parameter->typeNode)->clone());
}

void TypeResolver::visit(IfExpression* ifExpr) {
    ASTVisitor::visit(ifExpr);
    auto* t = getReturnType(ifExpr->expression.get());
    if (!t->isBool()) {
        this->compilationContext->reportProblem(("Expected bool type got: " + t->prettyName()).c_str(), ifExpr->expression.get());
    }
}

void TypeResolver::visit(FunctionDeclaration *fd) {
    fd->body->accept(this);
}

TypeResolver::TypeResolver(CompilationContext *compilationContext) : compilationContext(compilationContext) {
    this->sizeMap.insert({4, TypeEnum::int64Type});
}

RocType::RocType(TypeEnum typeEnum) : typeEnum(typeEnum) {}

void RocAnyType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

void RocRawStringType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

void RocStringType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

void RocBoolType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

void UnitRocType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

RocInt32Type::RocInt32Type() : IntNumberType(TypeEnum::int32Type) {
    this->matchingTraits.push_back(new RocAnyType());
}

llvm::Type * RocInt32Type::getWrapperType(RocLLVMContext *rocLLVMContext) {
    return rocLLVMContext->int32StructType;
}

void RocInt32Type::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

void RocInt64Type::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

void RocFloat64Type::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

RocWrapperType::RocWrapperType(RocType *inner) {
    this->inner = inner;
}

void RocWrapperType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

llvm::Type * RocFloat64Type::getLLVMType(RocLLVMContext *rocLLVMContext) {
    if (rocLLVMContext->float64Type) {
        return rocLLVMContext->float64Type;
    } else {
        rocLLVMContext->float64Type = Type::getDoubleTy(*rocLLVMContext->llvmContext);
        return rocLLVMContext->float64Type;
    }
}


FunctionSignatureResolver::FunctionSignatureResolver(CompilationContext *compilationContext) {
    this->compilationContext = compilationContext;
}

void FunctionSignatureResolver::visit(ModuleDeclaration *moduleDeclaration) {
    compilationContext->pushCompilationNode(moduleDeclaration);
    auto tf = new PredefinedTargetMethodCall(
                new RocInt32Type(),
                "toString",
                "int32ToString",
                {},
                new RocPtrType(new RocRawStringType(-1))
            );
    compilationContext->insertTargetFunction(tf);

    for (auto& f : moduleDeclaration->functions) {
        f->accept(this);
    }
    compilationContext->popCompilationNode();
}

void FunctionSignatureResolver::visit(FunctionDeclaration *functionDeclaration) {
    auto ctx = (RocFunctionContext*) functionDeclaration->addContextHolder(TYPE_CONTEXT, new RocFunctionContext());
    functionDeclaration->parameterList->accept(this);
    TypeResolver typeResolver(this->compilationContext);
    std::vector<RocType*> pt;
    for (auto& p: functionDeclaration->parameterList->parameters) {
        p->accept(&typeResolver);
        pt.push_back(getTypeContext(p->typeNode)->getGivenType());
    }
    ctx->parameterTypes = pt;
    functionDeclaration->functionReturnTypeNode->accept(&typeResolver);
    RocType* returnType = getReturnType(functionDeclaration
            ->functionReturnTypeNode
            ->typeNode);
    ctx->returnType = returnType;
    ctx->target = std::make_unique<FunctionDeclarationTargetWrapper>(functionDeclaration);
    compilationContext->insertTargetFunction(ctx->target.get());
}

static TargetFunctionCall* findTargetCall(FunctionCallNode *functionCallNode,
                           std::vector<RocType*> callTypes,
                           CompilationContext *compilationContext) {

    auto it = compilationContext->builtinFunctionResolver->functions.find(functionCallNode->getName());
    if (it != compilationContext->builtinFunctionResolver->functions.end()) {
        for (std::unique_ptr<BuiltinFunction>& f: it->second) {
            int i = 0;
            bool matched = true;
            for (auto& at: f->getArgumentTypes()) {
                if (!at->matches(callTypes.at(i))) {
                    matched = false;
                    break;
                }
                i++;
            }
            if (matched) {
                return f.get();
            }
        }
    }

    for (auto& f: compilationContext->moduleStack.back()->functions) {
        if (f.get()->getName()->getText() == functionCallNode->getName()) {
            if (f->parameterList->size() == functionCallNode->argumentList->size()) {
                return ((RocFunctionContext*) f->getContextHolder(TYPE_CONTEXT))->target.get();
            }
        }
    }

    if (functionCallNode->getParent()->isDotExpr()) {
        auto refExpr = (ReferenceExpression*) functionCallNode->getParent();
        auto ownerType = getReturnType(refExpr->reference);
        auto typeId = ownerType->typeId();
        auto functionByName = compilationContext->findFunctionByName(
                typeId,
                functionCallNode->functionName,
                functionCallNode->argumentList->size());
        return functionByName;
    }

    //TODO other
    return nullptr;
}




Type* RocAnyType::getLLVMType(RocLLVMContext *rocLLVMContext) {
    if (rocLLVMContext->anyTypeStructType) {
        return rocLLVMContext->anyTypeStructType;
    } else {
        defineAnyType(rocLLVMContext);
        return rocLLVMContext->anyTypeStructType;
    }
}

llvm::Type * UnitRocType::getLLVMType(RocLLVMContext *rocLLVMContext) {
    if (rocLLVMContext->voidType) {
        return rocLLVMContext->voidType;
    } else {
        rocLLVMContext->voidType = Type::getVoidTy(*rocLLVMContext->llvmContext);
        return rocLLVMContext->voidType;
    }
}

llvm::Type * RocBoolType::getLLVMType(RocLLVMContext *rocLLVMContext) {
    if (rocLLVMContext->boolType) {
        return rocLLVMContext->boolType;
    } else {
        rocLLVMContext->boolType = Type::getInt1Ty(*rocLLVMContext->llvmContext);
        return rocLLVMContext->boolType;
    }
}

llvm::Type * RocInt32Type::getLLVMType(RocLLVMContext *rocLLVMContext) {
    if (rocLLVMContext->int32Type) {
        return rocLLVMContext->int32Type;
    } else {
        rocLLVMContext->int32Type = Type::getInt32Ty(*rocLLVMContext->llvmContext);
        return rocLLVMContext->int32Type;
    }
}

std::vector<TargetFunctionCall *> RocInt32Type::getMethods() {
    std::vector<TargetFunctionCall *> result;
    for (auto& t: getMatchingTraits()) {
        for (auto& m: t->getMethods()) {
            result.push_back(m);
        }
    }
    return result;

}

llvm::Type * RocInt64Type::getLLVMType(RocLLVMContext *rocLLVMContext) {
    if (rocLLVMContext->int64Type) {
        return rocLLVMContext->int64Type;
    } else {
        rocLLVMContext->int64Type = Type::getInt64Ty(*rocLLVMContext->llvmContext);
        return rocLLVMContext->int64Type;
    }
}

llvm::Type * RocRawStringType::getLLVMType(RocLLVMContext *rocLLVMContext) {
    return rocLLVMContext->stringRawType;
}

llvm::Type * RocStringType::getLLVMType(RocLLVMContext *rocLLVMContext) {
    return rocLLVMContext->stringType;
}

CompileFunctionException::CompileFunctionException(const char *message,
                                                   Expression *expression,
                                                   std::string filePath): SyntaxException(message,
                                                                                          expression->getStartOffset(),
                                                                                          expression->getEndOffset(),
                                                                                          std::move(filePath)) { }

CompileFunctionException::CompileFunctionException(const char *message,
                                                   Expression *expression,
                                                   CompilationContext *compilationContext):

                                                   SyntaxException(message,
                                                                  expression->getStartOffset(),
                                                                  expression->getEndOffset(),
                                                                  compilationContext->moduleStack
                                                                  .back()->absolutePath) { }

CompileTypeException::CompileTypeException(const char *message,
                                           Expression *expression,
                                           std::string filePath): SyntaxException(message,
                                                                                          expression->getStartOffset(),
                                                                                          expression->getEndOffset(),
                                                                                          std::move(filePath)) { }

CompileTypeException::CompileTypeException(const char *message,
                                           Expression *expression,
                                           CompilationContext *compilationContext):

        SyntaxException(message,
                        expression->getStartOffset(),
                        expression->getEndOffset(),
                        compilationContext->moduleStack.back()->absolutePath) { }

CompileTypeException::CompileTypeException(const char *message,
                                           ASTNode *astNode,
                                           CompilationContext *compilationContext):

        SyntaxException(message,
                        astNode->getStartOffset(),
                        astNode->getEndOffset(),
                        compilationContext->moduleStack.back()->absolutePath) { }

RocPtrType::RocPtrType(RocType *inner) : RocType(TypeEnum::ptrRocType), inner(inner) {}

std::string RocPtrType::toString() {
    return "*";
}

void RocPtrType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

std::string RocPtrType::internalName() {
    return "*";
}

uint64_t RocPtrType::size() {
    return 8;
}

RocType *RocPtrType::clone() {
    return new RocPtrType(this->inner->clone());
}

int RocPtrType::typeId() {
    return 100;
}

std::string RocPtrType::prettyName() {
    return "*" + inner->prettyName();
}

Type *RocPtrType::getLLVMType(RocLLVMContext *rocLLVMContext) {
    return this->inner->getLLVMType(rocLLVMContext)->getPointerTo();
}

RocArrayType::RocArrayType(RocType *inner) : RocType(TypeEnum::arrayRocType), inner(inner) {}

std::string RocArrayType::toString() {
    return "[]" + inner->toString();
}

void RocArrayType::visit(RocTypeVisitor *visitor) {
    visitor->visit(this);
}

std::string RocArrayType::internalName() {
    return "[]" + inner->internalName();
}

uint64_t RocArrayType::size() {
    return 8;
}

RocType *RocArrayType::clone() {
    return new RocPtrType(this->inner->clone());
}

int RocArrayType::typeId() {
    return 200;
}

std::string RocArrayType::prettyName() {
    return "[]" + inner->prettyName();
}

Type *RocArrayType::getLLVMType(RocLLVMContext *rocLLVMContext) {
    return this->inner->getLLVMType(rocLLVMContext)->getPointerTo()->getPointerTo();
}


RocType * ArrayTypeResolver::resolve(ArrayCreateExpr *arrayCreateExpr, CompilationContext *compilationContext) {
    if (arrayCreateExpr->arguments.empty()) {
        throw CompileTypeException("Can't resolve type of emtpy array", arrayCreateExpr, compilationContext);
    }
    RocType *type = nullptr;
    for (auto& arg: arrayCreateExpr->arguments) {
        auto* givenType = getReturnType(arg.get());
        if (type == nullptr) {
            type = givenType;
        } else {

        }
    }
    return type->clone();
}

std::vector<TargetFunctionCall*> RocAnyType::getMethods() {
    std::vector<TargetFunctionCall*> result;
    auto ptrType = new RocPtrType(new RocStringType());
    result.push_back(new PredefinedTargetMethodCall(this, "toString", "Any.toString.0", {}, ptrType));
    return result;
}