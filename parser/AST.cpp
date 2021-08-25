//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#include "AST.h"
#include "Parser.h"

static void reportSytnaxError(Token *token, Lexer *lexer) {
    throw SyntaxException(("Expected '}' got '" + token->getText() + "'").c_str(), token, lexer->filePath);
}

/**
 * Check if next token exists.
 * Throw syntax exception for eof or nullptr
 *
 * @param prev prev token
 * @param nextToken possible next token
 * @param lexer given lexer
 * @return next token
 */
static Token *checkNextToken(Token *prev, Token *nextToken, Lexer *lexer) {
    if (nextToken == nullptr || nextToken->getTokenType() == eofToken) {
        throw SyntaxException("Expected token", prev, lexer->filePath);
    }
    return nextToken;
}

static void validateToken(Token *current,
                          ElementType expected,
                          const std::string &expectedSymbol,
                          VisitingContext *visitingContext) {

    if (current->getTokenType() != expected) {
        throw SyntaxException(("Unexpected token. Expected: " + expectedSymbol).c_str(),
                              current,
                              visitingContext->lexer->filePath);
    }
}

static bool isNewLine(ExpressionVisitor *expressionVisitor) {
    return expressionVisitor->currentExpression &&
    !expressionVisitor->currentExpression->getWhiteSpaces().empty() &&
    expressionVisitor->currentExpression->getWhiteSpaces().back()->isNewLine();
}

/**
 * Go further in expression
 *
 * @param visitor given expression visitor
 * @param expr
 * @param ctx
 */
static void goFurther(ExpressionVisitor *visitor,
                      VisitingContext *ctx,
                      bool goNext = true) {
    auto peeked = ctx->lexer->peekNext();
    switch (peeked->getTokenType()) {
        case funKeyword:
        case rightCurl:
            return;
        case returnKeyword:
            if (!isNewLine(visitor)) {
                throw new SyntaxException("Expected new line before given token", peeked, ctx);
            }
            return;
        default:
            break;
    }

    if (ctx->lexer->peekNext()->getTokenType() == ElementType::rightCurl) {
        return;
    }
    if (goNext) {
        ctx->lexer->nextToken()->visit(visitor, ctx);
    } else {
        ctx->lexer->currentToken->visit(visitor, ctx);
    }
}

/**
 * Visits given binary operator expression
 *
 * @param expressionVisitor
 * @param binOp given binary operator
 * @param ctx given visiting context
 */
std::unique_ptr<Expression> OpExpressionVisitor::visit(VisitingContext *ctx) const {
    auto lexer = ctx->lexer;
    auto *t = lexer->nextToken();

    RightHandExpressionVisitor v;
    t->visit(&v, ctx); //right hand

    auto right = std::move(v.currentExpression);

    if (this->binOp->getTokenType() == ElementType::lesserOp && lexer->currentToken->getTokenType() == ElementType::greaterOp) {
        auto literal = std::move(((LiteralExpr*) right.get())->literal);
        auto typeNode = new SingleTypeNode(literal.release());
        this->leftHand->typeVariables.push_back(typeNode);
        return std::unique_ptr<Expression>(this->leftHand);
    }

    if (!lexer->currentToken->isBinOp()) {
        ToBinExprVisitor binExprVisitor(std::unique_ptr<Expression>(this->leftHand), std::move(right));
        binOp->visit(&binExprVisitor, ctx);
        return std::move(binExprVisitor.binExpr);
    } else {
        auto *currentBinOp = (BinOp *) lexer->currentToken;
        if (binOp->hasHigherPrecedence(currentBinOp->getText())) {
            ToBinExprVisitor binExprVisitor(std::unique_ptr<Expression>(this->leftHand), std::move(right));
            binOp->visit(&binExprVisitor, ctx);
            auto currentExpression = std::move(binExprVisitor.binExpr);
            OpExpressionVisitor opExpressionVisitor(currentExpression.release(), currentBinOp);
            auto result = opExpressionVisitor.visit(ctx);
            return std::move(result);
        } else {
            OpExpressionVisitor newRightHandVisitor(right.release(), currentBinOp);
            auto newRightHand = newRightHandVisitor.visit(ctx);

            ToBinExprVisitor binExprVisitor(std::unique_ptr<Expression>(this->leftHand), std::move(newRightHand));
            binOp->visit(&binExprVisitor, ctx);
            return std::move(binExprVisitor.binExpr);
        }
    }
}

/**
 * Visits function call node i.e. expr(foo, bar).expr etc.
 *
 * @param fc given node
 * @param ctx given compilation context
 */
void ASTVisitor::visit(FunctionCallNode *fc) {
    fc->argumentList->accept(this);
}

/**
 * Constructor for creating module
 *
 * @param moduleName module name
 * @param imports
 * @param functions
 * @param structs
 * @param staticBlock
 */
ModuleDeclaration::ModuleDeclaration(std::string moduleName,
                                     std::string absolutePath,
                                     std::vector<std::unique_ptr<ImportDeclaration>> imports,
                                     std::vector<std::unique_ptr<FunctionDeclaration>> functions,
                                     std::vector<std::unique_ptr<StructDeclaration>> types,
                                     StaticBlock* staticBlock) : ASTNode(
        ElementType::moduleDeclaration) {

    this->moduleName = std::move(moduleName);
    this->absolutePath = std::move(absolutePath);
    this->staticBlock = staticBlock;
    this->types = std::move(types);
    this->imports = std::move(imports);
    this->functions = std::move(functions);
    if (this->staticBlock) this->staticBlock->setParent(this);
    for (const auto &f: this->functions) {
        f->setParent(this);
    }
    for (const auto &imp: this->imports) {
        imp->setParent(this);
    }
    for (const auto &t: this->types) {
        t->setParent(this);
    }
}

/**
 * Visits module declaration
 *
 * @param visitor given visitor
 * @param ctx given compilation context
 */
