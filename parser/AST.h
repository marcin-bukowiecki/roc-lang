//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#pragma once
#ifndef ROC_LANG_AST_H
#define ROC_LANG_AST_H

#include "Token.h"

#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>
#include <iterator>
#include <exception>

/**
 * Header file containing classes to build Abstract Syntax Tree and visitors to do proper transformations.
 */

class FunctionBody;

class ArgumentsVisitor;

class ASTVisitor;

class ASTNode;

class FunctionDeclaration;

class ModuleDeclaration;

class CodeBlock;

class FieldDeclaration;

class LiteralExpr;

class ImportDeclaration;

class FunctionCallNode;

class Lexer;

class Expression;

class LocalVariableRef;

class RocType;

class NodeContextHolder {

};

/**
 * Base class for compilation unit i.e. function, static block, module
 */
class CompilationNode {
public:
    std::set<std::string> globalSymbols;
    std::map<std::string, LocalVariableRef*> localSymbols;
    std::vector<std::unique_ptr<LocalVariableRef>> locals;

    CompilationNode();

    virtual int getAndIncrLabelCounter() {
        throw std::exception("Unsupported label counter operation");
    }

    virtual bool isFunctionDeclaration() {
        return false;
    }

    virtual bool isStaticBlock() {
        return false;
    }

    virtual bool isModule() {
        return false;
    }
};

/**
 * Context holding AST node information
 */
class NodeContext {
public:
    ASTNode *parent = nullptr;
    ElementType nodeType;

    explicit NodeContext(ElementType elementType) {
        this->nodeType = elementType;
    }

    virtual ~NodeContext() = default;
};

/**
 * Base class for all AST nodes
 */
class ASTNode {
private:
    std::map<std::string, NodeContextHolder*> contextHolder{};
public:
    std::unique_ptr<NodeContext> nodeContext;

    explicit ASTNode(ElementType elementType) {
        this->nodeContext = std::make_unique<NodeContext>(elementType);
    }

    virtual ~ASTNode() = default;

    NodeContextHolder* addContextHolder(const std::string& key, NodeContextHolder* givenContext) {
        contextHolder.insert({key, givenContext});
        return getContextHolder(key);
    }

    NodeContextHolder* getContextHolder(const std::string& key) {
        return contextHolder.find(key)->second;
    }

    void setParent(ASTNode *parent) const {
        this->nodeContext->parent = parent;
    }

    virtual ElementType getNodeType() {
        return nodeContext->nodeType;
    };

    virtual void accept(ASTVisitor *) = 0;

    virtual void replaceChild(Expression *old, std::unique_ptr<Expression> with) {
        throw std::exception("Unsupported replaceChild() operation");
    }

    virtual void removeChild(Expression *ptr) {
        throw std::exception("Unsupported removeChild() operation");
    }

    virtual std::string getText() = 0;

    virtual ASTNode *getParent() {
        return this->nodeContext->parent;
    }

    virtual bool isLiteralExpr() {
        return false;
    }

    virtual bool isDotExpr() {
        return false;
    }

    virtual size_t getStartOffset() {
        return -1;
    }

    virtual size_t getEndOffset() {
        return -1;
    }
};

/**
 * Base type for types
 */
class TypeNode : public ASTNode {
public:
    TypeNode();

    void accept(ASTVisitor *) override;

    std::string getText() override = 0;
};

/**
 * Type for simple types i.e. Int32
 */
class SingleTypeNode : public TypeNode {
public:
    Token* literal;

    explicit SingleTypeNode(Token* literal);

    ~SingleTypeNode() {
        delete literal;
    }

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return literal->getText();
    }

    size_t getStartOffset() override {
        return literal->getStartOffset();
    }

    size_t getEndOffset() override {
        return literal->getEndOffset();
    }
};

/**
 * For generics: <Int32, Int64>
 */
class TypeParametersList : public TypeNode {
public:
    std::unique_ptr<Lesser> lesser;
    std::vector<std::unique_ptr<TypeNode>> types;
    std::unique_ptr<Greater> greater;

    explicit TypeParametersList(std::unique_ptr<Lesser> lesser,
                                std::vector<std::unique_ptr<TypeNode>> types,
                                std::unique_ptr<Greater> greater);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        std::string acc;
        for (auto& t: types) {
            if (!acc.empty()) {
                acc.append(",");
            }
            acc.append(t->getText());
        }
        return "<" + acc + ">";
    }
};

/**
 * []Int32
 */
class ArrayTypeNode : public TypeNode {
public:
    std::unique_ptr<LeftBracket> l;
    std::unique_ptr<RightBracket> r;
    std::unique_ptr<TypeNode> singleTypeNode;

    explicit ArrayTypeNode(std::unique_ptr<LeftBracket> l,
                           std::unique_ptr<RightBracket> r,
                           std::unique_ptr<TypeNode> singleTypeNode);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return "[]" + singleTypeNode->getText();
    }
};

class StructFieldDeclaration : public ASTNode {
private:
    std::unique_ptr<Token> name;
public:
    explicit StructFieldDeclaration(std::unique_ptr<Token> name);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return this->name->getText();
    }
};

class StructDeclaration : public ASTNode {
public:
    std::unique_ptr<Token> keyword;
    std::unique_ptr<Token> name;
    std::vector<std::unique_ptr<StructFieldDeclaration>> fields;

    explicit StructDeclaration(std::unique_ptr<Token> keyword,
                               std::unique_ptr<Token> name,
                               std::vector<std::unique_ptr<StructFieldDeclaration>> fields);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return this->keyword->getText() + " " + this->name->getText() + "\n";
    }
};

class Parameter : public ASTNode {
public:
    Token* name;
    TypeNode* typeNode;

    explicit Parameter(Token* name, TypeNode* typeNode);

    ~Parameter() {
        delete name;
        delete typeNode;
    };

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return name->getText();
    }

    virtual bool isVarArgs() {
        return false;
    }
};

