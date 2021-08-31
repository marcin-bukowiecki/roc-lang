//
// Created by Marcin on 04.03.2021.
//
#pragma once
#ifndef ROC_LANG_TYPES_H
#define ROC_LANG_TYPES_H

#include <utility>
#include <vector>
#include <string>
#include "../parser/AST.h"
#include "../parser/Parser.h"
#include "RocCompiler.h"

static std::string TYPE_CONTEXT = "typeContext";

static int rocRawStringTypeId = 2;
static int rocInt32TypeId = 4;
static int float32TypeId = 10;
static int float64TypeId = 11;

static int typeIdStart = 1000;

class RocTypeVisitor;
class RocLLVMContext;
class RocPtrType;

namespace llvm {
    class Type;
};

static int rocTypeCtx = 0;
static int rocFunctionCallCtx = 1;
static int rocFunctionTypeCtx = 2;

/**
 * Enum for Roc types
 */
enum TypeEnum {
    unitType,
    anyType,

    stringRawType,
    stringType,

    boolType,

    int32Type,
    int64Type,

    float32Type,
    float64Type,

    ptrRocType,
    arrayRocType,
};

/**
 * Base class for types
 */
class RocType {
public:
    std::vector<RocType*> matchingTraits{};
    TypeEnum typeEnum;
    bool varargs = false;

    explicit RocType(TypeEnum typeEnum);

    virtual ~RocType() {
        for (auto& t: matchingTraits) {
            delete t;
        }
    };

    virtual llvm::Type* getWrapperType(RocLLVMContext *rocLlvmContext) {
        throw "Unsupported operation";
    };

    virtual std::string toString() = 0;

    virtual void visit(RocTypeVisitor *visitor) = 0;

    virtual std::string internalName() = 0;

    virtual bool matches(RocType *other) {
        if (other->isAny()) {
            return true;
        }
        return this->typeEnum = other->typeEnum;
    }

    virtual uint64_t size() = 0;

    virtual RocType* clone() = 0;

    virtual int typeId() = 0;

    virtual std::string prettyName() = 0;

    virtual llvm::Type* getLLVMType(RocLLVMContext *rocLlvmContext) = 0;

    virtual bool isNumber() {
        return false;
    }

    virtual bool isString() {
        return false;
    }

    virtual bool isRawString() {
        return false;
    }

    virtual bool isAny() {
        return false;
    }

    virtual bool isStruct() {
        return false;
    }

    virtual bool isPrimitive() {
        return false;
    }

    virtual bool isBool() {
        return false;
    }

    virtual bool isPtr() {
        return false;
    }

    virtual std::vector<TargetFunctionCall*> getMethods() {
        return {};
    }

    virtual RocPtrType* getPtrType();

    virtual std::vector<RocType *> getMatchingTraits() {
        return matchingTraits;
    }

    virtual bool equals(RocType* other) {
        return internalName() == other->internalName();
    }
};

/**
 * Any type
 */
class RocAnyType : public RocType {
public:
    explicit RocAnyType(bool varargs = false) : RocType(TypeEnum::anyType) {
        this->varargs = varargs;
    }

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "any";
    }

    std::string internalName() override {
        return "a";
    }

    uint64_t size() override {
        return 0;
    }

    RocType * clone() override {
        return new RocAnyType(this->varargs);
    }

    int typeId() override {
        return 1;
    }

    std::string prettyName() override {
        return "Any";
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLlvmContext) override;

    bool isAny() override {
        return true;
    }

    bool isStruct() override {
        return true;
    }

    bool isPtr() override {
        return true;
    }

    std::vector<TargetFunctionCall*> getMethods();
};

/**
 * Base class for numbers
 */
class RocNumberType : public RocType {
public:
    explicit RocNumberType(TypeEnum typeEnum) : RocType(typeEnum) { }

    bool isNumber() override {
        return true;
    }

    bool isPrimitive() override {
        return true;
    }
};

/**
 * Base class for int numbers
 */