void ModuleDeclaration::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

/**
 * Constructor for boolean True
 *
 * @param tk given True keyword
 */
TrueExpr::TrueExpr(std::unique_ptr<TrueKeyword> le) : Expression(ElementType::trueKeyword) {
    this->le = std::move(le);
}

/**
 * Visits true expr
 *
 * @param v given visitor
 * @param ctx given compilation context
 */
void TrueExpr::accept(ASTVisitor *v) {
    v->visit(this);
}

/**
 * Constructor for boolean False
 *
 * @param tk given False keyword
 */
FalseExpr::FalseExpr(std::unique_ptr<FalseKeyword> le) : Expression(ElementType::falseKeyword) {
    this->le = std::move(le);
}

/**
 * Visits false expr
 *
 * @param v given visitor
 * @param ctx given compilation context
 */
void FalseExpr::accept(ASTVisitor *v) {
    v->visit(this);
}

ImportDeclaration::ImportDeclaration(std::unique_ptr<Token> importKeyword,
                                     std::unique_ptr<Expression> expr,
                                     std::unique_ptr<Semicolon> semicolon) : Expression(
        ElementType::importDeclaration) {

    this->importKeyword = std::move(importKeyword);
    this->expr = std::move(expr);
    this->semicolon = std::move(semicolon);
}

void ImportDeclaration::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string ImportDeclaration::getText() {
    return importKeyword->getText() + " " + this->expr->getText() + semicolon->getText() + "\n";
}

/**
 * Visitor for ReferenceExpression
 *
 * @param visitor given visitor
 * @param ctx given context
 */
void ReferenceExpression::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

TypeNode::TypeNode() : ASTNode(ElementType::typeNode) {}

void TypeNode::accept(ASTVisitor *) {

}

SingleTypeNode::SingleTypeNode(Token* literal) : TypeNode() {
    this->literal = literal;
}

void SingleTypeNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

ArrayTypeNode::ArrayTypeNode(std::unique_ptr<LeftBracket> l,
                             std::unique_ptr<RightBracket> r,
                             std::unique_ptr<TypeNode> singleTypeNode) : TypeNode() {
    this->l = std::move(l);
    this->r = std::move(r);
    this->singleTypeNode = std::move(singleTypeNode);
    this->singleTypeNode->setParent(this);
}

void ArrayTypeNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

TypeParametersList::TypeParametersList(std::unique_ptr<Lesser> lesser,
                                       std::vector<std::unique_ptr<TypeNode>> types,
                                       std::unique_ptr<Greater> greater) {
    this->lesser = std::move(lesser);
    this->greater = std::move(greater);
    this->types = std::move(types);
    for (auto &t: this->types) {
        t->setParent(this);
    }
}

void TypeParametersList::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

StructFieldDeclaration::StructFieldDeclaration(std::unique_ptr<Token> name) : ASTNode(ElementType::structFieldDecl) {
    this->name = std::move(name);
}

void StructFieldDeclaration::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

/**
 * Constructor for struct declaration
 *
 * @param keyword
 * @param name
 * @param fields
 */
StructDeclaration::StructDeclaration(std::unique_ptr<Token> keyword,
                                     std::unique_ptr<Token> name,
                                     std::vector<std::unique_ptr<StructFieldDeclaration>> fields) : ASTNode(
        ElementType::structDecl) {
    this->keyword = std::move(keyword);
    this->name = std::move(name);
    this->fields = std::move(fields);
}

void StructDeclaration::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void ParenthesisExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void NumberNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

IntNode::IntNode(std::unique_ptr<IntNumber> value) : NumberNode(ElementType::intNode) {
    this->value = std::move(value);
};

void IntNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

DoubleNode::DoubleNode(std::unique_ptr<DoubleNumber> value) : NumberNode(ElementType::doubleNode) {
    this->value = std::move(value);
}

void DoubleNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void ParameterList::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

size_t ParameterList::size() const {
    return this->parameters.size();
}

TypeNode *TypeNodeVisitor::visit(VisitingContext *ctx) {
    this->start->visit(this, ctx);
    return this->result;
}

void TypeNodeVisitor::visit(LeftBracket *leftBracket, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    auto r = lexer->nextToken();
    TypeNodeVisitor typeNodeVisitor(lexer->nextToken());
    auto *inner = typeNodeVisitor.visit(ctx);
    this->result = new ArrayTypeNode(std::unique_ptr<LeftBracket>(leftBracket),
                                     std::unique_ptr<RightBracket>((RightBracket *) r),
                                     std::unique_ptr<TypeNode>(inner));
}

void TypeNodeVisitor::visit(Literal *literal, VisitingContext *ctx) {
    this->result = new SingleTypeNode(literal);
}

FunctionDeclaration *FunctionVisitor::visit(VisitingContext *ctx) const {
    auto lexer = ctx->lexer;
    auto fName = lexer->nextTokenSkipNL();
    if (!fName->isLiteral()) {
        std::string msg = "Expected function name, got: '" + fName->getText() + "'";
        throw SyntaxException(msg.c_str(), fName, lexer->filePath);
    }

    auto next = lexer->nextTokenSkipNL();
    if (next->getTokenType() != ElementType::leftParenthesis) {
        std::string msg = "Expected '(', got: '" + next->getText() + "'";
        throw SyntaxException(msg.c_str(), next, lexer->filePath);
    }

    ParameterListVisitor plv((LeftParenthesis *) next);
    plv.visit(ctx);

    FunctionReturnTypeNode *functionReturnTypeNode;
    next = lexer->nextTokenSkipNL();
    if (next->getTokenType() == ElementType::arrow) {
        auto arrow = (Arrow *) next;
        next = lexer->nextTokenSkipNL();
        TypeNodeVisitor typeNodeVisitor(next);
        typeNodeVisitor.visit(ctx);
        functionReturnTypeNode = new FunctionNonVoidReturnTypeNode(arrow, typeNodeVisitor.result);
        next = lexer->nextTokenSkipNL();
    } else if (next->getTokenType() != ElementType::leftCurl) {
        throw SyntaxException("Expected '->' with return type or '{' before function body",
                              next,
                              lexer->filePath);
    } else {
        functionReturnTypeNode = new FunctionVoidReturnTypeNode();
    }


    auto leftCurl = (LeftCurl *) next;
    FunctionBodyVisitor fbv;
    fbv.visit(ctx);
    auto body = std::make_unique<FunctionBody>(std::move(fbv.expressions));
    auto pl = std::make_unique<ParameterList>(plv.token,
                                              std::move(plv.parameters),
                                              plv.end);

    return new FunctionDeclaration(
            this->funKeyword,
            fName,
            std::move(pl),
            std::unique_ptr<FunctionReturnTypeNode>(functionReturnTypeNode),
            leftCurl,
            std::move(body),
            (RightCurl *) lexer->nextToken()
    );
}

