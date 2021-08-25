//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#include "Parser.h"

#include <utility>
#include <vector>
#include <memory>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>


void ParserVisitor::visit(LeftBracket *lb, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    lb->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(RightBracket *rb, VisitingContext *ctx) {

}

void ParserVisitor::visit(LeftCurl *lc, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    lc->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(RightCurl *rc, VisitingContext *ctx) {

}

void ParserVisitor::visit(QuotionMark *l, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    l->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(TrueKeyword *tk, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    tk->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(FalseKeyword *fk, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    fk->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(NotOp *notOp, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    notOp->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(Space *space, VisitingContext *ctx) {
    ctx->lexer->nextToken()->visit(this, ctx);
}

void ParserVisitor::visit(LeftParenthesis *lp, VisitingContext *ctx) {
}

void ParserVisitor::visit(RightParenthesis *rp, VisitingContext *ctx) {
}

void ParserVisitor::visit(RetKeyword *returnKeyword, VisitingContext *ctx) {
}

void ParserVisitor::visit(Literal *l, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    l->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(IntNumber *l, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    l->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(DoubleNumber *l, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    l->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(Sub *s, VisitingContext *ctx) {
    ExpressionVisitor startExpressionVisitor;
    s->visit(&startExpressionVisitor, ctx);
    expressions.push_back(std::move(startExpressionVisitor.currentExpression));
}

void ParserVisitor::visit(WhileKeyword *whileKeyword, VisitingContext *ctx) {
    WhileLoopVisitor whileLoopVisitor;
    whileLoopVisitor.visit(whileKeyword, ctx);
    expressions.push_back(std::move(whileLoopVisitor.whileLoopExpression));
}

void ParserVisitor::visit(ForKeyword *forKeyword, VisitingContext *ctx) {
    ForLoopVisitor forLoopVisitor;
    forLoopVisitor.visit(forKeyword, ctx);
    expressions.push_back(std::move(forLoopVisitor.forLoopExpression));
}

void ParserVisitor::visit(IfKeyword *ifKeyword, VisitingContext *ctx) {
    IfVisitor ifVisitor;
    ifKeyword->visit(&ifVisitor, ctx);
    expressions.push_back(std::unique_ptr<IfExpression>(ifVisitor.conditionalExpression));
}

void ParserVisitor::visit(ImportKeyword *importKeyword, VisitingContext *ctx) {
    ExpressionVisitor expressionVisitor;
    importKeyword->visit(&expressionVisitor, ctx);
    this->imports.push_back(std::move(expressionVisitor.currentExpression));
}

void ParserVisitor::visit(FunKeyword *funKeyword, VisitingContext *ctx) {
    FunctionVisitor functionVisitor(funKeyword);
    functions.push_back(std::unique_ptr<FunctionDeclaration>(functionVisitor.visit(ctx)));
}

ModuleParser::ModuleParser(ParseContext* parseContext) {
    this->parseContext = parseContext;
}

void ModuleParser::parse() {
    ParserVisitor parserVisitor;
    auto lexer = this->parseContext->lexer;
    this->parseContext->parseVisitor = &parserVisitor;
    PackageNode* packageNode = nullptr;
    auto peeked = lexer->peekNext();
    if (peeked->getTokenType() != ElementType::packageKeyword) {
        this->syntaxExceptions.emplace_back("Expected package declaration",
                                            peeked->getStartOffset(),
                                            peeked->getEndOffset(),
                                            lexer->filePath);
        return;
    } else {
        auto keyword = lexer->nextToken();
        std::vector<Token*> children;
        children.push_back(keyword);
        while (lexer->hasNext()) {
            auto next = lexer->nextToken();
            if (next->isNewLine()) {
                children.push_back(next);
                break;
            } else {
                children.push_back(next);
            }
        }
        packageNode = new PackageNode(children);
    }
    while (lexer->hasNext()) {
        try {
            auto t = lexer->nextToken();
            t->visit(&parserVisitor, this->parseContext);
        } catch (SyntaxException &e) {
            e.printMessage();
            this->syntaxExceptions.push_back(e);
            return;
        }
    }

    std::vector<std::unique_ptr<ImportDeclaration>> imports;

    std::shared_ptr<ModuleDeclaration> module = std::make_shared<ModuleDeclaration>(
            "DUMMY",
            this->parseContext->lexer->filePath,
            std::move(imports),
            std::move(parserVisitor.functions),
            std::move(parserVisitor.types),
            new StaticBlock(std::move(parserVisitor.expressions))
            );

    this->parseContext->moduleDeclarations.push_back(module);
    this->parsed = true;
}

SyntaxException::SyntaxException(const char *const message,
                                 Token *token,
                                 std::string filePath): exception(message) {
    this->startOffset = token->getStartOffset();
    this->endOffset = token->getEndOffset();
    this->filePath = std::move(filePath);
    this->message = message;
}

SyntaxException::SyntaxException(const char *const message,
                                 size_t startOffset,
                                 size_t endOffset,
                                 std::string filePath): exception(message) {
    this->startOffset = startOffset;
    this->endOffset = endOffset;
    this->filePath = std::move(filePath);
    this->message = message;
}

inline static std::string spacer(size_t times) {
    std::string acc;
    for (size_t i=0; i<times; i++) {
        acc+=" ";
    }
    return acc;
}

inline static std::string repeat(size_t times, char ch) {
    std::string acc;
    for (size_t i=0; i<times; i++) {
        acc+=ch;
    }
    return acc;
}

void SyntaxException::printMessage() const {
    Lexer lexer(filePath);

    bool markerMode = false;
    std::string marker;
    std::string lineAcc;
    lineAcc += lexer.current;

    std::cerr << "Error in " + filePath << std::endl;

    while (lexer.hasNext()) {
        char ch = lexer.nextChar();
        lineAcc += ch;

        if (ch == '\n' || ch == '\r') {
            std::cerr << lineAcc;
            if (markerMode) {
                std::cerr << marker << std::endl;
                std::cerr << message << std::endl;
                markerMode = false;
            }
            marker.clear();
            lineAcc.clear();
        }

        if (lexer.offset == this->startOffset) {
            markerMode = true;
        }
        if (markerMode) {
            marker += "^";
        } else {
            if (ch == '\n' || ch == '\r') {
                marker += " ";
            }
        }
    }

/*
    std::cerr << "error in " + this->filePath << " at line: " << this->startRow << ", column: " << this->startCol << std::endl;
    std::cerr << std::endl;
    */
}

void ParserVisitor::visit(NewLine *newLine, VisitingContext *ctx) {
    delete newLine;
}