class IntNumberType : public RocNumberType {
public:

    explicit IntNumberType(TypeEnum typeEnum) : RocNumberType(typeEnum) { }
    };

/**
 * String raw type i.e. "foo bar"
 */
class RocRawStringType : public RocType {
public:
    uint64_t length = 0;

    explicit RocRawStringType(uint64_t length) : RocType(TypeEnum::stringRawType) {
        this->length = length;
    }

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "str";
    }

    std::string internalName() override {
        return "str";
    }

    uint64_t size() override {
        return length;
    }

    RocType * clone() override {
        return new RocRawStringType(this->length);
    }

    int typeId() override {
        return rocRawStringTypeId;
    }

    std::string prettyName() override {
        return "StringRaw";
    }

    bool isString() override {
        return true;
    }

    bool isRawString() override {
        return true;
    }

    bool isPrimitive() override {
        return true;
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLlvmContext) override;
};

/**
 * String type i.e. "foo bar"
 */
class RocStringType : public RocType {
public:
    explicit RocStringType() : RocType(TypeEnum::stringType) { }

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "String";
    }

    std::string internalName() override {
        return "string";
    }

    uint64_t size() override {
        return 8;
    }

    RocType * clone() override {
        return new RocStringType();
    }

    int typeId() override {
        return 3;
    }

    std::string prettyName() override {
        return "String";
    }

    bool isString() override {
        return true;
    }

    bool isStruct() override {
        return true;
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLlvmContext) override;
};

/**
 * Unit type or just void/nothing
 */
class UnitRocType : public RocType {
public:
    UnitRocType() : RocType(TypeEnum::unitType) {}

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "Unit";
    }

    std::string internalName() override {
        return "V";
    }

    uint64_t size() override {
        return 0;
    }

    RocType * clone() override {
        return new UnitRocType();
    }

    int typeId() override {
        return 0;
    }

    std::string prettyName() override {
        return "Unit";
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLlvmContext) override;
};

/**
 * Bool type
 */
class RocBoolType : public RocAnyType {
public:
    RocBoolType() : RocAnyType(TypeEnum::boolType) {}

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "Bool";
    }

    std::string internalName() override {
        return "Z";
    }

    uint64_t size() override {
        return 1;
    }

    RocType * clone() override {
        return new RocBoolType();
    }

    int typeId() override {
        return 21;
    }

    std::string prettyName() override {
        return "Bool";
    }

    bool isStruct() override {
        return false;
    }

    bool isBool() override {
        return true;
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLlvmContext) override;
};

/**
 * Int32 type
 */
class RocInt32Type : public IntNumberType {
public:
    RocInt32Type();

    llvm::Type* getWrapperType(RocLLVMContext *rocLlvmContext) override;

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "Int32";
    }

    std::string internalName() override {
        return "i32";
    }

    uint64_t size() override {
        return 4;
    }

    RocType * clone() override {
        return new RocInt32Type();
    }

    int typeId() override {
        return rocInt32TypeId;
    }

    std::string prettyName() override {
        return "Int32";
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLlvmContext) override;

    std::vector<TargetFunctionCall*> getMethods();
};

/**
 * Int64 type
 */
class RocInt64Type : public IntNumberType {
public:
    RocInt64Type() : IntNumberType(TypeEnum::int64Type) {}

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "Int64";
    }

    std::string internalName() override {
        return "i";
    }

    static std::unique_ptr<RocInt32Type> create() {
        return std::make_unique<RocInt32Type>();
    }

    uint64_t size() override {
        return 8;
    }

    RocType * clone() override {
        return new RocInt64Type();
    }

    int typeId() override {
        return 5;
    }

    std::string prettyName() override {
        return "Int64";
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLLVMContext) override;
};

/**
 * Float64 type
 */
class RocFloat64Type : public IntNumberType {
public:
    RocFloat64Type() : IntNumberType(TypeEnum::float64Type) {}