FunctionDeclaration::FunctionDeclaration(FunKeyword *keyword,
                                         Token *name,
                                         std::unique_ptr<ParameterList> parameterList,
                                         std::unique_ptr<FunctionReturnTypeNode> functionReturnTypeNode,
                                         LeftCurl *leftCurl,
                                         std::unique_ptr<FunctionBody> body,
                                         RightCurl *rightCurl) : ASTNode(ElementType::functionDeclaration) {

    this->keyword = keyword;
    this->name = name;
    this->parameterList = std::move(parameterList);
    this->functionReturnTypeNode = std::move(functionReturnTypeNode);
    this->body = std::move(body);
    this->parameterList->nodeContext->parent = this;
    this->body->nodeContext->parent = this;
    this->leftCurl = leftCurl;
    this->rightCurl = rightCurl;
}

void FunctionDeclaration::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

Token *FunctionDeclaration::getName() const {
    return name;
}

void FunctionBody::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void FunctionBody::replaceChild(Expression *old, std::unique_ptr<Expression> with) {
    int i = 0;
    for (const auto &expr: this->expressions) {
        if (old == expr.get()) {
            break;
        }
        i++;
    }
    this->expressions.at(i) = std::move(with);
}

CodeBlock::CodeBlock(std::vector<std::unique_ptr<Expression>> expressions) : ASTNode(ElementType::codeBlock) {
    this->expressions = std::move(expressions);
    for (const auto &expr: this->expressions) {
        expr->setParent(this);
    }
}

void CodeBlock::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string CodeBlock::getText() {
    std::string acc;
    for (const auto &expr: expressions) {
        acc += expr->getText();
    }
    return acc;
}

void CodeBlock::replaceChild(Expression *old, std::unique_ptr<Expression> with) {
    int i = 0;
    for (const auto &expr: this->expressions) {
        if (expr.get() == old) {
            this->expressions.at(i) = std::move(with);
            break;
        }
        i++;
    }
}

AddExpr::AddExpr(std::unique_ptr<Expression> l,
                 std::unique_ptr<Token> add,
                 std::unique_ptr<Expression> r) : BinExpr(std::move(l), std::move(add), std::move(r),
                                                          ElementType::addOp) {}

void AddExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

DivExpr::DivExpr(std::unique_ptr<Expression> l,
                 std::unique_ptr<Token> div,
                 std::unique_ptr<Expression> r) : BinExpr(std::move(l), std::move(div), std::move(r),
                                                          ElementType::divOp) {}

void DivExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

SubExpr::SubExpr(std::unique_ptr<Expression> l,
                 std::unique_ptr<Token> sub,
                 std::unique_ptr<Expression> r) : BinExpr(std::move(l), std::move(sub), std::move(r),
                                                          ElementType::subOp) {}

void SubExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

MulExpr::MulExpr(std::unique_ptr<Expression> l,
                 std::unique_ptr<Token> mul,
                 std::unique_ptr<Expression> r) : BinExpr(std::move(l), std::move(mul), std::move(r),
                                                          ElementType::mulOp) {}

void MulExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

InverseDivExpr::InverseDivExpr(std::unique_ptr<Expression> l,
                               std::unique_ptr<Token> inverseDiv,
                               std::unique_ptr<Expression> r) : BinExpr(std::move(l), std::move(inverseDiv),
                                                                        std::move(r), ElementType::inverseDivOp) {}

void InverseDivExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

PowExpr::PowExpr(std::unique_ptr<Expression> l,
                 std::unique_ptr<Token> pow,
                 std::unique_ptr<Expression> r) : BinExpr(std::move(l), std::move(pow), std::move(r),
                                                          ElementType::powOp) {}

void PowExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

ModExpr::ModExpr(std::unique_ptr<Expression> l,
                 std::unique_ptr<Token> mod,
                 std::unique_ptr<Expression> r) : BinExpr(std::move(l), std::move(mod), std::move(r),
                                                          ElementType::modOp) {}

void ModExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void NotExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

AndExpr::AndExpr(std::unique_ptr<Expression> l, std::unique_ptr<Token> andKeyword, std::unique_ptr<Expression> r) :
        BinExpr(std::move(l), std::move(andKeyword), std::move(r), ElementType::andKeyword) {}

void AndExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}


OrExpr::OrExpr(std::unique_ptr<Expression> l, std::unique_ptr<Token> orKeyword, std::unique_ptr<Expression> r) :
        BinExpr(std::move(l), std::move(orKeyword), std::move(r), ElementType::orKeyword) {}

void OrExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

LesserExpr::LesserExpr(std::unique_ptr<Expression> l, std::unique_ptr<Token> lesser, std::unique_ptr<Expression> r) :
        BinExpr(std::move(l), std::move(lesser), std::move(r), ElementType::lesserOp) {}

void LesserExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

LesserOrEqualExpr::LesserOrEqualExpr(std::unique_ptr<Expression> l, std::unique_ptr<Token> lesser,
                                     std::unique_ptr<Expression> r) :
        BinExpr(std::move(l), std::move(lesser), std::move(r), ElementType::lesserOrEqual) {}

void LesserOrEqualExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

GreaterExpr::GreaterExpr(std::unique_ptr<Expression> l, std::unique_ptr<Token> greater, std::unique_ptr<Expression> r) :
        BinExpr(std::move(l), std::move(greater), std::move(r), ElementType::greaterOp) {}

void GreaterExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

GreaterOrEqualExpr::GreaterOrEqualExpr(std::unique_ptr<Expression> l, std::unique_ptr<Token> greater,
                                       std::unique_ptr<Expression> r) :
        BinExpr(std::move(l), std::move(greater), std::move(r), ElementType::greaterOrEqual) {}

void GreaterOrEqualExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void RefAssignExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

NegativeExpr::NegativeExpr(std::unique_ptr<Token> sub, std::unique_ptr<Expression> r) : Expression(
        ElementType::negateExpr) {
    this->sub = std::move(sub);
    this->expression = std::move(r);
    this->expression->setParent(this);
}

void NegativeExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string NegativeExpr::getText() {
    return "-" + this->expression->getText();
}

StringNode::StringNode(Token *l, std::vector<Token *> tokens, Token *r) : Expression(ElementType::stringExpr) {
    this->lqm = l;
    this->tokens = std::move(tokens);
    this->rqm = r;
}

void StringNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

LiteralExpr::LiteralExpr(std::unique_ptr<Literal> token) : Expression(ElementType::literalExpr) {
    this->literal = std::move(token);
}

void LiteralExpr::accept(ASTVisitor *visitor) {
    visitor->visit((LiteralExpr *) this);
}

ReturnExpression::ReturnExpression(std::unique_ptr<Token> returnKeyword, std::unique_ptr<Expression> expression)
        : Expression(ElementType::returnExpr) {
    this->returnKeyword = std::move(returnKeyword);
    this->expression = std::move(expression);
    this->expression->setParent(this);
}

void ReturnExpression::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string ReturnExpression::getText() {
    return this->returnKeyword->getText() + " " + this->expression->getText();
}

IfExpression::IfExpression(IfKeyword *ifKeyword,
                           std::unique_ptr<Expression> expression,
                           Token *leftCurl,
                           std::unique_ptr<CodeBlock> block,
                           Token *rightCurl) : Expression(ElementType::ifExpression),
                                                                             ifKeyword(ifKeyword),
                                                                             expression(std::move(expression)),
                                                                             leftCurl(leftCurl),
                                                                             block(std::move(block)),
                                                                             rightCurl(rightCurl) {
    this->expression->setParent(this);
    this->block->setParent(this);
}

void IfExpression::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string IfExpression::getText() {
    return "if " + this->expression->getText();
}

ElseExpression::ElseExpression(ElseKeyword *elseKeyword,
                               Token *leftCurl,
                               std::unique_ptr<CodeBlock> block,
                               Token *rightCurl) : Expression(ElementType::elseExpression),
                                                   elseKeyword(std::move(elseKeyword)),
                                                   leftCurl(leftCurl),
                                                   block(std::move(block)),
                                                   rightCurl(rightCurl) {
    this->block->setParent(this);
}

void ElseExpression::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string ElseExpression::getText() {
    std::string acc = elseKeyword->getText();
    acc += leftCurl->getText();
    acc += block->getText();
    acc += rightCurl->getText();
    return acc;
}

ArgumentList::ArgumentList(LeftParenthesis *lp,
                           std::vector<std::unique_ptr<ArgumentExpression>> arguments,
                           RightParenthesis *rp) : ASTNode(ElementType::argumentList) {
    this->lp = lp;
    this->rp = rp;
    this->arguments = std::move(arguments);
    for (const auto &arg: this->arguments) {
        arg->setParent(this);
    }
}

void ArgumentList::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

size_t ArgumentList::size() const {
    return this->arguments.size();
}

Parameter::Parameter(Token* name, TypeNode* typeNode) : ASTNode(ElementType::parameter) {
    this->name = name;
    this->typeNode = typeNode;
}

void Parameter::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

ArgumentExpression::ArgumentExpression(std::unique_ptr<Expression> expression) : ASTNode(ElementType::argument) {
    this->expression = std::move(expression);
    this->expression->setParent(this);
}

void ArgumentExpression::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void ArgumentExpression::replaceChild(Expression *old, std::unique_ptr<Expression> with) {
    this->expression = std::move(with);
}

std::string ArgumentExpression::getText() {
    return this->expression->getText();
}

Expression::Expression(ElementType elementType) : ASTNode(elementType) {}

FunctionCallNode::FunctionCallNode(std::unique_ptr<LiteralExpr> literalExpr,
                                   std::unique_ptr<ArgumentList> argumentList) : Expression(ElementType::functionCall) {
    this->literalExpr = std::move(literalExpr);
    this->argumentList = std::move(argumentList);
    this->argumentList->setParent(this);
    this->literalExpr->setParent(this);
    this->functionName = this->literalExpr->literal->getText();
}

void FunctionCallNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string FunctionCallNode::getText() {
    return this->literalExpr->getText() + "(" + argumentList->getText() + ")";
}

std::string FunctionCallNode::getName() {
    return this->literalExpr->getText();
}

size_t FunctionCallNode::getStartOffset() {
    return this->literalExpr->literal->getStartOffset();
}

size_t FunctionCallNode::getEndOffset() {
    return this->argumentList->rp->getEndOffset();
}

