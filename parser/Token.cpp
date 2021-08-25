//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include <string>
#include <utility>

Token::Token(int startOffset, ElementType tokenType) {
    this->startOffset = startOffset;
    this->tokenType = tokenType;
}

bool Token::isWhitespace() const {
    return this->tokenType == ElementType::whitespace;
}

RightCurl::RightCurl(int startOffset) : Token(startOffset, ElementType::rightCurl) {}

std::string RightCurl::getText() {
    return "}";
}

void RightCurl::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

EqualOp::EqualOp(int startOffset) : BinOp(startOffset, ElementType::equalOp) {}

std::string EqualOp::getText() {
    return "==";
}

void EqualOp::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

NotEqualOp::NotEqualOp(int startOffset) : BinOp(startOffset, ElementType::notEqualOp) {}

std::string NotEqualOp::getText() {
    return "!=";
}

void NotEqualOp::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

AssignOp::AssignOp(int startOffset) : BinOp(startOffset, ElementType::assignOp) {}

/*
BinExpr* AssignOp::toBinExpr(Expression *left, Expression *right) {
    if (left->isLiteralExpr() && left->furtherExpression == nullptr) {
        return new SingleAssignExpr((LiteralExpr*) left, this, right);
    } else {
        std::shared_ptr<Expression> last = left->dropLast();
        auto* parent = (Expression*) last->getParent();
        parent->furtherExpression = nullptr;
        return new RefAssignExpr(left, last, this, right);
    }
}
*/
std::string AssignOp::getText() {
    return "=";
}

void AssignOp::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

LeftCurl::LeftCurl(int startOffset) : Token(startOffset, ElementType::leftCurl) {}

std::string LeftCurl::getText() {
    return "{";
}

void LeftCurl::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Eof::Eof(int startOffset) : Token(startOffset, ElementType::eofToken) {}

std::string Eof::getText() {
    return "end of file";
}

void Eof::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Space::Space(int startOffset) : Token(startOffset, ElementType::whitespace) {}

std::string Space::getText() {
    return " ";
}

void Space::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Sub::Sub(int startOffset) : BinOp(startOffset, ElementType::subOp) {}

std::string Sub::getText() {
    return "-";
}

void Sub::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Add::Add(int startOffset) : BinOp(startOffset, ElementType::addOp) {}

std::string Add::getText() {
    return "+";
}

void Add::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Lesser::Lesser(int startOffset) : BinOp(startOffset, ElementType::lesserOp) {}

std::string Lesser::getText() {
    return "<";
}

void Lesser::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

LesserOrEqual::LesserOrEqual(int startOffset) : BinOp(startOffset, ElementType::lesserOrEqual) {}

std::string LesserOrEqual::getText() {
    return "<=";
}

void LesserOrEqual::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Greater::Greater(int startOffset) : BinOp(startOffset, ElementType::greaterOp) {}

std::string Greater::getText() {
    return ">";
}

void Greater::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

GreaterOrEqual::GreaterOrEqual(int startOffset) : BinOp(startOffset, ElementType::greaterOrEqual) {}

std::string GreaterOrEqual::getText() {
    return ">=";
}

void GreaterOrEqual::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Mul::Mul(int startOffset) : BinOp(startOffset, ElementType::mulOp) {}

std::string Mul::getText() {
    return "*";
}

void Mul::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Mod::Mod(int startOffset) : BinOp(startOffset, ElementType::modOp) {}

std::string Mod::getText() {
    return "%";
}

void Mod::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void NotOp::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Pow::Pow(int startOffset) : BinOp(startOffset, ElementType::powOp) {}

std::string Pow::getText() {
    return "^";
}

void Pow::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

OrKeyword::OrKeyword(int startOffset) : BinOp(startOffset, ElementType::orKeyword) {}

void OrKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

AndKeyword::AndKeyword(int startOffset) : BinOp(startOffset, ElementType::andKeyword) {}

void AndKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Div::Div(int startOffset) : BinOp(startOffset, ElementType::divOp) {}

std::string Div::getText() {
    return "/";
}

void Div::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

InverseDiv::InverseDiv(int startOffset) : BinOp(startOffset, ElementType::inverseDivOp) {}

std::string InverseDiv::getText() {
    return "\\";
}

void InverseDiv::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Dot::Dot(int startOffset) : Token(startOffset, ElementType::dot) {}

std::string Dot::getText() {
    return ".";
}

void Dot::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Arrow::Arrow(int startOffset) : Token(startOffset, ElementType::arrow) {}

std::string Arrow::getText() {
    return "->";
}

void Arrow::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Semicolon::Semicolon(int startOffset) : Token(startOffset, ElementType::semicolon) {}

std::string Semicolon::getText() {
    return ";";
}

void Semicolon::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void Comma::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

NewLine::NewLine(int startOffset) : Token(startOffset, ElementType::newLine) {}

