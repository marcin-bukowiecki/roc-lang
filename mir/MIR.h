//
// Created by Marcin on 18.03.2021.
//
#pragma once
#ifndef ROC_LANG_MIR_H
#define ROC_LANG_MIR_H

#include <utility>
#include <vector>
#include <memory>
#include <string>
#include "../compiler/Types.h"

namespace llvm {
    class Value;

    class Function;
}

class MIRVisitor;

class MIRElse;

namespace roc {
    enum AllocationSpace {
        HeapAllocation,
        StackAllocation,
    };
}

class MIRValue {
public:
    MIRValue *parent = nullptr;

    MIRValue() = default;

    virtual void accept(MIRVisitor *mirVisitor);

    virtual std::string getText() {
        return "";
    }

    virtual RocType *getType() {
        return nullptr;
    }

    //TODO for future support
    virtual RocType *getGenericType() {
        return getType();
    }

    virtual bool isReturn() {
        return false;
    }

    virtual std::vector<MIRValue *> getChildren() {
        std::vector<MIRValue *> result;
        return result;
    }
};

class MIRRawString : public MIRValue {
public:
    std::string value;
    RocRawStringType* type;

    explicit MIRRawString(std::string value);

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return value;
    }

    RocType *getType() override {
        return type;
    }
};

class MIRTypeDecl : public MIRValue {
public:
    RocType *rocType;

    explicit MIRTypeDecl(RocType *rocType) {
        this->rocType = rocType;
    }

    virtual ~MIRTypeDecl() {
        delete rocType;
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return rocType->toString();
    }
};

class MIRFunctionParameter : public MIRValue {
public:
    std::string name;
    MIRTypeDecl *type;

    MIRFunctionParameter(std::string name, MIRTypeDecl *typeDecl) {
        this->name = std::move(name);
        this->type = typeDecl;
        this->type->parent = this;
    }

    virtual ~MIRFunctionParameter() {
        delete type;
    }

    std::string getText() override {
        return name + " " + type->getText();
    }
};

class MIRBlock : public MIRValue {
public:
    std::string name;
    std::vector<MIRValue *> values;
    bool blockInitialized = false;

    MIRBlock(std::string name, std::vector<MIRValue *> values);

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        std::string acc;
        for (auto &v: values) {
            acc.append(v->getText());
        }
        return name + "\n" + acc + "\n";
    }
};

class MIRCondition : public MIRValue {
public:
    MIRValue* expr;

    MIRCondition(MIRValue *expr);

    ~MIRCondition() {
        delete expr;
    }

    std::vector<MIRValue *> getChildren() override {
        return {expr};
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return expr->getText();
    }

    RocType *getType() override {
        return expr->getType();
    }
};

class MIRLabel {
public:
    int id;
    std::string name;

    MIRLabel(int id, const std::string &name);
};

class MIRIf : public MIRValue {
public:
    MIRLabel* startLabel = nullptr;
    MIRCondition *condition;
    MIRBlock *block;
    UnitRocType *type;
    MIRLabel* endLabel = nullptr;
    MIRElse* elseBlock = nullptr;
    bool jumpOver = false;

    MIRIf(MIRCondition *condition, MIRBlock *block);

    ~MIRIf();

    std::vector<MIRValue *> getChildren() override {
        return {condition, block};
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return condition->getText() + block->getText();
    }

    RocType *getType() override {
        return type;
    }

    bool hasNextBlock() {
        return elseBlock != nullptr;
    }
};


class MIRElse : public MIRValue {
public:
    MIRBlock *block = nullptr;
    MIRIf* ifBlock = nullptr;
    MIRLabel* endLabel = nullptr;

    MIRElse(MIRBlock *block, MIRIf *ifBlock) : block(block), ifBlock(ifBlock) {}

    ~MIRElse() {
        delete block;
        delete ifBlock;
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::vector<MIRValue *> getChildren() override {
        std::vector<MIRValue*> result;
        if (block != nullptr) {
            result.push_back(block);
        }
        if (ifBlock != nullptr) {
            result.push_back(ifBlock);
        }
        return result;
    }

    bool hasNextBlock() {
        return ifBlock != nullptr && ifBlock->hasNextBlock();
    }
};

class MIRTrue : public MIRValue {
public:
    RocBoolType* type;

    explicit MIRTrue() {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return "true";
    }