void ArgumentsVisitor::visit(VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    while (lexer->hasNext()) {
        auto t = lexer->nextToken();
        if (t->getTokenType() == ElementType::rightParenthesis) {
            break;
        }
        ExpressionVisitor v;
        t->visit(&v, ctx);
        this->arguments.push_back(std::make_unique<ArgumentExpression>(std::move(v.currentExpression)));
        if (lexer->currentToken->getTokenType() == ElementType::rightParenthesis) {
            break;
        }
    }
}

void DslArgumentsVisitor::visit(VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    while (lexer->hasNext()) {
        auto t = lexer->currentToken;
        ExpressionVisitor v;
        t->visit(&v, ctx);
        this->arguments.push_back(std::make_unique<ArgumentExpression>(std::move(v.currentExpression)));
        if (lexer->currentToken->getTokenType() == ElementType::literal) {
            break;
        }
    }
}

void EqualOpExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void NotEqualOpExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void ExpressionVisitor::visit(LeftCurl *leftCurl, VisitingContext *ctx) {

}

void ExpressionVisitor::visit(NewLine *newLine, VisitingContext *ctx) {
    if (this->currentExpression) {
        this->currentExpression->addWhiteSpace(newLine);
    } else {
        delete newLine;
    }
    goFurther(this, ctx);
}

/**
 * Visit left bracket in expression
 *
 * @param leftBracket
 * @param ctx
 */
void ExpressionVisitor::visit(LeftBracket *leftBracket, VisitingContext *ctx) {/*
    auto lexer = ctx->lexer;
    // [..,..,..,][3]
    if (this->lastExpression && this->lastExpression->getNodeType() == ElementType::arrayCreateExpr) {
        StartExpressionVisitor expressionVisitor;
        lexer->nextTokenSkipWS()->visit(&expressionVisitor, ctx);
        auto expr = std::make_unique<ArrayAnonymousGetExpr>(std::unique_ptr<Token>(leftBracket),
                                                            std::move(expressionVisitor.currentExpression),
                                                            std::unique_ptr<Token>(lexer->currentToken));
        goFurther(this, std::move(expr), ctx);
        return;
    }

    // a = [..,..,..] a[2]
    if (this->lastExpression && (this->lastExpression->getNodeType() == ElementType::literalExpr ||
                                 this->lastExpression->getNodeType() == ElementType::arrayGetExpr)) {
        StartExpressionVisitor expressionVisitor;
        lexer->nextToken()->visit(&expressionVisitor, ctx);
        auto expr = std::make_unique<ArrayGetExpr>(std::unique_ptr<Token>(leftBracket),
                                                   std::move(expressionVisitor.currentExpression),
                                                   std::unique_ptr<Token>(lexer->currentToken));
        goFurther(this, std::move(expr), ctx);
        return;
    }

    ArrayCreateExprVisitor arrayCreateExprVisitor;
    arrayCreateExprVisitor.visit(ctx);
    auto expr = std::make_unique<ArrayCreateExpr>(
            std::unique_ptr<Token>(leftBracket),
            std::move(arrayCreateExprVisitor.expressions),
            std::unique_ptr<Token>(lexer->currentToken));
    goFurther(this, std::move(expr), ctx);*/
}


void ExpressionVisitor::visit(QuotionMark *qm, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    StringVisitor sv;
    sv.visit(qm, ctx);
    this->currentExpression = std::move(sv.conditionalExpression);
    goFurther(this, ctx);
}

void ExpressionVisitor::visit(ForKeyword *forKeyword, VisitingContext *ctx) {
    forKeyword->visit(this, ctx);
}

void ExpressionVisitor::visit(TrueKeyword *l, VisitingContext *ctx) {
    if (this->currentExpression) throw SyntaxException("Unexpected token", l, ctx);
    auto lexer = ctx->lexer;
    this->currentExpression = std::make_unique<TrueExpr>(std::unique_ptr<TrueKeyword>(l));
}

void ExpressionVisitor::visit(FalseKeyword *l, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    this->currentExpression = std::make_unique<FalseExpr>(std::unique_ptr<FalseKeyword>(l));
}

void ExpressionVisitor::visit(Literal *l, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    this->currentExpression = std::make_unique<LiteralExpr>(std::unique_ptr<Literal>(l));
    goFurther(this, ctx);
}

void ExpressionVisitor::visit(IntNumber *intNumber, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    this->currentExpression = std::make_unique<IntNode>(std::unique_ptr<IntNumber>(intNumber));
    auto next = lexer->nextToken();
    if (!ctx->mode.empty() && ctx->mode.back() == CurrentMode::dslArguments) {
        if (next->getTokenType() == ElementType::literal) {
            return;
        }
    }
    next->visit(this, ctx);
}

void ExpressionVisitor::visit(DoubleNumber *doubleNumber, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    this->currentExpression = std::make_unique<DoubleNode>(std::unique_ptr<DoubleNumber>(doubleNumber));
    auto next = lexer->nextToken();
    next->visit(this, ctx);
}

void ExpressionVisitor::visit(Sub *sub, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), sub);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Div *div, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), div);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Mul *mul, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), mul);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Pow *pow, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), pow);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Mod *mod, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), mod);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(AssignOp *assign, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), assign);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Greater *greater, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), greater);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(EqualOp *equalOp, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), equalOp);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(OrKeyword *orKeyword, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), orKeyword);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(AndKeyword *andKeyword, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), andKeyword);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Lesser *lesser, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), lesser);
    this->currentExpression = opExpressionVisitor.visit(ctx);
    if (this->currentExpression->isLiteralExpr()) {
        goFurther(this, ctx);
    }
}

void ExpressionVisitor::visit(LesserOrEqual *lesser, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), lesser);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(GreaterOrEqual *greater, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), greater);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Dot *dot, VisitingContext *ctx) {
    RightHandExpressionVisitor v;
    ctx->lexer->nextToken()->visit(&v, ctx); //right hand
    this->currentExpression = std::make_unique<ReferenceExpression>(
            this->currentExpression.release(),
            dot,
            v.currentExpression.release()
    );
    goFurther(this, ctx, false);
}

