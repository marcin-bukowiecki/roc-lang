//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#pragma once
#ifndef ROC_LANG_PARSER_H
#define ROC_LANG_PARSER_H

#include <iostream>
#include <vector>
#include <memory>
#include <exception>

#include "Token.h"
#include "Lexer.h"
#include "AST.h"

class SyntaxException;
class ParseContext;

class ParserVisitor : public TokenVisitor {
public:
    std::vector<std::unique_ptr<FunctionDeclaration>> functions;
    std::vector<std::unique_ptr<Expression>> expressions;
    std::vector<std::unique_ptr<Expression>> imports;
    std::vector<std::unique_ptr<StructDeclaration>> types;

    void visit(TrueKeyword* tk, VisitingContext* ctx) override;
    void visit(FalseKeyword* fk, VisitingContext* ctx) override;
    void visit(NotOp* notOp, VisitingContext* ctx) override;
    void visit(Space* space, VisitingContext* ctx) override;
    void visit(LeftParenthesis* lp, VisitingContext* ctx) override;
    void visit(RightParenthesis* rp, VisitingContext* ctx) override;
    void visit(LeftBracket* lb, VisitingContext* ctx) override;
    void visit(RightBracket* lb, VisitingContext* ctx) override;
    void visit(LeftCurl* lc, VisitingContext* ctx) override;
    void visit(RightCurl* rc, VisitingContext* ctx) override;
    void visit(Literal* l, VisitingContext* ctx) override;
    void visit(IntNumber* l, VisitingContext* ctx) override;
    void visit(DoubleNumber* l, VisitingContext* ctx) override;
    void visit(WhileKeyword* whileKeyword, VisitingContext* ctx) override;
    void visit(ForKeyword* forKeyword, VisitingContext* ctx) override;
    void visit(FunKeyword* funKeyword, VisitingContext* ctx) override;
    void visit(RetKeyword* returnKeyword, VisitingContext* ctx) override;
    void visit(IfKeyword* ifKeyword, VisitingContext* ctx) override;
    void visit(QuotionMark* l, VisitingContext* ctx) override;
    void visit(ImportKeyword* ik, VisitingContext* ctx) override;
    void visit(Sub* s, VisitingContext* ctx) override;
    void visit(NewLine* s, VisitingContext* ctx) override;
};

/**
 * Parser for parsing Module which is main compilation unit
 */
class ModuleParser {
private:
    /**
     * Flag to indicated that this Module parser was used
     */
    bool parsed = false;

public:
    /**
     * Given parsing cotnext to hold some usefull information
     */
    ParseContext* parseContext;

    /**
     * Given module name
     */
    std::string moduleName;

    std::string absolutePath;

    /**
     * Vector for syntax exceptions
     */
    std::vector<SyntaxException> syntaxExceptions;

	explicit ModuleParser(ParseContext* parseContext);

	void parse();
};

class ParseContext : public VisitingContext {
public:
	std::vector<std::shared_ptr<ModuleDeclaration>> moduleDeclarations{};
	std::vector<ParseProblem> problems{};
    ParserVisitor* parseVisitor{};
    explicit ParseContext(Lexer* lexer): VisitingContext(lexer) {}
    void registerProblem(ParseProblem parseProblem) {
        problems.push_back(parseProblem);
    }
};

class TupleParser : public TokenVisitor {
public:
    std::vector<std::unique_ptr<Expression>> expressions;

    void visit(LeftParenthesis* lp, VisitingContext* ctx) override {
        auto lexer = ctx->lexer;
        while (lexer->hasNext()) {
            auto* next = lexer->nextToken();
            ExpressionVisitor startExpressionVisitor;
            next->visit(&startExpressionVisitor, ctx);
            expressions.push_back(std::move(startExpressionVisitor.currentExpression));
            if (lexer->currentToken->getTokenType() == ElementType::rightParenthesis) {
                break;
            }
        }
    }
};

class ArrayCreateExprVisitor {
public:
    std::vector<std::unique_ptr<Expression>> expressions;
    std::vector<std::unique_ptr<Comma>> commas;

    void visit(VisitingContext* ctx) {
        auto lexer = ctx->lexer;
        while (lexer->hasNext()) {
            auto t = lexer->nextToken();
            if (t->getTokenType() == ElementType::rightBracket) {
                break;
            }
            ExpressionVisitor expressionVisitor;
            t->visit(&expressionVisitor, ctx);
            expressions.push_back(std::move(expressionVisitor.currentExpression));
            if (lexer->currentToken->getTokenType() == ElementType::rightBracket) {
                break;
            }
            if (lexer->currentToken->getTokenType() == ElementType::comma) {
                commas.push_back(std::unique_ptr<Comma>((Comma*) lexer->currentToken));
            }
        }
    }
};

class SyntaxException : public std::exception {
public:
    size_t startOffset;
    size_t endOffset;
    std::string filePath;
    std::string message;

    SyntaxException(const char *message,
                    Token *token,
                    VisitingContext* ctx): SyntaxException(message, token, ctx->lexer->filePath) { }

    SyntaxException(const char *message, Token *token, std::string filePath);

    SyntaxException(const char *message,
                    size_t startOffset,
                    size_t endOffset,
                    std::string filePath);

    virtual void printMessage() const;
};

#endif