    RocType *getType() override {
        return type;
    }
};

class MIRFalse : public MIRValue {
public:
    RocBoolType* type;

    explicit MIRFalse() {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return "false";
    }

    RocType *getType() override {
        return type;
    }
};

class MIRConstantInt : public MIRValue {
public:
    RocInt32Type* type;
    int value;

    explicit MIRConstantInt(int value) {
        this->value = value;
        this->type = new RocInt32Type();
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return std::to_string(value);
    }

    RocType *getType() override {
        return type;
    }
};

class MIRLocalVariableAccess : public MIRValue {
public:
    std::string name;
    int index;
    RocType* type;

    MIRLocalVariableAccess(std::string name, int index, RocType* type) {
        this->name = std::move(name);
        this->index = index;
        this->type = type;
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return name;
    }

    RocType *getType() override {
        return type;
    }
};

class MIRReturnValue : public MIRValue {
public:
    MIRValue *value;

    explicit MIRReturnValue(MIRValue *value);

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return "ret " + value->getText();
    }

    RocType *getType() override {
        return value->getType();
    }

    bool isReturn() override {
        return true;
    }
};

class MIRReturnVoidValue : public MIRValue {
public:
    RocType* type;

    MIRReturnVoidValue() {
        this->type = new UnitRocType();
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::string getText() override {
        return "ret";
    }

    RocType *getType() override {
        return type;
    }

    bool isReturn() override {
        return true;
    }
};

class MIRFunction : public MIRValue, public TargetFunctionCall {
public:
    std::string name;
    std::vector<MIRFunctionParameter *> parameters;
    MIRBlock *body;
    MIRTypeDecl *returnTypeDecl;
    FunctionDeclaration *functionDeclaration;

    llvm::Function *llvmFunction{};
    std::map<int, llvm::Value *> localsMap;

    MIRFunction(std::string name,
                std::vector<MIRFunctionParameter *> parameters,
                MIRTypeDecl *returnTypeDecl,
                MIRBlock *body) {

        this->name = std::move(name);
        this->parameters = std::move(parameters);

        this->body = body;
        this->body->parent = this;
        for (auto &p: this->parameters) {
            p->parent = this;
        }

        this->returnTypeDecl = returnTypeDecl;
        this->returnTypeDecl->parent = this;
    }

    void accept(MIRVisitor *mirVisitor) override;

    std::vector<RocType *> getArgumentTypes() override;

    std::string getName() override;

    RocType *getReturnType() override {
        return returnTypeDecl->rocType;
    }
};

/**
 * static call bar()
 */
class MIRFunctionCall : public MIRValue {
protected:
    TargetFunctionCall *targetCall{};
public:
    std::string name{};
    std::vector<MIRValue *> arguments{};

    MIRFunctionCall() = default;

    MIRFunctionCall(std::string name,
                    std::vector<MIRValue *> arguments,
                    TargetFunctionCall *targetCall) {

        this->name = std::move(name);
        this->arguments = std::move(arguments);
        this->targetCall = targetCall;
    }

    TargetFunctionCall* getTargetCall() {
        return targetCall;
    }

    virtual std::string getName() {
        return targetCall->getName();
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return targetCall->getReturnType();
    }

    std::vector<RocType *> getGivenArgumentTypes() {
        std::vector<RocType *> result;
        for (auto &arg: arguments) {
            result.push_back(arg->getType());
        }
        return result;
    }
};

/**
 * static call bar()
 */
class MIRCCall : public MIRFunctionCall {
public:
    RocType* returnType{};

    MIRCCall(std::string name,
             std::vector<MIRValue *> arguments,
             RocType* returnType): MIRFunctionCall(name, std::move(arguments), nullptr) {

        this->returnType = returnType;
    }

    std::string getName() override {
        return name;
    }

    RocType * getType() override {
        return returnType;
    }

    void accept(MIRVisitor *mirVisitor) override;
};

/**
 * instance call foo.bar()
 */
class MIRFunctionInstanceCall : public MIRFunctionCall {
public:
    MIRValue *caller;