class ParameterList : public ASTNode {
public:
    LeftParenthesis* lp;
    std::vector<Parameter*> parameters;
    RightParenthesis* rp;

    explicit ParameterList(LeftParenthesis* lp,
                           std::vector<Parameter*> parameters,
                           RightParenthesis* rp) : ASTNode(ElementType::parameterList) {
        this->parameters = std::move(parameters);
        this->lp = lp;
        this->rp = rp;
        for (const auto &p: this->parameters) {
            p->setParent(this);
        }
    }

    ~ParameterList() override {
        delete lp;
        for (auto& p: parameters) {
            delete p;
        }
        delete rp;
    }

    void accept(ASTVisitor *) override;

    size_t size() const;

    std::string getText() override {
        std::string acc;
        for (const auto &p: this->parameters) {
            acc.append(p->getText());
        }
        return acc;
    }
};

class Expression : public ASTNode {
private:
    std::vector<Token*> whitespaces{};
public:
    std::vector<TypeNode*> typeVariables{};
    explicit Expression(ElementType elementType);
    ~Expression() override {
        for (const auto &item : whitespaces) {
            delete item;
        }
        for (const auto &item : typeVariables) {
            delete item;
        }
    }
    std::string getText() override = 0;
    virtual bool isEmpty() {
        return false;
    }
    void addWhiteSpace(Token* t) {
        whitespaces.push_back(t);
    }
    std::vector<Token*> getWhiteSpaces() {
        return whitespaces;
    }
};

class ParenthesisExpr : public Expression {
private:
    std::unique_ptr<Token> lp;
    std::unique_ptr<Token> rp;

public:
    std::unique_ptr<Expression> inner;

    explicit ParenthesisExpr(std::unique_ptr<Token> lp, std::unique_ptr<Expression> inner, std::unique_ptr<Token> rp)
            : Expression(ElementType::parenthesisExpr) {
        this->lp = std::move(lp);
        this->rp = std::move(rp);
        this->inner = std::move(inner);
        this->inner->setParent(this);
    }

    std::string getText() override {
        return lp->getText() + inner->getText() + rp->getText();
    }

    void accept(ASTVisitor *) override;
};

class ForLoopExpression : public Expression {
public:
    std::unique_ptr<ForKeyword> forKeyword;
    std::unique_ptr<Expression> forLoopSubject;
    std::unique_ptr<CodeBlock> codeBlock;

    ForLoopExpression(std::unique_ptr<ForKeyword> forKeyword,
                      std::unique_ptr<Expression> forLoopSubject,
                      std::unique_ptr<CodeBlock> codeBlock);

    void accept(ASTVisitor *) override;

    std::string getText() override;
};

class WhileLoopExpression : public Expression {
public:
    std::unique_ptr<Token> keyword;
    std::unique_ptr<Expression> expression;
    std::unique_ptr<CodeBlock> body;

    explicit WhileLoopExpression(std::unique_ptr<Token> keyword, std::unique_ptr<Expression> expression,
                                 std::unique_ptr<CodeBlock> body);

    void accept(ASTVisitor *) override;

    std::string getText() override;
};

class TupleCreate : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> expressions;

    explicit TupleCreate(std::vector<std::unique_ptr<Expression>> expressions) : Expression(ElementType::tupleExpression) {
        this->expressions = std::move(expressions);
        for (const auto &e: this->expressions) {
            e->setParent(this);
        }
    }

    std::string getText() override {
        std::string acc;
        for (const auto &e: this->expressions) {
            acc.append(e->getText());
        }
        return acc;
    }

    void accept(ASTVisitor *) override;
};

class LiteralExpr : public Expression {
public:
    std::unique_ptr<Literal> literal;

    explicit LiteralExpr(std::unique_ptr<Literal> token);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return literal->getText();
    }

    bool isLiteralExpr() override {
        return true;
    }
};

class ArrayAnonymousGetExpr : public Expression {
private:
    std::unique_ptr<Token> lb;
    std::unique_ptr<Token> rb;

public:
    std::unique_ptr<Expression> index;

    ArrayAnonymousGetExpr(std::unique_ptr<Token> lb,
                          std::unique_ptr<Expression> index,
                          std::unique_ptr<Token> rb) : Expression(ElementType::arrayAnonymousGetExpr) {
        this->lb = std::move(lb);
        this->index = std::move(index);
        this->index->nodeContext->parent = this;
        this->rb = std::move(rb);
    }

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return lb->getText() + index->getText() + rb->getText();
    }
};

class ArrayGetExpr : public Expression {
private:
    std::unique_ptr<Token> lb;
    std::unique_ptr<Token> rb;

public:
    std::unique_ptr<Expression> index;

    ArrayGetExpr(std::unique_ptr<Token> lb,
                 std::unique_ptr<Expression> index,
                 std::unique_ptr<Token> rb) : Expression(ElementType::arrayGetExpr) {
        this->lb = std::move(lb);
        this->index = std::move(index);
        this->index->nodeContext->parent = this;
        this->rb = std::move(rb);
    }

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return lb->getText() + index->getText() + rb->getText();
    }
};

class TrueExpr : public Expression {
public:
    std::unique_ptr<TrueKeyword> le;

    explicit TrueExpr(std::unique_ptr<TrueKeyword> le);

    std::string getText() override {
        return le->getText();
    }

    void accept(ASTVisitor *) override;
};

class FalseExpr : public Expression {
public:
    std::unique_ptr<FalseKeyword> le;

    explicit FalseExpr(std::unique_ptr<FalseKeyword> le);

    std::string getText() override {
        return le->getText();
    }

    void accept(ASTVisitor *) override;
};

class KeyValueExpr : public Expression {
public:
    std::unique_ptr<Expression> key;
    std::unique_ptr<Colon> colon;
    std::unique_ptr<Expression> value;

    KeyValueExpr(std::unique_ptr<Expression> key,
                 std::unique_ptr<Colon> colon,
                 std::unique_ptr<Expression> value);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return key->getText() + colon->getText() + value->getText();
    }
};