std::string NewLine::getText() {
    return "\n";
}

void NewLine::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

StructKeyword::StructKeyword(int startOffset) : Token(startOffset, ElementType::structKeyword) {}

void StructKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

std::string StructKeyword::getText() {
    return "struct";
}

ImportKeyword::ImportKeyword(int startOffset) : Token(startOffset, ElementType::importKeyword) {}

void ImportKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

std::string ImportKeyword::getText() {
    return "import";
}

FunKeyword::FunKeyword(int startOffset) : Token(startOffset, ElementType::funKeyword) {}

std::string FunKeyword::getText() {
    return "fun";
}

void FunKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

TrueKeyword::TrueKeyword(int startOffset) : Token(startOffset, ElementType::trueKeyword) {}

std::string TrueKeyword::getText() {
    return "true";
}

void TrueKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

FalseKeyword::FalseKeyword(int startOffset) : Token(startOffset, ElementType::falseKeyword) {}

std::string FalseKeyword::getText() {
    return "false";
}

void FalseKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

WhileKeyword::WhileKeyword(int startOffset) : Token(startOffset, ElementType::whileKeyword) {}

std::string WhileKeyword::getText() {
    return "while";
}

void WhileKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

ForKeyword::ForKeyword(int startOffset) : Token(startOffset, ElementType::forKeyword) {}

std::string ForKeyword::getText() {
    return "for";
}

void ForKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

MetKeyword::MetKeyword(int startOffset) : Token(startOffset, ElementType::metKeyword) {}

std::string MetKeyword::getText() {
    return "met";
}

void MetKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

PackageKeyword::PackageKeyword(int startOffset) : Token(startOffset, ElementType::packageKeyword) {
    this->startOffset = startOffset;
}

std::string PackageKeyword::getText() {
    return "package";
}

void PackageKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

VarKeyword::VarKeyword(int startOffset) : Token(startOffset, ElementType::varKeyword) {}

void VarKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

ValKeyword::ValKeyword(int startOffset) : Token(startOffset, ElementType::valKeyword) {}

void ValKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

IsKeyword::IsKeyword(int startOffset) : Token(startOffset, ElementType::isKeyword) {}

void IsKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

InKeyword::InKeyword(int startOffset) : Token(startOffset, ElementType::inKeyword) {}

void InKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

ContinueKeyword::ContinueKeyword(int startOffset) : Token(startOffset, ElementType::continueKeyword) {}

void ContinueKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

BreakKeyword::BreakKeyword(int startOffset) : Token(startOffset, ElementType::breakKeyword) {}

void BreakKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

VarargsKeyword::VarargsKeyword(int startOffset) : Token(startOffset, ElementType::varargsKeyword) {}

void VarargsKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

RetKeyword::RetKeyword(int startOffset) : Token(startOffset, ElementType::returnKeyword) {}

std::string RetKeyword::getText() {
    return "ret";
}

void RetKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void IfKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void ElseKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void LamKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

Literal::Literal(int startOffset, std::string content) : Token(startOffset, ElementType::literal) {
    this->content = std::move(content);
}

std::string Literal::getText() {
    return this->content;
}

void Literal::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

TraitKeyword::TraitKeyword(int startOffset) : Token(startOffset, ElementType::interfaceKeyword) {}

std::string TraitKeyword::getText() {
    return this->content;
}

void TraitKeyword::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void IntNumber::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void DoubleNumber::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

LeftParenthesis::LeftParenthesis(int startOffset) : Token(startOffset, ElementType::leftParenthesis) {}

std::string LeftParenthesis::getText() {
    return "(";
}

void LeftParenthesis::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

RightParenthesis::RightParenthesis(int startOffset) : Token(startOffset, ElementType::rightParenthesis) {}

std::string RightParenthesis::getText() {
    return ")";
}

void RightParenthesis::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void Colon::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

QuotionMark::QuotionMark(int startOffset) : Token(startOffset, ElementType::quotationMark) {}

std::string QuotionMark::getText() {
    return "\"";
}

void QuotionMark::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void LeftBracket::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

void RightBracket::visit(TokenVisitor *tokenVisitor, VisitingContext *context) {
    tokenVisitor->visit(this, context);
}

VisitingContext::VisitingContext(Lexer *lexer) : lexer(lexer) {}

static void unexpectedToken(Token* token, VisitingContext* visitingContext) {
    throw SyntaxException("Unexpected token", token, visitingContext->lexer->filePath);
}

void TokenVisitor::visit(Add *token, VisitingContext *context) {
    unexpectedToken(token, context);
}

void TokenVisitor::visit(Div *token, VisitingContext *context) {
    unexpectedToken(token, context);
}

void TokenVisitor::visit(InverseDiv *token, VisitingContext *context) {
    unexpectedToken(token, context);
}

void TokenVisitor::visit(Eof *, VisitingContext *) {

}