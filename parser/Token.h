//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#pragma once
#ifndef ROC_LANG_TOKEN_H
#define ROC_LANG_TOKEN_H

#include <string>
#include <memory>
#include <map>
#include <utility>
#include <utility>
#include <vector>

class TokenVisitor;
class VisitingContext;
class Lexer;

/**
 * operator precedence map
 */
const std::map<std::string, int> OP_PRECEDENCE = {
        {"=",   1},
        {"or",  3},
        {"and", 4},
        {"==", 8},
        {"!=", 8},
        {"<",   9},
        {">",   9},
        {"+",   11},
        {"-",   12},
        {"*",   13},
        {"/",   13},
        {"%",   13},
        {"!",   14},
        {"^",   14}
};

/**
 * Enum for token type
 */
enum ElementType {
    arrow,
    forLoopExpression,

    staticBlock,

    tupleExpression,

    varKeyword,
    valKeyword,

    metKeyword,

    breakKeyword,
    continueKeyword,

    varargsKeyword,

    inKeyword,
    isKeyword,

    localAccess,
    localStore,
    globalAccess,
    globalStore,

    interfaceKeyword,
    structKeyword,

    keyValue,

    intNode,
    doubleNode,

    functionCall,

    notOp,

    trueKeyword,
    falseKeyword,

    parenthesisExpr,

    importKeyword,

    whitespace,
    newLine,

    literal,

    arrayCreateExpr,
    arrayAnonymousGetExpr,

    arrayGetExpr,

    quotationMark,
    colon,
    dot,
    comma,

    leftParenthesis,
    rightParenthesis,

    leftCurl,
    rightCurl,

    leftBracket,
    rightBracket,

    semicolon,

    funKeyword,

    returnKeyword,

    packageNode,
    literalExpr,
    stringExpr,
    negateExpr,
    returnExpr,
    whileExpr,
    parameterList,
    functionDeclaration,
    functionReturnType,
    functionBody,
    argument,
    argumentList,
    moduleDeclaration,
    modulePathDeclaration,
    importDeclaration,
    parameter,
    typeNode,
    structFieldDecl,
    structDecl,
    subOp,
    addOp,
    divOp,
    inverseDivOp,
    modOp,
    mulOp,
    greaterOp,
    greaterOrEqual,
    lesserOp,
    lesserOrEqual,
    powOp,
    assignOp,
    equalOp,
    notEqualOp,
    refAssignExpr,
    referenceExpression,

    arendOffset,

    eofToken,

    ifKeyword,
    elseKeyword,

    codeBlock,

    ifExpression,
    elseExpression,

    lamKeyword,
    lambdaDeclaration,

    orKeyword,
    andKeyword,

    whileKeyword,
    forKeyword,

    nullToken,

    packageKeyword
};

/**
 * Base Token class
 */
class Token {
protected:
    size_t startOffset;
    ElementType tokenType;

public:
    Token(int startOffset, ElementType tokenType);

    virtual ~Token() = default;

    /**
     * Get string representation of token
     *
     * @return string representation of token
     */
    virtual std::string getText() = 0;

    virtual void visit(TokenVisitor *tokenVisitor, VisitingContext *context) = 0;

    /**
     * Check if given token is a binary operator
     *
     * @return true for binary operator, false otherwise
     */
    virtual bool isBinOp() {
        return false;
    }

    bool isWhitespace() const;

    size_t getEndOffset() {
        return getStartOffset() + this->getText().size();
    }

    size_t getStartOffset() const {
        return startOffset;
    }

    ElementType getTokenType() const {
        return tokenType;
    }

    virtual bool isNewLine() {
        return getTokenType() == ElementType::newLine;
    }

    virtual bool isElseKeyword() {
        return getTokenType() == ElementType::elseKeyword;
    }

    virtual bool isLiteral() {
        return getTokenType() == ElementType::literal;
    }

    virtual bool isRightParenthesis() {
        return getTokenType() == ElementType::rightParenthesis;
    }

    virtual size_t width() {
        return getText().size();
    }
};

/**
 * Represents Null constant
 */
class NullToken : public Token {
public:
    NullToken(int startOffset) : Token(startOffset, ElementType::nullToken) {}