class ArrayCreateExpr : public Expression {
private:
    std::unique_ptr<Token> lb;
    std::unique_ptr<Token> rb;

public:
    std::vector<std::unique_ptr<Expression>> arguments;

    ArrayCreateExpr(std::unique_ptr<Token> lb, std::vector<std::unique_ptr<Expression>> arguments,
                    std::unique_ptr<Token> rb);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        std::string text;
        for (const auto &e: this->arguments) {
            text.append(e->getText());
        }
        return lb->getText() + text + rb->getText();
    }
};

class ReturnExpression : public Expression {
public:
    std::unique_ptr<Token> returnKeyword;
    std::unique_ptr<Expression> expression;

    ReturnExpression(std::unique_ptr<Token> returnKeyword, std::unique_ptr<Expression> expression);

    void accept(ASTVisitor *) override;

    std::string getText() override;
};


class ElseExpression : public Expression {
public:
    ElseKeyword* elseKeyword;
    Token* leftCurl;
    std::unique_ptr<CodeBlock> block;
    Token* rightCurl;

    ElseExpression(ElseKeyword* elseKeyword,
                   Token* leftCurl,
                   std::unique_ptr<CodeBlock> block,
                   Token* rightCurl);

    ~ElseExpression() {
        delete elseKeyword;
        delete leftCurl;
        delete rightCurl;
    }

    void accept(ASTVisitor *) override;

    std::string getText() override;
};

class IfExpression : public Expression {
private:
    IfKeyword* ifKeyword;
    Token* leftCurl;
    Token* rightCurl;

public:
    std::unique_ptr<Expression> expression;
    std::unique_ptr<CodeBlock> block;
    std::unique_ptr<ElseExpression> elseExpression;

    IfExpression(IfKeyword* ifKeyword,
                 std::unique_ptr<Expression> expression,
                 Token* leftCurl,
                 std::unique_ptr<CodeBlock> block,
                 Token* rightCurl);

    void accept(ASTVisitor *) override;

    std::string getText() override;

    size_t getStartOffset() override {
        return ifKeyword->getStartOffset();
    }
};

class BinExpr : public Expression {
public:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::unique_ptr<Token> op;

    BinExpr(std::unique_ptr<Expression> left,
            std::unique_ptr<Token> op,
            std::unique_ptr<Expression> right,
            ElementType nodeType);

    void accept(ASTVisitor *visitor) override = 0;

    void replaceChild(Expression *old, std::unique_ptr<Expression> with) override {
        if (left.get() == old) {
            this->left = std::move(with);
        } else if (right.get() == old) {
            this->right = std::move(with);
        }
    }

    std::string getText() override {
        return this->left->getText() + op->getText() + this->right->getText();
    }

    size_t getStartOffset() override {
        return left->getStartOffset();
    }

    size_t getEndOffset() override {
        return right->getEndOffset();
    }
};

class NotExpr : public BinExpr {
public:
    NotExpr(std::unique_ptr<Token> notOp,
            std::unique_ptr<Expression> r) : BinExpr(nullptr, std::move(notOp), std::move(r), ElementType::notOp) {}

    void accept(ASTVisitor *visitor) override;
};