    void visit(RocTypeVisitor *visitor) override;

    std::string toString() override {
        return "Float64";
    }

    std::string internalName() override {
        return "f64";
    }

    uint64_t size() override {
        return 8;
    }

    RocType * clone() override {
        return new RocFloat64Type();
    }

    int typeId() override {
        return float64TypeId;
    }

    std::string prettyName() override {
        return "Float64";
    }

    llvm::Type* getLLVMType(RocLLVMContext *rocLLVMContext) override;
};

/**
 * Type to wrap primitive types and pass to i.e. foo(*Any)
 */
class RocWrapperType : public RocAnyType {
public:
    RocType* inner;

    explicit RocWrapperType(RocType *inner);

    std::string toString() override {
        return inner->toString();
    }

    void visit(RocTypeVisitor *visitor) override;

    std::string internalName() override {
        return inner->internalName();
    }

    uint64_t size() override {
        return inner->size();
    }

    RocType *clone() override {
        return new RocWrapperType(inner->clone());
    }

    int typeId() override {
        return inner->typeId();
    }

    std::string prettyName() override {
        return inner->prettyName();
    };

    llvm::Type *getLLVMType(RocLLVMContext *rocLLvmContext) override {
        return inner->getWrapperType(rocLLvmContext);
    }

    bool isPtr() override {
        return false;
    }

    bool isPrimitive() override {
        return false;
    }

    bool isStruct() override {
        return true;
    }

    bool isNumber() override {
        return true;
    }
};

class RocPtrType : public RocType {
public:
    RocType* inner;

    explicit RocPtrType(RocType *inner);

    std::string toString() override;

    void visit(RocTypeVisitor *visitor) override;

    std::string internalName() override;

    uint64_t size() override;

    RocType *clone() override;

    int typeId() override;

    std::string prettyName() override;

    llvm::Type *getLLVMType(RocLLVMContext *rocLlvmContext) override;

    bool isPtr() override {
        return true;
    }
};

/**
 * i.e. []Int32 will be represented as **int, []Foo will be represented as **Foo
 */
class RocArrayType : public RocType {
public:
    RocType* inner;

    explicit RocArrayType(RocType *inner);

    ~RocArrayType() override {
        delete inner;
    }

    std::string toString() override;

    void visit(RocTypeVisitor *visitor) override;

    std::string internalName() override;

    uint64_t size() override;

    RocType *clone() override;

    int typeId() override;

    std::string prettyName() override;

    llvm::Type *getLLVMType(RocLLVMContext *rocLlvmContext) override;
};

/**
 * Context holding type information for TypedASTNode (see in /parser/AST.h file)
 */
class RocTypeNodeContext : public NodeContextHolder {
private:
    RocType* givenType{};
    RocType* givenGenericType{};

public:
    RocTypeNodeContext() = default;

    virtual ~RocTypeNodeContext() {
        delete givenType;
        delete givenGenericType;
    }

    RocType *getGivenType() const {
        return givenType;
    }

    void setGivenType(RocType *givenType) {
        this->givenType = givenType;
    }

    RocType *getGivenGenericType() const {
        return givenGenericType;
    }

    void setGivenGenericType(RocType *givenGenericType) {
        this->givenGenericType = givenGenericType;
    }

    virtual int contextId() {
        return rocTypeCtx;
    }
};

class RocFunctionCallContext : public RocTypeNodeContext {
public:
    TargetFunctionCall* targetFunctionCall;

    int contextId() override {
        return rocFunctionCallCtx;
    }
};

class RocFunctionContext : public RocTypeNodeContext {
public:
    std::vector<RocType*> parameterTypes;
    RocType* returnType;
    std::unique_ptr<FunctionDeclarationTargetWrapper> target;

    int contextId() override {
        return rocFunctionTypeCtx;
    }
};

/**
 * Visitor for resolving types
 */
class TypeResolver : public ASTVisitor {
public:
    CompilationContext* compilationContext;
    std::map<int, TypeEnum> sizeMap;