    MIRFunctionInstanceCall(MIRValue *caller,
                            std::string name,
                            std::vector<MIRValue *> arguments,
                            TargetFunctionCall *targetCall) {

        this->caller = caller;
        this->name = std::move(name);
        this->arguments = std::move(arguments);
        this->targetCall = targetCall;
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRBinOpBase : public MIRValue {
public:
    MIRValue *left;
    MIRValue *right;
    RocType *type;
    std::string symbol;

    MIRBinOpBase(MIRValue *left,
                 MIRValue *right,
                 std::string symbol) : left(std::move(left)), right(std::move(right)), symbol(std::move(symbol)) {
        this->left->parent = this;
        this->right->parent = this;
    }

    ~MIRBinOpBase() {
        delete type;
    }

    virtual void accept(MIRVisitor *mirVisitor);

    std::vector<MIRValue *> getChildren() override {
        return {left, right};
    }

    std::string getText() override {
        return left->getText() + symbol + right->getText();
    }

    RocType *getType() override {
        return type;
    }
};

class MIRMod : public MIRBinOpBase {
public:
    MIRMod(MIRValue *left,
           MIRValue *right,
           RocType *type) : MIRBinOpBase(std::move(left), std::move(right), "%") {
        this->type = type;
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt64Mod : public MIRBinOpBase {
public:
    MIRInt64Mod(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "%") {
        this->type = new RocInt64Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Mod : public MIRBinOpBase {
public:
    MIRInt32Mod(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "%") {
        this->type = new RocInt32Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt64Add : public MIRBinOpBase {
public:
    MIRInt64Add(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "+") {
        this->type = new RocInt64Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Add : public MIRBinOpBase {
public:
    MIRInt32Add(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "+") {
        this->type = new RocInt32Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Sub : public MIRBinOpBase {
public:
    MIRInt32Sub(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "-") {
        this->type = new RocInt32Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt64Sub : public MIRBinOpBase {
public:
    MIRInt64Sub(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "-") {
        this->type = new RocInt32Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Div : public MIRBinOpBase {
public:
    MIRInt32Div(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "/") {
        this->type = new RocFloat64Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt64Div : public MIRBinOpBase {
public:
    MIRInt64Div(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "/") {
        this->type = new RocFloat64Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Mul : public MIRBinOpBase {
public:
    MIRInt32Mul(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "*") {
        this->type = new RocInt32Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt64Mul : public MIRBinOpBase {
public:
    MIRInt64Mul(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left),
                                                                std::move(right),
                                                                "*") {
        this->type = new RocInt64Type();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

//
// Logical operators
//

class MIRAnd : public MIRBinOpBase {
public:
    MIRAnd(MIRValue *left,
           MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), "and") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIROr : public MIRBinOpBase {
public:
    MIROr(MIRValue *left,
          MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), "or") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Eq : public MIRBinOpBase {
public:
    MIRInt32Eq(MIRValue *left,
               MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), "==") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32NotEq : public MIRBinOpBase {
public:
    MIRInt32NotEq(MIRValue *left, MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), "!=") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Gt : public MIRBinOpBase {
public:
    MIRInt32Gt(MIRValue *left,
               MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), ">") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Lt : public MIRBinOpBase {
public:
    MIRInt32Lt(MIRValue *left,
               MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), "<") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Le : public MIRBinOpBase {
public:
    MIRInt32Le(MIRValue *left,
               MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), "<=") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRInt32Ge : public MIRBinOpBase {
public:
    MIRInt32Ge(MIRValue *left,
               MIRValue *right) : MIRBinOpBase(std::move(left), std::move(right), ">=") {
        this->type = new RocBoolType();
    }

    void accept(MIRVisitor *mirVisitor) override;
};

class MIRStringToRaw : public MIRValue {
public:
    RocRawStringType* type;
    MIRValue *expr;

    explicit MIRStringToRaw(MIRValue *expr) {
        this->expr = std::move(expr);
        this->expr->parent = this;
        this->type = new RocRawStringType(-1);
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return type;
    }
};

class MIRToPtr : public MIRValue {
public:
    RocPtrType* type;
    MIRValue *expr;

    explicit MIRToPtr(MIRValue *expr) {
        this->expr = expr;
        this->expr->parent = this;
        this->type = new RocPtrType(expr->getGenericType()->clone());
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return type;
    }
};

class MIRToWrapper : public MIRValue {
public:
    RocType* type;
    MIRValue *expr;

    explicit MIRToWrapper(MIRValue *expr) {
        this->expr = expr;
        this->expr->parent = this;
        this->type = new RocWrapperType(expr->getGenericType()->clone());
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return type;
    }
};

class MIRCastTo : public MIRValue {
public:
    RocType* targetType;
    MIRValue *from;

    explicit MIRCastTo(MIRValue *from, RocType* targetType) {
        this->from = from;
        this->from->parent = this;
        this->targetType = targetType;
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return targetType;
    }
};

class MIRInt32Array : public MIRValue {
public:
    std::unique_ptr<RocArrayType> at;
    std::vector<MIRValue *> elements;
    roc::AllocationSpace allocationSpace = roc::AllocationSpace::StackAllocation;

    explicit MIRInt32Array(std::vector<MIRValue *> elements) {
        this->elements = std::move(elements);
        this->at = std::make_unique<RocArrayType>(new RocInt32Type());
        for (auto &e: this->elements) {
            e->parent = this;
        }
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return at.get();
    }
};

class MIRInt32ArraySet : public MIRValue {
public:
    MIRValue *ref;
    MIRValue *index;
    MIRValue *value;
    std::unique_ptr<RocType> type;

    explicit MIRInt32ArraySet(MIRValue *ref,
                              MIRValue *index,
                              MIRValue *value) {
        this->ref = std::move(ref);
        this->index = std::move(index);
        this->value = std::move(value);
        this->ref->parent = this;
        this->index->parent = this;
        this->value->parent = this;
        this->type = std::unique_ptr<RocInt32Type>();
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return type.get();
    }
};

class MIRInt32ArrayGet : public MIRValue {
public:
    MIRValue *ref;
    MIRValue *index;
    std::unique_ptr<RocType> type;

    explicit MIRInt32ArrayGet(MIRValue *ref, MIRValue *index) {
        this->ref = std::move(ref);
        this->index = std::move(index);
        this->ref->parent = this;
        this->index->parent = this;
        this->type = std::unique_ptr<RocInt32Type>();
    }

    void accept(MIRVisitor *mirVisitor) override;

    RocType *getType() override {
        return type.get();
    }
};

class MIRModule {
public:
    std::string name;
    std::vector<std::unique_ptr<MIRFunction>> functions;
    std::shared_ptr<ModuleDeclaration> moduleDeclaration;

    MIRModule(std::string name, std::vector<std::unique_ptr<MIRFunction>> functions) {
        this->name = std::move(name);
        this->functions = std::move(functions);
    }

    void visit(MIRVisitor *mirVisitor);
};

class ToMIRVisitor : public ASTVisitor {
public:
    std::vector<MIRValue *> valueStack;
    std::vector<std::unique_ptr<MIRFunction>> functionStack;
    std::unique_ptr<MIRModule> mirModule;

    void visit(ModuleDeclaration *moduleDeclaration) override;

    void visit(StaticBlock *staticBlock) override;

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(StringNode *) override;

    void visit(FunctionCallNode *) override;

    void visit(ReturnExpression *returnExpression) override;

    void visit(ModExpr *modExpr) override;

    void visit(AddExpr *addExpr) override;

    void visit(SubExpr *subExpr) override;

    void visit(DivExpr *divExpr) override;

    void visit(MulExpr *mulExpr) override;

    void visit(IntNode *intNode) override;

    void visit(LocalAccess *node) override;

    void visit(ArrayCreateExpr *arrayCreateExpr) override;

    void visit(AndExpr *andExpr) override;

    void visit(OrExpr *orExpr) override;

    void visit(EqualOpExpr *equalOpExpr) override;

    void visit(NotEqualOpExpr *notEqualOpExpr) override;

    void visit(GreaterExpr *greaterExpr) override;

    void visit(LesserExpr *lesserExpr) override;

    void visit(GreaterOrEqualExpr *greaterOrEqualExpr) override;

    void visit(LesserOrEqualExpr *lesserOrEqualExpr) override;

    void visit(IfExpression *ifExpression) override;

    void visit(CodeBlock *codeBlock) override;

    void visit(TrueExpr *trueExpr) override;

    void visit(FalseExpr *falseExpr) override;
};

class MIRVisitor {
public:

    virtual void visit(MIRToPtr *mirToPtr) {
        mirToPtr->expr->accept(this);
    }

    virtual void visit(MIRToWrapper *mirToWrapper) {
        mirToWrapper->expr->accept(this);
    }

    virtual void visit(MIRValue *mirValue) {
        for (auto &ch: mirValue->getChildren()) {
            ch->accept(this);
        }
    };

    virtual void visitIf(MIRIf *mirIf) {
        MIRVisitor::visit((MIRValue *) mirIf);
    };

    virtual void visitElse(MIRElse *mirElse) {
        MIRVisitor::visit((MIRValue *) mirElse);
    };

    virtual void visit(MIRReturnValue *mirReturnValue) {
        mirReturnValue->value->accept(this);
    };

    virtual void visit(MIRReturnVoidValue *mirReturnValue) { };

    virtual void visit(MIRTypeDecl *mirTypeDecl) { };

    virtual void visit(MIRModule *mirModule) {
        for (auto &f: mirModule->functions) {
            f->accept(this);
        }
    };

    virtual void visit(MIRFunction *mirFunction) {
        for (auto &expr: mirFunction->body->values) {
            expr->accept(this);
        }
    };

    virtual void visit(MIRBlock *mirBlock) {};

    virtual void visit(MIRFunctionParameter *mirFunctionParameter) {};

    virtual void visit(MIRCCall *mircCall) {};

    virtual void visit(MIRFunctionCall *mirFunctionCall) {};

    virtual void visit(MIRFunctionInstanceCall *mirFunctionCall) {};

    virtual void visit(MIRRawString *mirRawString) {};

    virtual void visit(MIRStringToRaw *mirStringToRaw) {};

    virtual void visit(MIRCastTo *mirCastTo) {};

    virtual void visit(MIRConstantInt *mirConstantInt) {};

    virtual void visit(MIRMod *mirMod) {
        mirMod->left->accept(this);
        mirMod->right->accept(this);
    };

    virtual void visit(MIRInt32Mod *mirInt32Mod) {
        mirInt32Mod->left->accept(this);
        mirInt32Mod->right->accept(this);
    };

    virtual void visit(MIRInt64Mod *mirInt64Mod) {
        mirInt64Mod->left->accept(this);
        mirInt64Mod->right->accept(this);
    };

    virtual void visit(MIRInt64Add *mirInt64Add) {
        mirInt64Add->left->accept(this);
        mirInt64Add->right->accept(this);
    };

    virtual void visit(MIRInt32Add *mirInt32Add) {
        mirInt32Add->left->accept(this);
        mirInt32Add->right->accept(this);
    };

    virtual void visit(MIRInt64Sub *mirInt64Sub) {
        mirInt64Sub->left->accept(this);
        mirInt64Sub->right->accept(this);
    };

    virtual void visit(MIRInt32Sub *mirInt32Sub) {
        mirInt32Sub->left->accept(this);
        mirInt32Sub->right->accept(this);
    };

    virtual void visit(MIRInt32Div *mirInt32Div) {
        mirInt32Div->left->accept(this);
        mirInt32Div->right->accept(this);
    };

    virtual void visit(MIRInt64Div *mirInt64Div) {
        mirInt64Div->left->accept(this);
        mirInt64Div->right->accept(this);
    };

    virtual void visit(MIRInt32Mul *mirInt32Mul) {
        mirInt32Mul->left->accept(this);
        mirInt32Mul->right->accept(this);
    };

    virtual void visit(MIRInt64Mul *mirInt64Mul) {
        mirInt64Mul->left->accept(this);
        mirInt64Mul->right->accept(this);
    };

    virtual void visit(MIRLocalVariableAccess *la) {}

    virtual void visit(MIRInt32Array *mirInt32Array) {
        for (auto &e: mirInt32Array->elements) {
            e->accept(this);
        }
    }

    virtual void visit(MIRInt32ArrayGet *ag) {
        ag->ref->accept(this);
        ag->index->accept(this);
    }

    virtual void visit(MIRInt32ArraySet *as) {
        as->ref->accept(this);
        as->index->accept(this);
        as->value->accept(this);
    }


    virtual void visit(MIRAnd *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visit(MIROr *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visit(MIRInt32Ge *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visit(MIRInt32Gt *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visit(MIRInt32Lt *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visit(MIRInt32Le *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visit(MIRInt32Eq *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visit(MIRInt32NotEq *node) {
        node->left->accept(this);
        node->right->accept(this);
    }

    virtual void visitTrue(MIRTrue *node) { }

    virtual void visitFalse(MIRFalse *node) { }
};

class SmartTypeCaster : public MIRVisitor {
public:
    void visit(MIRFunctionCall *node) override;

    void visit(MIRFunctionInstanceCall *mirFunctionCall) override;
};

#endif //ROC_LANG_MIR_H