void ExpressionVisitor::visit(LeftParenthesis *lp, VisitingContext *ctx) {
    if (this->currentExpression->isLiteralExpr()) {
        ArgumentsVisitor argumentsVisitor;
        argumentsVisitor.visit(ctx);
        auto name = std::unique_ptr<LiteralExpr>((LiteralExpr *) this->currentExpression.release());
        this->currentExpression = std::make_unique<FunctionCallNode>(std::move(name),
                                                                     std::make_unique<ArgumentList>(
                                                                             lp,
                                                                             std::move(argumentsVisitor.arguments),
                                                                             (RightParenthesis *) ctx->lexer->currentToken)
        );
        goFurther(this, ctx);
        return;
    }
    auto lexer = ctx->lexer;
    TupleParser tupleParser;
    tupleParser.visit(lp, ctx);
    size_t n = tupleParser.expressions.size();
    if (n == 1) {
        this->currentExpression = std::make_unique<ParenthesisExpr>(
                std::unique_ptr<Token>(lp),
                std::move(tupleParser.expressions.back()),
                std::unique_ptr<Token>(lexer->currentToken));
    } else {
        this->currentExpression = std::make_unique<TupleCreate>(std::move(tupleParser.expressions));
    }
    auto next = lexer->nextToken();
    next->visit(this, ctx);
}

void ExpressionVisitor::visit(Add *add, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), add);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(InverseDiv *div, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), div);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(NotOp *notOp, VisitingContext *ctx) {
    OpExpressionVisitor opExpressionVisitor(this->currentExpression.release(), notOp);
    this->currentExpression = opExpressionVisitor.visit(ctx);
}

void ExpressionVisitor::visit(Colon *, VisitingContext *) {
}

void ExpressionVisitor::visit(ImportKeyword *keyword, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    auto *t = lexer->nextToken();
    ExpressionVisitor startExpressionVisitor;
    t->visit(&startExpressionVisitor, ctx);
    auto current = lexer->currentToken;
    validateToken(current, ElementType::semicolon, "';'", ctx);
    this->currentExpression = std::make_unique<ImportDeclaration>(std::unique_ptr<Token>(keyword),
                                                                  std::move(startExpressionVisitor.currentExpression),
                                                                  std::unique_ptr<Semicolon>(
                                                                          (Semicolon *) lexer->currentToken));
}

void ExpressionVisitor::visit(IfKeyword *ifKeyword, VisitingContext *ctx) {
    /*
    auto* t = lexer->nextToken();
    ExpressionVisitor expressionVisitor(this->lexer);
    t->accept(&expressionVisitor, ctx);

    auto expr = expressionVisitor.last;
    CodeBlockVisitor codeBlockVisitor(lexer);
    codeBlockVisitor.parse(ctx);

    this->last = std::make_shared<IfExpression>(std::shared_ptr<Token>(ifKeyword), expr, std::make_shared<CodeBlock>(codeBlockVisitor.expressions));
    */
}

void ExpressionVisitor::visit(RetKeyword *returnKeyword, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    auto t = lexer->nextToken();
    ExpressionVisitor startExpressionVisitor;
    t->visit(&startExpressionVisitor, ctx);
    this->currentExpression = std::make_unique<ReturnExpression>(std::unique_ptr<Token>(returnKeyword),
                                                                 std::move(startExpressionVisitor.currentExpression));
}

void ExpressionVisitor::visit(Semicolon *, VisitingContext *) {

}

void FunctionBodyVisitor::visit(VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    while (lexer->hasNext()) {
        auto next = lexer->nextToken();
        if (next->getTokenType() == ElementType::rightCurl) {
            break;
        }
        if (next->isNewLine()) {
            delete next;
            continue;
        }
        ExpressionVisitor expressionVisitor;
        next->visit(&expressionVisitor, ctx);
        if (expressionVisitor.currentExpression) {
            expressions.push_back(std::move(expressionVisitor.currentExpression));
        }
    }
}

void ConditionBlockVisitor::visit(VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    while (lexer->hasNext()) {
        auto peeked = lexer->peekNext();
        if (peeked->getTokenType() == ElementType::rightCurl) {
            break;
        }
        if (peeked->isElseKeyword()) {
            break;
        }

        auto next = lexer->nextToken();
        ExpressionVisitor startExpressionVisitor;
        next->visit(&startExpressionVisitor, ctx);
        this->expressions.push_back(std::move(startExpressionVisitor.currentExpression));
    }
    this->lastToken = lexer->nextToken();
}

void StringVisitor::visit(QuotionMark *lqm, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    std::vector<Token *> tokens;
    Token *rqm = nullptr;

    while (lexer->hasNext()) {
        auto current = lexer->nextTokenWithWS();
        if (current->getTokenType() == ElementType::quotationMark) {
            rqm = current;
            break;
        }
        tokens.push_back(current);
    }
    this->conditionalExpression = std::make_unique<StringNode>(lqm, tokens, rqm);
}

std::string StringNode::getTokensAsText() {
    std::string acc;
    for (auto t : this->tokens) {
        acc.append(t->getText());
    }
    return acc;
}

std::string StringNode::getText() {
    std::string acc;
    acc.append(lqm->getText());
    for (auto t : this->tokens) {
        acc.append(t->getText());
    }
    acc.append(rqm->getText());
    return acc;
}

std::string StringNode::getAsStringValue() {
    std::string acc;
    for (auto t : this->tokens) {
        acc.append(t->getText());
    }
    return acc;
}

ParameterListVisitor::ParameterListVisitor(LeftParenthesis *token) : token(token) {}