class AndExpr : public BinExpr {
public:
    AndExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class OrExpr : public BinExpr {
public:
    OrExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class AddExpr : public BinExpr {
public:
    AddExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class SubExpr : public BinExpr {
public:
    SubExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class MulExpr : public BinExpr {
public:
    MulExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class DivExpr : public BinExpr {
public:
    DivExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class ModExpr : public BinExpr {
public:
    ModExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class PowExpr : public BinExpr {
public:
    PowExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class InverseDivExpr : public BinExpr {
public:
    InverseDivExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class LesserExpr : public BinExpr {
public:
    LesserExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class LesserOrEqualExpr : public BinExpr {
public:
    LesserOrEqualExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class GreaterExpr : public BinExpr {
public:
    GreaterExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class GreaterOrEqualExpr : public BinExpr {
public:
    GreaterOrEqualExpr(std::unique_ptr<Expression>, std::unique_ptr<Token>, std::unique_ptr<Expression>);

    void accept(ASTVisitor *visitor) override;
};

class RefAssignExpr : public Expression {
public:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::unique_ptr<Token> assignOp;

    RefAssignExpr(std::unique_ptr<Expression> left,
                  std::unique_ptr<Token> assignOp,
                  std::unique_ptr<Expression> right) : Expression(ElementType::refAssignExpr) {
        this->assignOp = std::move(assignOp);
        this->left = std::move(left);
        this->right = std::move(right);
        this->left->setParent(this);
        this->right->setParent(this);
    }

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return this->left->getText() + assignOp->getText() + this->right->getText();
    }
};

class EqualOpExpr : public BinExpr {
public:
    EqualOpExpr(std::unique_ptr<Expression> left, std::unique_ptr<Token> equalOp, std::unique_ptr<Expression> right) :
            BinExpr(std::move(left), std::move(equalOp), std::move(right), ElementType::equalOp) {}

    void accept(ASTVisitor *) override;
};

class NotEqualOpExpr : public BinExpr {
public:
    NotEqualOpExpr(std::unique_ptr<Expression> left, std::unique_ptr<Token> equalOp, std::unique_ptr<Expression> right)
            :
            BinExpr(std::move(left), std::move(equalOp), std::move(right), ElementType::notEqualOp) {}

    void accept(ASTVisitor *) override;
};

class NegativeExpr : public Expression {
public:
    std::unique_ptr<Token> sub;
    std::unique_ptr<Expression> expression;

    NegativeExpr(std::unique_ptr<Token> sub, std::unique_ptr<Expression> expression);

    void accept(ASTVisitor *) override;

    std::string getText() override;
};

class StringNode : public Expression {
public:
    Token *lqm;
    Token *rqm;
    std::vector<Token *> tokens;

    StringNode(Token *lqm, std::vector<Token *> tokens, Token *rqm);

    void accept(ASTVisitor *) override;

    std::string getTokensAsText();

    std::string getText() override;

    std::string getAsStringValue();

    size_t getStartOffset() override {
        return lqm->getStartOffset();
    }

    size_t getEndOffset() override {
        return lqm->getEndOffset();
    }
};

class NumberNode : public Expression {
public:
    explicit NumberNode(ElementType elementType) : Expression(elementType) {}

    void accept(ASTVisitor *) override;

    std::string getText() override = 0;
};

class IntNode : public NumberNode {
public:
    std::unique_ptr<IntNumber> value;

    explicit IntNode(std::unique_ptr<IntNumber> value);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return value->getText();
    };

    size_t getStartOffset() override {
        return value->getStartOffset();
    }

    size_t getEndOffset() override {
        return value->getEndOffset();
    }
};

class DoubleNode : public NumberNode {
public:
    std::unique_ptr<DoubleNumber> value;

    explicit DoubleNode(std::unique_ptr<DoubleNumber> value);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return value->getText();
    };
};

class ArgumentExpression : public ASTNode {
public:
    std::unique_ptr<Expression> expression;

    explicit ArgumentExpression(std::unique_ptr<Expression> expression);

    void accept(ASTVisitor *) override;

    void replaceChild(Expression *old, std::unique_ptr<Expression> with) override;

    std::string getText() override;
};

/**
 * List of arguments for function call
 *
 * (expr, expr, expr)
 */
class ArgumentList : public ASTNode {
public:
    LeftParenthesis* lp;
    std::vector<std::unique_ptr<ArgumentExpression>> arguments;
    RightParenthesis* rp;

    ArgumentList(LeftParenthesis* lp,
                 std::vector<std::unique_ptr<ArgumentExpression>> arguments,
                 RightParenthesis* rp);

    ~ArgumentList() {
        delete lp;
        delete rp;
    }

    void accept(ASTVisitor *) override;

    size_t size() const;

    std::string getText() override {
        std::string acc;
        for (const auto &e: this->arguments) {
            acc += e->getText();
        }
        return acc;
    }
};

/**
 * Body of function
 *
 * {
 *  expr
 *  expr
 * }
 */
class FunctionBody : public ASTNode {
public:
    std::vector<std::unique_ptr<Expression>> expressions;

    explicit FunctionBody(std::vector<std::unique_ptr<Expression>> expressions) : ASTNode(ElementType::functionBody),
                                                                                  expressions(std::move(expressions)) {

        for (const auto &expr: this->expressions) {
            expr->setParent(this);
        }
    }

    void replaceChild(Expression *old, std::unique_ptr<Expression> with) override;

    void accept(ASTVisitor *) override;

    std::string getText() override {
        std::string acc;
        for (const auto &expr: expressions) {
            acc += expr->getText();
        }
        return acc;
    }
};

/**
 * Return node of function
 *
 * -> Int {
 *
 */
class FunctionReturnTypeNode : public ASTNode {
public:
    TypeNode* typeNode;

    explicit FunctionReturnTypeNode(TypeNode* typeNode) : ASTNode(ElementType::functionReturnType) {
        this->typeNode = typeNode;
    }

    ~FunctionReturnTypeNode() {
        delete typeNode;
    }

    void accept(ASTVisitor *) override = 0;

    std::string getText() override = 0;
};

class FunctionNonVoidReturnTypeNode : public FunctionReturnTypeNode {
public:
    Arrow* arrow;

    FunctionNonVoidReturnTypeNode(Arrow* arrow, TypeNode* typeNode);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return "" + arrow->getText() + typeNode->getText();
    }
};

class FunctionVoidReturnTypeNode : public FunctionReturnTypeNode {
public:
    FunctionVoidReturnTypeNode();

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return "";
    }
};

/**
 * fun adder(a Int, b Int) -> Int {
 *  ret a + b
 * }
 *
 */
class FunctionDeclaration : public CompilationNode, public ASTNode {
private:
    FunKeyword* keyword;
    Token* name;
    LeftCurl* leftCurl;
    RightCurl* rightCurl;
public:
    int labels = 0;
    std::unique_ptr<ParameterList> parameterList;
    std::unique_ptr<FunctionReturnTypeNode> functionReturnTypeNode;
    std::unique_ptr<FunctionBody> body;

    FunctionDeclaration(FunKeyword* keyword,
                        Token* name,
                        std::unique_ptr<ParameterList> parameterList,
                        std::unique_ptr<FunctionReturnTypeNode> fFunctionReturnTypeNode,
                        LeftCurl* leftCurl,
                        std::unique_ptr<FunctionBody> body,
                        RightCurl* rightCurl);

    ~FunctionDeclaration() {
        delete keyword;
        delete name;
        delete leftCurl;
        delete rightCurl;
    }

    Token *getName() const;

    int getAndIncrLabelCounter() override {
        return labels++;
    }

    bool isFunctionDeclaration() override {
        return true;
    }

    void accept(ASTVisitor *) override;

    std::string getText() override {
        std::string acc =
                this->keyword->getText() + " " + this->name->getText() + "(" + parameterList->getText() + ")\n";
        acc += leftCurl->getText();
        acc += "\n";
        acc += "    " + this->body->getText();
        acc += "\n";
        acc += rightCurl->getText();
        acc += "\n";
        return acc;
    }
};

class Lambda : public Expression, public CompilationNode {
public:
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::vector<std::unique_ptr<Expression>> lambdaBody;

    Lambda(std::vector<std::unique_ptr<Parameter>> parameters, std::vector<std::unique_ptr<Expression>> lambdaBody)
            : Expression(ElementType::lambdaDeclaration) {
        this->parameters = std::move(parameters);
        this->lambdaBody = std::move(lambdaBody);
    }

    void accept(ASTVisitor *) override;

    std::string getText() override;
};

class CodeBlock : public ASTNode {
public:
    std::vector<std::unique_ptr<Expression>> expressions;

    explicit CodeBlock(std::vector<std::unique_ptr<Expression>> expressions);

    void accept(ASTVisitor *) override;

    std::string getText() override;

    void replaceChild(Expression *old, std::unique_ptr<Expression> with) override;
};

class FunctionCallNode : public Expression {
public:
    std::unique_ptr<LiteralExpr> literalExpr;
    std::unique_ptr<ArgumentList> argumentList;
    std::string functionName;

    FunctionCallNode(std::unique_ptr<LiteralExpr> literalExpr, std::unique_ptr<ArgumentList> argumentList);

    void accept(ASTVisitor *) override;

    std::string getText() override;

    virtual std::string getName();

    size_t getStartOffset() override;

    size_t getEndOffset() override;
};

class ReferenceExpression : public Expression {
public:
    Expression* reference;
    Token* dot;
    Expression* furtherExpression;

    explicit ReferenceExpression(Expression* reference,
                                 Token* dot,
                                 Expression* furtherExpression) : Expression(ElementType::referenceExpression) {
        this->reference = reference;
        this->reference->setParent(this);
        this->dot = dot;
        this->furtherExpression = furtherExpression;
        this->furtherExpression->setParent(this);
    }

    ~ReferenceExpression() {
        delete reference;
        delete dot;
        delete furtherExpression;
    }

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return reference->getText() + "." + furtherExpression->getText();
    }

    bool isDotExpr() override {
        return true;
    }

    void replaceChild(Expression *old, std::unique_ptr<Expression> with) override {
        if (reference == old) {
            this->reference = with.release();
        } else if (furtherExpression == old) {
            this->furtherExpression = with.release();
        }
    }
};

class LocalVariableRef {
public:
    Parameter* parameter;
    std::string name;
    int index;
};

class LocalAccess : public Expression {
public:
    std::unique_ptr<Literal> literal;
    LocalVariableRef* localVariableRef;

    explicit LocalAccess(std::unique_ptr<Literal> token);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return literal->getText();
    }

    size_t getStartOffset() override {
        return literal->getStartOffset();
    }

    size_t getEndOffset() override {
        return literal->getEndOffset();
    }
};

class LocalStore : public Expression {
public:
    std::unique_ptr<Literal> literal;
    std::unique_ptr<Token> assignOp;
    std::unique_ptr<Expression> rightExpr;

    LocalStore(std::unique_ptr<Literal> literal,
               std::unique_ptr<Token> assignOp,
               std::unique_ptr<Expression> rightExpr);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return literal->getText() + " " + assignOp->getText() + " " + rightExpr->getText();
    }
};

class GlobalAccess : public Expression {
public:
    std::unique_ptr<Literal> literal;

    explicit GlobalAccess(std::unique_ptr<Literal> token);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return literal->getText();
    }
};

class GlobalStore : public Expression {
public:
    std::unique_ptr<Literal> literal;
    std::unique_ptr<Token> assignOp;
    std::unique_ptr<Expression> rightExpr;

    GlobalStore(std::unique_ptr<Literal> literal,
                std::unique_ptr<Token> assignOp,
                std::unique_ptr<Expression> rightExpr);

    void accept(ASTVisitor *) override;

    std::string getText() override {
        return literal->getText() + " " + assignOp->getText() + " " + rightExpr->getText();
    }
};

class PackageNode : public ASTNode {
private:
    std::vector<Token*> children;

public:
    explicit PackageNode(std::vector<Token*> children) : ASTNode(ElementType::packageNode) {
        this->children = std::move(children);
    }

    ~PackageNode() override {
        for (const auto &ch : children) {
            delete ch;
        }
    }

    std::string getText() override {
        std::string acc;
        for (const auto &ch : children) {
            acc+=ch->getText();
        }
        return acc;
    }

    PackageKeyword* getPackageKeyword() {
        return (PackageKeyword*) children.at(0);
    }

    std::string getCanonicalPath() {
        std::string acc;
        for (const auto &ch : children) {
            if (ch->getTokenType() == ElementType::literal || ch->getTokenType() == ElementType::dot) {
                acc+=ch->getText();
            }
        }
        return acc;
    }

    void accept(ASTVisitor *visitor) override;

    virtual bool isDefault() {
        return false;
    }
};

/**
 * Represents import declaration
 */
class ImportDeclaration : public Expression {
public:
    std::unique_ptr<Token> importKeyword;
    std::unique_ptr<Expression> expr;
    std::unique_ptr<Semicolon> semicolon;

    ImportDeclaration(std::unique_ptr<Token> importKeyword,
                      std::unique_ptr<Expression> expr,
                      std::unique_ptr<Semicolon> semicolon);

    void accept(ASTVisitor *) override;

    std::string getText() override;

    std::string getPath() const {
        return importKeyword->getText() + " " + expr->getText();
    }
};

class StaticBlock : public ASTNode, public CompilationNode {
public:
    std::vector<std::unique_ptr<Expression>> expressions;
    int labels = 1;

    explicit StaticBlock(std::vector<std::unique_ptr<Expression>> expressions);

    void accept(ASTVisitor *) override;

    int getAndIncrLabelCounter() override {
        return labels++;
    }

    std::string getText() override {
        std::string acc;
        for (const auto &expr: this->expressions) {
            acc += expr->getText();
        }
        return acc;
    }

    bool isStaticBlock() override {
        return true;
    }

    void replaceChild(Expression *old, std::unique_ptr<Expression> with) override {
        size_t i = 0;
        for (auto const &expr: this->expressions) {
            if (expr.get() == old) {
                this->expressions.at(i) = std::move(with);
                break;
            }
        }
    }
};

class WithFields {
public:
    //std::vector<std::unique_ptr<ImportDeclaration>> fields;
};

class WithImports {
public:
    std::vector<std::unique_ptr<ImportDeclaration>> imports;
};

class ModuleDeclaration : public ASTNode, public CompilationNode, public WithFields, public WithImports {
public:
    std::string moduleName;
    std::string absolutePath;
    PackageNode* packageNode;
    StaticBlock* staticBlock;
    std::vector<std::unique_ptr<FunctionDeclaration>> functions;
    std::vector<std::unique_ptr<StructDeclaration>> types;

    ModuleDeclaration(std::string moduleName,
                      std::string absolutePath,
                      std::vector<std::unique_ptr<ImportDeclaration>> imports,
                      std::vector<std::unique_ptr<FunctionDeclaration>> functions,
                      std::vector<std::unique_ptr<StructDeclaration>> types,
                      StaticBlock* staticBlock);

    ~ModuleDeclaration() {
        //delete packageNode;
        delete staticBlock;
    }

    void accept(ASTVisitor *astVisitor) override;

    bool isModule() override {
        return true;
    }

    std::string getCanonicalName() const {
        if (this->packageNode) {
            return packageNode->getCanonicalPath() + "." + moduleName;
        } else {
            return moduleName;
        }
    }

    std::string getText() override {
        std::string acc;
        for (auto &f: this->functions) {
            acc += f->getText();
        }
        acc += this->staticBlock->getText();
        acc += "\n";
        return acc;
    }
};

class ParameterListVisitor {
public:
    LeftParenthesis *token{};
    std::vector<Parameter*> parameters{};
    RightParenthesis *end{};

    explicit ParameterListVisitor(LeftParenthesis *token);

    void visit(VisitingContext *ctx);
};

class StringVisitor : public TokenVisitor {
public:
    std::unique_ptr<Expression> conditionalExpression;

    void visit(QuotionMark *lqm, VisitingContext *) override;
};

class IfVisitor : public TokenVisitor {
public:
    IfExpression* conditionalExpression;

    void visit(IfKeyword *, VisitingContext *) override;
};

class ExpressionVisitor : public TokenVisitor {
public:
    std::unique_ptr<Expression> currentExpression;

    void visit(TrueKeyword *, VisitingContext *) override;

    void visit(FalseKeyword *, VisitingContext *) override;

    void visit(DoubleNumber *, VisitingContext *) override;

    void visit(IntNumber *, VisitingContext *) override;

    void visit(Literal *, VisitingContext *) override;

    void visit(LeftParenthesis *, VisitingContext *) override;

    void visit(QuotionMark *, VisitingContext *) override;

    void visit(Semicolon *, VisitingContext *) override;

    void visit(LeftBracket *, VisitingContext *) override;

    void visit(Colon *, VisitingContext *) override;

    void visit(Add *, VisitingContext *) override;

    void visit(Lesser *, VisitingContext *) override;

    void visit(Greater *, VisitingContext *) override;

    void visit(LesserOrEqual *, VisitingContext *) override;

    void visit(GreaterOrEqual *, VisitingContext *) override;

    void visit(EqualOp *, VisitingContext *) override;

    void visit(AndKeyword *, VisitingContext *) override;

    void visit(OrKeyword *, VisitingContext *) override;

    void visit(Sub *, VisitingContext *) override;

    void visit(Div *, VisitingContext *) override;

    void visit(InverseDiv *, VisitingContext *) override;

    void visit(Mul *, VisitingContext *) override;

    void visit(Mod *, VisitingContext *) override;

    void visit(Pow *, VisitingContext *) override;

    void visit(NotOp *, VisitingContext *) override;

    void visit(AssignOp *, VisitingContext *) override;

    void visit(RetKeyword *, VisitingContext *) override;

    void visit(IfKeyword *, VisitingContext *) override;

    void visit(ForKeyword *, VisitingContext *) override;

    void visit(Dot *, VisitingContext *) override;

    void visit(ImportKeyword *, VisitingContext *) override;

    void visit(LeftCurl *, VisitingContext *) override;

    void visit(NewLine *, VisitingContext *) override;
};

class OpExpressionVisitor {
public:
    Expression *leftHand;
    BinOp *binOp;

    OpExpressionVisitor(Expression *leftHand, BinOp *binOp) {
        this->leftHand = leftHand;
        this->binOp = binOp;
    }

    std::unique_ptr<Expression> visit(VisitingContext *ctx) const;
};

class RightHandExpressionVisitor : public ExpressionVisitor {
public:
    void visit(Add *add, VisitingContext *ctx) override {}

    void visit(Sub *, VisitingContext *) override {}

    void visit(Div *, VisitingContext *) override {}

    void visit(Mod *, VisitingContext *) override {}

    void visit(Pow *, VisitingContext *) override {}

    void visit(Mul *mul, VisitingContext *ctx) override {}

    void visit(Lesser *, VisitingContext *ctx) override {}

    void visit(LesserOrEqual *, VisitingContext *ctx) override {}

    void visit(Greater *, VisitingContext *ctx) override {}

    void visit(GreaterOrEqual *, VisitingContext *ctx) override {}

    void visit(AndKeyword *, VisitingContext *ctx) override {}

    void visit(OrKeyword *, VisitingContext *ctx) override {}
};

class ReferenceExpressionVisitor : public RightHandExpressionVisitor {
public:
    void visit(Dot *, VisitingContext *) override {};

    void visit(AssignOp *, VisitingContext *) override {};

    void visit(IntNumber *, VisitingContext *) override;

    void visit(Sub *, VisitingContext *) override;
};

class ArgumentsVisitor : public TokenVisitor {
public:
    std::vector<std::unique_ptr<ArgumentExpression>> arguments;

    void visit(VisitingContext *ctx);
};

class DslArgumentsVisitor : public TokenVisitor {
public:
    std::vector<std::unique_ptr<ArgumentExpression>> arguments;

    void visit(VisitingContext *ctx);
};

class ConditionBlockVisitor : public TokenVisitor {
public:
    Token* lastToken;
    std::vector<std::unique_ptr<Expression>> expressions;

    virtual void visit(VisitingContext *ctx);
};

class FunctionBodyVisitor {
public:
    std::vector<std::unique_ptr<Expression>> expressions;

    void visit(VisitingContext *ctx);
};

class WhileLoopVisitor : public TokenVisitor {
public:
    std::unique_ptr<WhileLoopExpression> whileLoopExpression;

    void visit(WhileKeyword *keyword, VisitingContext *ctx) override;
};

class ForLoopVisitor : public TokenVisitor {
public:
    std::vector<std::unique_ptr<Expression>> expressions;
    std::vector<std::unique_ptr<Semicolon>> semicolons;
    std::unique_ptr<Expression> forLoopExpression;
    std::unique_ptr<Expression> subject;

    void visit(ForKeyword *keyword, VisitingContext *ctx) override;
};

/**
 * Visitor for type declaration
 */
class TypeNodeVisitor : public TokenVisitor {
public:
    Token *start;
    TypeNode *result;

    explicit TypeNodeVisitor(Token *start) {
        this->start = start;
    }

    TypeNode *visit(VisitingContext *ctx);

    void visit(Literal *literal, VisitingContext *ctx) override;

    void visit(LeftBracket *leftBracket, VisitingContext *ctx) override;
};

/**
 * Visitor for function declaration
 */
class FunctionVisitor {
public:
    FunKeyword *funKeyword;

    explicit FunctionVisitor(FunKeyword *funKeyword) {
        this->funKeyword = funKeyword;
    }

    FunctionDeclaration *visit(VisitingContext *ctx) const;
};

class Config {
public:
    std::string srcInput;
    std::string srcOutput;
};

class ASTVisitor {
public:
    virtual void visit(FunctionVoidReturnTypeNode *node) {
        node->typeNode->accept(this);
    }

    virtual void visit(FunctionNonVoidReturnTypeNode *node) {
        node->typeNode->accept(this);
    }

    virtual void visit(GlobalStore *node) {

    }

    virtual void visit(GlobalAccess *node) {

    }

    virtual void visit(LocalStore *node) {

    }

    virtual void visit(LocalAccess *node) {

    }

    virtual void visit(KeyValueExpr *) {

    }

    virtual void visit(SingleTypeNode *) {

    }

    virtual void visit(ArrayTypeNode *arrayTypeNode) {
        arrayTypeNode->singleTypeNode->accept(this);
    }

    virtual void visit(TypeParametersList *typeParametersList) {
        for (auto& t: typeParametersList->types) {
            t->accept(this);
        }
    }

    virtual void visit(GreaterOrEqualExpr *ge) {
        ge->left->accept(this);
        ge->right->accept(this);
    }

    virtual void visit(LesserOrEqualExpr *le) {
        le->left->accept(this);
        le->right->accept(this);
    }

    virtual void visit(NotEqualOpExpr *ne) {
        ne->left->accept(this);
        ne->right->accept(this);
    }

    virtual void visit(WhileLoopExpression *loop) {
        loop->expression->accept(this);
        loop->body->accept(this);
    }

    virtual void visit(RefAssignExpr *ref) {
        ref->left->accept(this);
        ref->right->accept(this);
    }

    virtual void visit(TrueExpr *) {}

    virtual void visit(FalseExpr *) {}

    virtual void visit(StructFieldDeclaration *sf) {}

    virtual void visit(StructDeclaration *sd) {
        for (const auto &f: sd->fields) {
            f->accept(this);
        }
    }

    virtual void visit(TupleCreate *tc) {
        for (const auto &e: tc->expressions) {
            e->accept(this);
        }
    }

    virtual void visit(ReferenceExpression *referenceExpression) {
        referenceExpression->reference->accept(this);
        referenceExpression->furtherExpression->accept(this);
    }

    virtual void visit(ForLoopExpression *forLoopExpression) {
        forLoopExpression->forLoopSubject->accept(this);
        forLoopExpression->codeBlock->accept(this);
    }

    virtual void visit(ModuleDeclaration *moduleNode) {
        if (moduleNode->staticBlock != nullptr) {
            moduleNode->staticBlock->accept(this);
        }
        for (const auto &fd : moduleNode->functions) {
            fd->accept(this);
        }
    }

    virtual void visit(FunctionDeclaration *fd) {
        fd->parameterList->accept(this);
        fd->body->accept(this);
    }

    virtual void visit(FunctionBody *fb) {
        for (const auto &e : fb->expressions) {
            e->accept(this);
        }
    }

    virtual void visit(CodeBlock *cb) {
        for (const auto &e : cb->expressions) {
            e->accept(this);
        }
    }

    virtual void visit(ParameterList *pl) {
        for (const auto &a : pl->parameters) {
            a->accept(this);
        }
    }

    virtual void visit(DoubleNode *) {}

    virtual void visit(StringNode *) {}

    virtual void visit(LiteralExpr *) {}

    virtual void visit(ParenthesisExpr *expr) {
        expr->inner->accept(this);
    }

    virtual void visit(ArgumentList *al) {
        for (const auto &a : al->arguments) {
            a->accept(this);
        }
    }

    virtual void visit(ArgumentExpression *argument) {
        argument->expression->accept(this);
    }

    virtual void visit(FunctionCallNode *);

    virtual void visit(ImportDeclaration *) {}

    virtual void visit(Parameter *) {}

    virtual void visit(AddExpr *addExpr) {
        addExpr->left->accept(this);
        addExpr->right->accept(this);
    }

    virtual void visit(DivExpr *divExpr) {
        divExpr->left->accept(this);
        divExpr->right->accept(this);
    }

    virtual void visit(SubExpr *subExpr) {
        subExpr->left->accept(this);
        subExpr->right->accept(this);
    }

    virtual void visit(NegativeExpr *expr) {
        expr->expression->accept(this);
    }

    virtual void visit(MulExpr *mulExpr) {
        mulExpr->left->accept(this);
        mulExpr->right->accept(this);
    }

    virtual void visit(InverseDivExpr *inverseDivExpr) {
        inverseDivExpr->left->accept(this);
        inverseDivExpr->right->accept(this);
    }

    virtual void visit(PowExpr *powExpr) {
        powExpr->left->accept(this);
        powExpr->right->accept(this);
    }

    virtual void visit(ModExpr *modExpr) {
        modExpr->left->accept(this);
        modExpr->right->accept(this);
    }

    virtual void visit(NotExpr *expr) {
        expr->right->accept(this);
    }

    virtual void visit(OrExpr *expr) {
        expr->left->accept(this);
        expr->right->accept(this);
    }

    virtual void visit(AndExpr *expr) {
        expr->left->accept(this);
        expr->right->accept(this);
    }

    virtual void visit(GreaterExpr *expr) {
        expr->left->accept(this);
        expr->right->accept(this);
    }

    virtual void visit(LesserExpr *expr) {
        expr->left->accept(this);
        expr->right->accept(this);
    }

    virtual void visit(EqualOpExpr *equalOpExpr) {
        equalOpExpr->left->accept(this);
        equalOpExpr->right->accept(this);
    };

    virtual void visit(NumberNode *) {}

    virtual void visit(StaticBlock *staticBlock) {
        for (const auto &e : staticBlock->expressions) {
            e->accept(this);
        }
    }

    virtual void visit(IntNode *intNode) {};

    virtual void visit(ArrayCreateExpr *arrayCreateExpr) {
        for (const auto &e: arrayCreateExpr->arguments) {
            e->accept(this);
        }
    };

    virtual void visit(ArrayAnonymousGetExpr *arrayAnonymousGetExpr) {
        arrayAnonymousGetExpr->index->accept(this);
    };

    virtual void visit(ArrayGetExpr *arrayGetExpr) {
        arrayGetExpr->index->accept(this);
    };

    virtual void visit(PackageNode *node) { };

    virtual void visit(ReturnExpression *re) {
        re->expression->accept(this);
    }

    virtual void visit(FieldDeclaration *fd) {}

    virtual void visit(IfExpression *ifExpression) {
        ifExpression->expression->accept(this);
        ifExpression->block->accept(this);
        if (ifExpression->elseExpression) {
            ifExpression->elseExpression->accept(this);
        }
    }

    virtual void visit(ElseExpression *elseExpression) {
        elseExpression->block->accept(this);
    }

    virtual void visit(Lambda *) {}

    virtual void visit(ForKeyword *) {}
};

class ToBinExprVisitor : public TokenVisitor {
public:
    std::unique_ptr<Expression> binExpr;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    ToBinExprVisitor(std::unique_ptr<Expression> left,
                     std::unique_ptr<Expression> right) : TokenVisitor() {
        this->left = std::move(left);
        this->right = std::move(right);
    }

    void visit(NotOp *no, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<NotExpr>(std::unique_ptr<NotOp>(no), std::move(right));
    }

    void visit(Add *add, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<AddExpr>(std::move(left), std::unique_ptr<Token>(add), std::move(right));
    }

    void visit(Sub *sub, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<SubExpr>(std::move(left), std::unique_ptr<Token>(sub), std::move(right));
    }

    void visit(Div *div, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<DivExpr>(std::move(left), std::unique_ptr<Token>(div), std::move(right));
    }

    void visit(Mul *mul, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<MulExpr>(std::move(left), std::unique_ptr<Token>(mul), std::move(right));
    }

    void visit(Mod *mod, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<ModExpr>(std::move(left), std::unique_ptr<Token>(mod), std::move(right));
    }

    void visit(Pow *pow, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<PowExpr>(std::move(left), std::unique_ptr<Token>(pow), std::move(right));
    }

    void visit(InverseDiv *id, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<InverseDivExpr>(std::move(left), std::unique_ptr<Token>(id), std::move(right));
    }

    void visit(EqualOp *eq, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<EqualOpExpr>(std::move(left), std::unique_ptr<Token>(eq), std::move(right));
    }

    void visit(NotEqualOp *notEq, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<NotEqualOpExpr>(std::move(left), std::unique_ptr<Token>(notEq),
                                                         std::move(right));
    }

    void visit(Greater *greater, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<GreaterExpr>(std::move(left), std::unique_ptr<Token>(greater),
                                                      std::move(right));
    }

    void visit(GreaterOrEqual *greaterOrEqual, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<GreaterOrEqualExpr>(std::move(left), std::unique_ptr<Token>(greaterOrEqual),
                                                             std::move(right));
    }

    void visit(Lesser *lesser, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<LesserExpr>(std::move(left), std::unique_ptr<Token>(lesser), std::move(right));
    }

    void visit(LesserOrEqual *lesserOrEqual, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<LesserOrEqualExpr>(std::move(left), std::unique_ptr<Token>(lesserOrEqual),
                                                            std::move(right));
    }

    void visit(AssignOp *ao, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<RefAssignExpr>(std::move(left), std::unique_ptr<Token>(ao), std::move(right));
    }

    void visit(AndKeyword *ak, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<AndExpr>(std::move(left), std::unique_ptr<Token>(ak), std::move(right));
    }

    void visit(OrKeyword *ok, VisitingContext *ctx) override {
        this->binExpr = std::make_unique<OrExpr>(std::move(left), std::unique_ptr<Token>(ok), std::move(right));
    }
};

class ASTPrinter : public ASTVisitor {
private:
    std::string tab = "";

public:

    void visit(ModuleDeclaration *moduleDeclaration) override {
        std::cout << tab + "ModuleDeclaration\n";
        auto prev = tab;
        tab+="    ";
        ASTVisitor::visit(moduleDeclaration);
        tab=prev;
    }

    void visit(FunctionDeclaration *functionDeclaration) override {
        std::cout << tab + "FunctionDeclaration\n";
        auto prev = tab;
        tab+="    ";
        ASTVisitor::visit(functionDeclaration);
        tab=prev;
    }

    void visit(ReturnExpression *returnExpression) override {
        std::cout << tab + "ReturnExpression\n";
        auto prev = tab;
        tab+="    ";
        ASTVisitor::visit(returnExpression);
        tab=prev;
    }

    void visit(FunctionCallNode *functionCallNode) override {
        std::cout << tab + "FunctionCallNode\n";
        auto prev = tab;
        tab+="    ";
        ASTVisitor::visit(functionCallNode);
        tab=prev;
    }
};

#endif