    explicit TypeResolver(CompilationContext *compilationContext);

    void visit(ReferenceExpression *referenceExpression) override;

    void visit(ModuleDeclaration *moduleNode) override;

    void visit(ArrayCreateExpr *arrayCreateExpr) override;

    void visit(StaticBlock *staticBlock) override;

    void visit(FunctionCallNode *node) override;

    void visit(StringNode *stringNode) override;

    void visit(Parameter *parameter) override;

    void visit(FunctionDeclaration *fd) override;

    void visit(FunctionVoidReturnTypeNode *functionVoidReturnTypeNode) override;

    void visit(FunctionNonVoidReturnTypeNode *functionNonVoidReturnTypeNode) override;

    void visit(AddExpr *addExpr) override;

    void visit(SubExpr *subExpr) override;

    void visit(DivExpr *divExpr) override;

    void visit(MulExpr *mulExpr) override;

    void visit(ModExpr *modExpr) override;

    void visit(SingleTypeNode *singleTypeNode) override;

    void visit(ArrayTypeNode *arrayTypeNode) override;

    void visit(IntNode *intNode) override;

    void visit(LocalAccess *node) override;

    void visit(IfExpression *ifExpression) override;

    void visit(EqualOpExpr *opExpr) override;
};

/**
 * Visitor for resolving function signatures
 */
class FunctionSignatureResolver : public ASTVisitor {
public:
    CompilationContext* compilationContext;

    explicit FunctionSignatureResolver(CompilationContext *compilationContext);

    void visit(ModuleDeclaration *moduleDeclaration) override;

    void visit(FunctionDeclaration *functionDeclaration) override;
};

/**
 * Base Roc Type Visitor i.e. map to LLVM types
 */
class RocTypeVisitor {
public:

    virtual void visit(RocAnyType *type) { };

    virtual void visit(RocPtrType *type) { };

    virtual void visit(RocRawStringType *type) { };

    virtual void visit(RocStringType *type) { };

    virtual void visit(UnitRocType *type) { };

    virtual void visit(RocBoolType *type) { };

    virtual void visit(RocInt32Type *type) { };

    virtual void visit(RocInt64Type *type) { };

    virtual void visit(RocFloat64Type *type) { };

    virtual void visit(RocArrayType *type) { };
};

/**
 * abstract class describing function call site
 */
class TargetFunctionCall {
public:

    virtual ~TargetFunctionCall() = default;

    virtual std::string getName() = 0;

    virtual std::string getRealName() {
        return getName();
    }

    virtual std::vector<RocType*> getArgumentTypes() = 0;

    virtual RocType* getReturnType() = 0;
};

class CompileFunctionException : public SyntaxException {
public:
    CompileFunctionException(const char *message,
                             Expression *expression,
                             std::string filePath);

    CompileFunctionException(const char *message,
                             Expression *expression,
                             CompilationContext *compilationContext);
};

class CompileTypeException : public SyntaxException {
public:
    CompileTypeException(const char *message, Expression *expression, std::string filePath);

    CompileTypeException(const char *message, Expression *expression, CompilationContext *compilationContext);

    CompileTypeException(const char *message, ASTNode *astNode, CompilationContext *compilationContext);
};

/**
 * @author Array type resolver
 */
class ArrayTypeResolver {
public:
    static RocType* resolve(ArrayCreateExpr* arrayCreateExpr, CompilationContext *compilationContext);
};

static RocTypeNodeContext* createTypeContext(Expression* expression) {
    return (RocTypeNodeContext*) expression->addContextHolder(TYPE_CONTEXT, new RocTypeNodeContext());
}

static RocTypeNodeContext* getTypeContext(Expression* expression) {
    return (RocTypeNodeContext*) expression->getContextHolder(TYPE_CONTEXT);
}

static RocType* getReturnType(Expression* expression) {
    return getTypeContext(expression)->getGivenType();
}

#endif //ROC_LANG_TYPES_H