void ParameterListVisitor::visit(VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    while (lexer->hasNext()) {
        auto name = lexer->nextTokenSkipNL();
        if (name->getTokenType() == ElementType::rightParenthesis) {
            this->end = (RightParenthesis *) name;
            break;
        }
        if (name->getTokenType() == ElementType::comma) {
            delete name;
            continue;
        }
        if (!name->isLiteral()) {
            std::string msg = "Expected parameter name, got '" + name->getText() + "'";
            throw SyntaxException(msg.c_str(), name, lexer->filePath);
        }
        auto type = lexer->nextTokenSkipNL();
        if (type->getTokenType() == ElementType::rightParenthesis) {
            this->end = (RightParenthesis *) type;
            break;
        }
        if (type->getTokenType() == ElementType::comma) {
            delete type;
            continue;
        }

        this->parameters.push_back(new Parameter(name, new SingleTypeNode(type)));
    }
    if (!lexer->currentToken->isRightParenthesis()) {
        std::string msg = "Expected ')' after parameters declaration, got '" + lexer->currentToken->getText() + "'";
        throw SyntaxException(msg.c_str(), lexer->currentToken, lexer->filePath);
    }
}

StaticBlock::StaticBlock(std::vector<std::unique_ptr<Expression>> expressions) : ASTNode(ElementType::staticBlock) {
    this->expressions = std::move(expressions);
    for (const auto &expr: this->expressions) {
        expr->setParent(this);
    }
}

void StaticBlock::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

KeyValueExpr::KeyValueExpr(std::unique_ptr<Expression> key,
                           std::unique_ptr<Colon> colon,
                           std::unique_ptr<Expression> value) : Expression(ElementType::keyValue) {
    this->key = std::move(key);
    this->key->setParent(this);
    this->value = std::move(value);
    this->value->setParent(this);
    this->colon = std::move(colon);
}

void KeyValueExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

ArrayCreateExpr::ArrayCreateExpr(std::unique_ptr<Token> lb,
                                 std::vector<std::unique_ptr<Expression>> arguments,
                                 std::unique_ptr<Token> rb) : Expression(ElementType::arrayCreateExpr) {
    this->lb = std::move(lb);
    this->rb = std::move(rb);
    this->arguments = std::move(arguments);
    for (const auto &a: this->arguments) {
        a->nodeContext->parent = this;
    }
}

void ArrayCreateExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void ArrayAnonymousGetExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void ArrayGetExpr::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

CompilationNode::CompilationNode() = default;;

LocalAccess::LocalAccess(std::unique_ptr<Literal> token) : Expression(ElementType::localAccess) {
    this->literal = std::move(token);
}

void LocalAccess::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

LocalStore::LocalStore(std::unique_ptr<Literal> literal,
                       std::unique_ptr<Token> assignOp,
                       std::unique_ptr<Expression> rightExpr) : Expression(ElementType::localStore) {
    this->literal = std::move(literal);
    this->assignOp = std::move(assignOp);
    this->rightExpr = std::move(rightExpr);
}

void LocalStore::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

GlobalAccess::GlobalAccess(std::unique_ptr<Literal> token) : Expression(ElementType::globalAccess) {
    this->literal = std::move(token);
}

void GlobalAccess::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

GlobalStore::GlobalStore(std::unique_ptr<Literal> literal, std::unique_ptr<Token> assignOp,
                         std::unique_ptr<Expression> rightExpr) : Expression(ElementType::globalStore) {
    this->literal = std::move(literal);
    this->assignOp = std::move(assignOp);
    this->rightExpr = std::move(rightExpr);
}

void GlobalStore::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void PackageNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

void Lambda::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string Lambda::getText() {
    std::string acc = "lam ";
    for (const auto &p: this->parameters) {
        acc.append(p->name->getText());
    }
    acc = acc + " . ";
    for (const auto &p: this->lambdaBody) {
        acc.append(p->getText());
    }
    return acc;
}

void TupleCreate::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

ForLoopExpression::ForLoopExpression(std::unique_ptr<ForKeyword> forKeyword,
                                     std::unique_ptr<Expression> forLoopSubject,
                                     std::unique_ptr<CodeBlock> codeBlock) : Expression(
        ElementType::forLoopExpression) {

    this->forKeyword = std::move(forKeyword);
    this->forLoopSubject = std::move(forLoopSubject);
    this->forLoopSubject->setParent(this);
    this->codeBlock = std::move(codeBlock);
    this->codeBlock->setParent(this);
}

void ForLoopExpression::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string ForLoopExpression::getText() {
    std::string acc = "for ";
    acc += forLoopSubject->getText();
    acc += " do\n";
    acc += codeBlock->getText();
    acc += "\n";
    acc += "end\n";
    return acc;
}

WhileLoopExpression::WhileLoopExpression(std::unique_ptr<Token> keyword,
                                         std::unique_ptr<Expression> expression,
                                         std::unique_ptr<CodeBlock> body) : Expression(ElementType::whileExpr) {
    this->keyword = std::move(keyword);
    this->expression = std::move(expression);
    this->body = std::move(body);
    this->expression->setParent(this);
    this->body->setParent(this);
}

void WhileLoopExpression::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

std::string WhileLoopExpression::getText() {
    return this->keyword->getText() + " " + this->expression->getText() + " do \n" + this->body->getText() + "\n end";
}