    std::string getText() override {
        return "Null";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override {}
};

/**
 * Represents '{' token
 */
class LeftCurl : public Token {
public:
    LeftCurl(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents '}' token
 */
class RightCurl : public Token {
public:
    RightCurl(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents '[' token
 */
class LeftBracket : public Token {
public:
    LeftBracket(int startOffset) : Token(startOffset, ElementType::leftBracket) {};

    std::string getText() override {
        return "[";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents ']' token
 */
class RightBracket : public Token {
public:
    RightBracket(int startOffset) : Token(startOffset, ElementType::rightBracket) {};

    std::string getText() override {
        return "]";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents new line token
 */
class NewLine : public Token {
public:
    NewLine(int startOffset);

    ~NewLine() {

    }

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents '.' token
 */
class Colon : public Token {
public:
    explicit Colon(int startOffset) : Token(startOffset, ElementType::colon) {};

    std::string getText() override {
        return ".";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents eof token
 */
class Eof : public Token {
public:
    Eof(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;

    void setStartOffset(int startOffset) {
        this->startOffset = startOffset;
    }

    size_t width() override {
        return 1;
    }
};

/**
 * Represents ' ' token
 */
class Space : public Token {
public:
    Space(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents '->' token
 */
class Arrow : public Token {
public:
    Arrow(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents ';' token
 */
class Semicolon : public Token {
public:
    Semicolon(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents '(' token
 */
class LeftParenthesis : public Token {
public:
    LeftParenthesis(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Represents ')' token
 */
class RightParenthesis : public Token {
public:
    RightParenthesis(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Dot : public Token {
public:
    Dot(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Comma : public Token {
public:
    Comma(int startOffset) : Token(startOffset, ElementType::comma) { }

    std::string getText() override {
        return ",";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Literal : public Token {
public:
    std::string content;

    Literal(int startOffset, std::string content);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;

    bool isLiteral() override {
        return true;
    }
};

class StructKeyword : public Token {
public:
    std::string content;

    StructKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class TraitKeyword : public Token {
public:
    std::string content;

    TraitKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class IntNumber : public Literal {
public:
    int value;

    IntNumber(int startOffset, std::string content, int value) : Literal(startOffset, std::move(content)) {
        this->value = value;
    };

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class DoubleNumber : public Literal {
public:
    double value;

    DoubleNumber(int startOffset, std::string content, double value) : Literal(startOffset, std::move(content)) {
        this->value = value;
    };

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class IsKeyword : public Token {
public:
    IsKeyword(int startOffset);

    std::string getText() override {
        return "is";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class InKeyword : public Token {
public:
    InKeyword(int startOffset);

    std::string getText() override {
        return "in";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class ContinueKeyword : public Token {
public:
    ContinueKeyword(int startOffset);

    std::string getText() override {
        return "continue";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class BreakKeyword : public Token {
public:
    BreakKeyword(int startOffset);

    std::string getText() override {
        return "break";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class ValKeyword : public Token {
public:
    ValKeyword(int startOffset);

    std::string getText() override {
        return "val";
    }
    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class VarKeyword : public Token {
public:
    VarKeyword(int startOffset);

    std::string getText() override {
        return "var";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class VarargsKeyword : public Token {
public:
    VarargsKeyword(int startOffset);

    std::string getText() override {
        return "varargs";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class RetKeyword : public Token {
public:
    RetKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class IfKeyword : public Token {
public:
    IfKeyword(int startOffset) : Token(startOffset, ElementType::ifKeyword) {};

    std::string getText() override {
        return "if";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class ElseKeyword : public Token {
public:
    ElseKeyword(int startOffset) : Token(startOffset, ElementType::elseKeyword) {};

    std::string getText() override {
        return "else";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class LamKeyword : public Token {
public:
    LamKeyword(int startOffset) : Token(startOffset, ElementType::lamKeyword) {}

    std::string getText() override {
        return "lam";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class FunKeyword : public Token {
public:
    FunKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class TrueKeyword : public Token {
public:
    TrueKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class FalseKeyword : public Token {
public:
    FalseKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class WhileKeyword : public Token {
public:
    WhileKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class ForKeyword : public Token {
public:
    ForKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class MetKeyword : public Token {
public:
    MetKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class PackageKeyword : public Token {
public:
    PackageKeyword(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class QuotionMark : public Token {
public:
    QuotionMark(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

/**
 * Base class for binary oeprators
 */
class BinOp : public Token {
public:
    BinOp(int startOffset, ElementType tokenType) : Token(startOffset, tokenType) {}

    bool isBinOp() override {
        return true;
    }

    bool hasHigherPrecedence(const std::string& other) {
        auto leftOp = OP_PRECEDENCE.find(getText());
        if (leftOp == OP_PRECEDENCE.end()) {
            throw ("Unsupported operator: " + getText()).c_str();
        }
        auto rightOp = OP_PRECEDENCE.find(other);
        if (rightOp == OP_PRECEDENCE.end()) {
            throw ("Unsupported operator: " + other).c_str();
        }
        return leftOp->second > rightOp->second;
    }
};

class OrKeyword : public BinOp {
public:
    OrKeyword(int startOffset);

    std::string getText() override {
        return "or";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class AndKeyword : public BinOp {
public:
    AndKeyword(int startOffset);

    std::string getText() override {
        return "and";
    }

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Div : public BinOp {
public:
    Div(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class InverseDiv : public BinOp {
public:
    InverseDiv(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Lesser : public BinOp {
public:
    Lesser(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class LesserOrEqual : public BinOp {
public:
    LesserOrEqual(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Greater : public BinOp {
public:
    Greater(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class GreaterOrEqual : public BinOp {
public:
    GreaterOrEqual(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Mul : public BinOp {
public:
    Mul(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Mod : public BinOp {
public:
    Mod(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class AssignOp : public BinOp {
public:
    AssignOp(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class EqualOp : public BinOp {
public:
    EqualOp(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class NotEqualOp : public BinOp {
public:
    NotEqualOp(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Pow : public BinOp {
public:
    Pow(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class NotOp : public BinOp {
public:
    NotOp(int startOffset) : BinOp(startOffset, ElementType::notOp) {}

    std::string getText() override {
        return "!";
    };

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Sub : public BinOp {
public:
    Sub(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class Add : public BinOp {
public:
    Add(int startOffset);

    std::string getText() override;

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;
};

class ImportKeyword : public Token {
public:
    ImportKeyword(int startOffset);

    void visit(TokenVisitor *tokenVisitor, VisitingContext *context) override;

    std::string getText() override;
};

enum CurrentMode {
    dslArguments
};

class VisitingContext {
public:
    Lexer* lexer;
    std::vector<CurrentMode> mode;
    explicit VisitingContext(Lexer *lexer);
};

class TokenVisitor {
public:
    virtual void visit(PackageKeyword *, VisitingContext *) {}

    virtual void visit(VarKeyword *, VisitingContext *) {}

    virtual void visit(ValKeyword *, VisitingContext *) {}

    virtual void visit(InKeyword *, VisitingContext *) {}

    virtual void visit(IsKeyword *, VisitingContext *) {}

    virtual void visit(VarargsKeyword *, VisitingContext *) {}

    virtual void visit(BreakKeyword *, VisitingContext *) {}

    virtual void visit(ContinueKeyword *, VisitingContext *) {}

    virtual void visit(GreaterOrEqual *, VisitingContext *) {}

    virtual void visit(LesserOrEqual *, VisitingContext *) {}

    virtual void visit(NotEqualOp *, VisitingContext *) {}

    virtual void visit(TrueKeyword *, VisitingContext *) {}

    virtual void visit(FalseKeyword *, VisitingContext *) {}

    virtual void visit(IntNumber *, VisitingContext *) {}

    virtual void visit(DoubleNumber *, VisitingContext *) {}

    virtual void visit(StructKeyword *, VisitingContext *) {}

    virtual void visit(TraitKeyword *, VisitingContext *) {}

    virtual void visit(NotOp *, VisitingContext *) {}

    virtual void visit(AndKeyword *, VisitingContext *) {}

    virtual void visit(OrKeyword *, VisitingContext *) {}

    virtual void visit(Lesser *, VisitingContext *) {}

    virtual void visit(Greater *, VisitingContext *) {}

    virtual void visit(Comma *, VisitingContext *) {}

    virtual void visit(Colon *, VisitingContext *) {}

    virtual void visit(Space *, VisitingContext *) {}

    virtual void visit(LeftParenthesis *, VisitingContext *) {}

    virtual void visit(RightParenthesis *, VisitingContext *) {}

    virtual void visit(Literal *, VisitingContext *) {}

    virtual void visit(FunKeyword *, VisitingContext *) {}

    virtual void visit(MetKeyword *, VisitingContext *) {}

    virtual void visit(RetKeyword *, VisitingContext *) {}

    virtual void visit(QuotionMark *, VisitingContext *) {}

    virtual void visit(NewLine *, VisitingContext *) {}

    virtual void visit(Semicolon *, VisitingContext *) {}

    virtual void visit(LeftCurl *, VisitingContext *) {}

    virtual void visit(RightCurl *, VisitingContext *) {}

    virtual void visit(Eof *, VisitingContext *);

    virtual void visit(Add *, VisitingContext *);

    virtual void visit(InverseDiv *, VisitingContext *);

    virtual void visit(Div *, VisitingContext *);

    virtual void visit(Mul *, VisitingContext *) {}

    virtual void visit(Mod *, VisitingContext *) {}

    virtual void visit(Sub *, VisitingContext *) {}

    virtual void visit(Pow *, VisitingContext *) {}

    virtual void visit(AssignOp *assignOp, VisitingContext *visitor) {}

    virtual void visit(Arrow *, VisitingContext *) {}

    virtual void visit(LeftBracket *, VisitingContext *) {}

    virtual void visit(RightBracket *, VisitingContext *) {}

    virtual void visit(IfKeyword *, VisitingContext *) {}

    virtual void visit(ElseKeyword *, VisitingContext *) {}

    virtual void visit(LamKeyword *, VisitingContext *) {}

    virtual void visit(Dot *, VisitingContext *) {}

    virtual void visit(WhileKeyword *, VisitingContext *) {}

    virtual void visit(ForKeyword *, VisitingContext *) {}

    virtual void visit(EqualOp *, VisitingContext *) {}

    virtual void visit(ImportKeyword *, VisitingContext *) {}
};

class ParseProblem {
public:
    int startOffset;
    int endOffset;
    std::string filePath;
    std::string message;

    ParseProblem(int startOffset,
                 int endOffset,
                 std::string filePath,
                 std::string message
                 ) : startOffset(startOffset), endOffset(endOffset), filePath(std::move(filePath)), message(std::move(message)) {}
};

#endif