void ForLoopVisitor::visit(ForKeyword *forKeyword, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    while (lexer->hasNext()) {
        auto next = lexer->nextToken();
        if (next->getTokenType() == ElementType::newLine) {
            continue;
        }
        if (next->getTokenType() == ElementType::leftCurl) {
            if (this->expressions.size() == 2) {
                //this->expressions.push_back(std::make_unique<EmptyExpression>());
            }
            break;
        }
        ExpressionVisitor startExpressionVisitor;
        next->visit(&startExpressionVisitor, ctx);

        if (startExpressionVisitor.currentExpression) {
            this->expressions.push_back(std::move(startExpressionVisitor.currentExpression));
        } else {
            //this->expressions.push_back(std::make_unique<EmptyExpression>());
        }

        if (lexer->currentToken->getTokenType() == ElementType::leftCurl) {
            if (this->expressions.size() == 2) {
                //this->expressions.push_back(std::make_unique<EmptyExpression>());
            }
            break;
        }
        if (lexer->currentToken->getTokenType() == ElementType::semicolon) {
            this->semicolons.push_back(std::unique_ptr<Semicolon>((Semicolon *) lexer->currentToken));
        }
    }

    auto doKeyword = lexer->currentToken;

    expressions.clear();

    while (lexer->hasNext()) {
        Token *next = lexer->nextToken();
        if (next->getTokenType() == ElementType::newLine) {
            continue;
        }
        if (next->getTokenType() == ElementType::rightCurl) {
            break;
        }
        ExpressionVisitor startExpressionVisitor;
        next->visit(&startExpressionVisitor, ctx);
        this->expressions.push_back(std::move(startExpressionVisitor.currentExpression));
        if (lexer->currentToken->getTokenType() == ElementType::rightCurl) {
            break;
        }
    }

    auto last = lexer->currentToken;
    delete last;
    delete doKeyword;

    this->forLoopExpression = std::make_unique<ForLoopExpression>(
            std::unique_ptr<ForKeyword>(forKeyword),
            std::move(this->subject),
            std::make_unique<CodeBlock>(std::move(this->expressions)));
}

void ReferenceExpressionVisitor::visit(IntNumber *in, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    auto next = lexer->nextToken();
    this->currentExpression = std::make_unique<IntNode>(std::unique_ptr<IntNumber>(in));
}

void ReferenceExpressionVisitor::visit(Sub *sub, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    RightHandExpressionVisitor rightHandExpressionVisitor;
    auto next = lexer->nextToken();
    next->visit(&rightHandExpressionVisitor, ctx);
    this->currentExpression = std::make_unique<NegativeExpr>(std::unique_ptr<Token>(sub),
                                                             std::move(rightHandExpressionVisitor.currentExpression));
}

void WhileLoopVisitor::visit(WhileKeyword *whileKeyword, VisitingContext *ctx) {
    auto lexer = ctx->lexer;
    ExpressionVisitor startExpressionVisitor;
    lexer->nextToken()->visit(&startExpressionVisitor, ctx);
    auto whileExpression = std::move(startExpressionVisitor.currentExpression);

    auto doKeyword = lexer->currentToken;

    auto next = lexer->nextToken();
    std::vector<std::unique_ptr<Expression>> expressions;
    while (lexer->hasNext() && lexer->currentToken->getTokenType() != ElementType::rightCurl) {
        if (lexer->currentToken->isNewLine()) {
            lexer->nextToken();
            continue;
        }
        ExpressionVisitor sv;
        lexer->currentToken->visit(&sv, ctx);
        expressions.push_back(std::move(sv.currentExpression));
    }

    auto endKeyword = lexer->currentToken;

    this->whileLoopExpression = std::make_unique<WhileLoopExpression>(
            std::unique_ptr<WhileKeyword>(whileKeyword),
            std::move(whileExpression),
            std::make_unique<CodeBlock>(std::move(expressions)));
}

void IfVisitor::visit(IfKeyword *ifKeyword, VisitingContext *ctx) {
    IfExpression* ifExpression = nullptr;
    auto lexer = ctx->lexer;
    auto next = lexer->nextToken();

    ExpressionVisitor expressionVisitor;
    next->visit(&expressionVisitor, ctx);
    auto ifCondition = std::move(expressionVisitor.currentExpression);
    auto leftCurl = lexer->currentToken;

    ConditionBlockVisitor ifBlockVisitor;
    ifBlockVisitor.visit(ctx);
    auto ifBlock = std::make_unique<CodeBlock>(std::move(ifBlockVisitor.expressions));

    if (lexer->currentToken->getTokenType() == ElementType::rightCurl) {
        ifExpression = new IfExpression(
                ifKeyword,
                std::move(ifCondition),
                leftCurl,
                std::move(ifBlock),
                lexer->currentToken);
    } else {
        reportSytnaxError(lexer->currentToken, lexer);
    }

    auto peeked = lexer->peekNext();
    if (peeked->isElseKeyword()) {
        auto elseKeyword = (ElseKeyword*) lexer->nextToken();
        leftCurl = lexer->nextToken();

        ConditionBlockVisitor elseBlockVisitor;
        elseBlockVisitor.visit(ctx);
        auto elseBlock =  std::make_unique<CodeBlock>(std::move(elseBlockVisitor.expressions));
        ifExpression->elseExpression = std::make_unique<ElseExpression>(
                elseKeyword,
                leftCurl,
                std::move(elseBlock),
                lexer->currentToken
        );
    }

    this->conditionalExpression = ifExpression;
}

FunctionVoidReturnTypeNode::FunctionVoidReturnTypeNode() : FunctionReturnTypeNode(new SingleTypeNode(new Literal(0, "Unit"))) {}

void FunctionVoidReturnTypeNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

FunctionNonVoidReturnTypeNode::FunctionNonVoidReturnTypeNode(Arrow* arrow,
                                                             TypeNode* typeNode) : FunctionReturnTypeNode(typeNode) {
    this->arrow = std::move(arrow);
}

void FunctionNonVoidReturnTypeNode::accept(ASTVisitor *visitor) {
    visitor->visit(this);
}

BinExpr::BinExpr(std::unique_ptr<Expression> left,
                 std::unique_ptr<Token> op,
                 std::unique_ptr<Expression> right,
                 ElementType nodeType) : Expression(nodeType) {

    this->left = std::move(left);
    this->op = std::move(op);
    this->right = std::move(right);
    this->right->nodeContext->parent = this;
    if (this->left) {
        this->left->nodeContext->parent = this;
    }
}
