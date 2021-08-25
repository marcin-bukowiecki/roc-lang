//
// Created by Marcin on 18.03.2021.
//
#include "MIR.h"
#include "../compiler/Extensions.h"

#include <utility>

void forEachChildren(std::vector<std::unique_ptr<Expression>> *children, ToMIRVisitor *toMIRVisitor) {
    for (auto& ch: *children) {
        ch->accept(toMIRVisitor);
    }
}

std::vector<MIRValue*> popElements(ToMIRVisitor *toMirVisitor, int n) {
    std::vector<MIRValue*> result(n);
    for (int i = 0; i < n; i++) {
        result[n - 1 - i] = toMirVisitor->valueStack.back();
        toMirVisitor->valueStack.pop_back();
    }
    return result;
}

MIRValue* popElement(ToMIRVisitor *toMirVisitor) {
    auto result = toMirVisitor->valueStack.back();
    toMirVisitor->valueStack.pop_back();
    return result;
}

void MIRTypeDecl::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRCCall::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRFunctionCall::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRFunctionInstanceCall::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRModule::visit(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

MIRBlock::MIRBlock(std::string name,
                   std::vector<MIRValue*> values) : MIRValue(), name(std::move(name)), values(std::move(values)) {}

void MIRBlock::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRTrue::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visitTrue(this);
}

void MIRFalse::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visitFalse(this);
}

void ToMIRVisitor::visit(ModuleDeclaration *moduleDeclaration) {
    moduleDeclaration->staticBlock->accept(this);
    for (auto& f :moduleDeclaration->functions) {
        f->accept(this);
    }
    this->mirModule = std::make_unique<MIRModule>(moduleDeclaration->moduleName,
                                                  std::move(this->functionStack));
    this->functionStack.clear();
}

void ToMIRVisitor::visit(StaticBlock *staticBlock) {
    for (auto& exp :staticBlock->expressions) {
        exp->accept(this);
    }
    this->valueStack.push_back(new MIRReturnValue(new MIRConstantInt(0)));
    auto block = new MIRBlock("entry", std::move(this->valueStack));
    this->valueStack.clear();
    auto returnType = new MIRTypeDecl(new RocInt32Type());
    std::vector<MIRFunctionParameter*> parameters;
    this->functionStack.push_back(std::make_unique<MIRFunction>("main", std::move(parameters), returnType, block));
}

void ToMIRVisitor::visit(FunctionDeclaration *fd) {
    std::vector<MIRFunctionParameter*> params;
    for (auto& p :fd->parameterList->parameters) {
        auto paramType = ((RocTypeNodeContext*) p->typeNode->getContextHolder(TYPE_CONTEXT))->getGivenType();
        params.push_back(new MIRFunctionParameter(p->name->getText(), new MIRTypeDecl(paramType->clone())));
    }

    for (auto& exp :fd->body->expressions) {
        exp->accept(this);
    }

    MIRTypeDecl* returnType;
    if (fd->functionReturnTypeNode) {
        returnType = new MIRTypeDecl(((RocTypeNodeContext*) fd->functionReturnTypeNode->typeNode->getContextHolder(TYPE_CONTEXT))->getGivenType());
    } else {
        returnType = new MIRTypeDecl(new UnitRocType());
    }

    auto block = new MIRBlock("entry", std::move(this->valueStack));
    this->valueStack.clear();

    if (block->values.empty()) {
        block->values.push_back(new MIRReturnVoidValue());
    } else if (!block->values.empty() && !block->values.back()->isReturn()) {
        block->values.push_back(new MIRReturnVoidValue());
    }

    this->functionStack.push_back(std::make_unique<MIRFunction>(fd->getName()->getText(),
                                                                params,
                                                                returnType,
                                                                block));
}

void ToMIRVisitor::visit(StringNode *stringNode) {
    this->valueStack.push_back(new MIRRawString(stringNode->getAsStringValue()));
}

void ToMIRVisitor::visit(FunctionCallNode *functionCallNode) {
    auto size = functionCallNode->argumentList->size();
    for (auto& arg :functionCallNode->argumentList->arguments) {
        arg->accept(this);
    }
    std::vector<MIRValue*> arguments(size);
    for (int i = 0; i < size; i++) {
        arguments[size - 1 - i] = this->valueStack.back();
        this->valueStack.pop_back();
    }
    auto ctx = (RocFunctionCallContext*) functionCallNode->getContextHolder("typeContext");
    auto tc = ctx->targetFunctionCall;

    if (functionCallNode->getParent()->isDotExpr()) {
        auto caller = this->valueStack.back();
        this->valueStack.pop_back();
        this->valueStack.push_back(new MIRFunctionInstanceCall(
                caller,
                functionCallNode->getName(),
                arguments,
                tc));
    } else {
        auto name = functionCallNode->getName();
        if (name == "ccall") {
            auto targetName = arguments.front()->getText();
            auto returnType = functionCallNode->literalExpr->typeVariables.front();
            arguments.erase(arguments.begin());
            this->valueStack.push_back(new MIRCCall(targetName, arguments, ((RocTypeNodeContext*) returnType->getContextHolder(TYPE_CONTEXT))->getGivenType()->clone()));
        } else {
            this->valueStack.push_back(new MIRFunctionCall(functionCallNode->getName(), arguments, tc));
        }
    }
}

void ToMIRVisitor::visit(ReturnExpression *returnExpression) {
    returnExpression->expression->accept(this);
    auto expr = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRReturnValue(expr));
}

void ToMIRVisitor::visit(IntNode *intNode) {
    this->valueStack.push_back(new MIRConstantInt(intNode->value->value));
}

void ToMIRVisitor::visit(LocalAccess *node) {
    auto t = ((RocTypeNodeContext*) node->getContextHolder(TYPE_CONTEXT))->getGivenType()->clone();
    this->valueStack.push_back(new MIRLocalVariableAccess(node->localVariableRef->name,
                                                         node->localVariableRef->index,
                                                         t));
}

void ToMIRVisitor::visit(ArrayCreateExpr *arrayCreateExpr) {
    forEachChildren(&arrayCreateExpr->arguments, this);
    auto size = arrayCreateExpr->arguments.size();
    auto elements = popElements(this, size);
    this->valueStack.push_back(new MIRInt32Array(std::move(elements)));
}

void ToMIRVisitor::visit(ModExpr *modExpr) {
    ASTVisitor::visit(modExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    auto type = getReturnType(modExpr);
    this->valueStack.push_back(new MIRInt64Mod(std::move(left), right));
}

void ToMIRVisitor::visit(AddExpr *addExpr) {
    ASTVisitor::visit(addExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Add(std::move(left), right));
}

void ToMIRVisitor::visit(SubExpr *subExpr) {
    ASTVisitor::visit(subExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Sub(std::move(left), right));
}

void ToMIRVisitor::visit(DivExpr *divExpr) {
    ASTVisitor::visit(divExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Div(std::move(left), std::move(right)));
}

void ToMIRVisitor::visit(MulExpr *mulExpr) {
    ASTVisitor::visit(mulExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Mul(std::move(left), std::move(right)));
}

void ToMIRVisitor::visit(AndExpr *andExpr) {
    ASTVisitor::visit(andExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRAnd(std::move(left), std::move(right)));
}

void ToMIRVisitor::visit(OrExpr *orExpr) {
    ASTVisitor::visit(orExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIROr(std::move(left), std::move(right)));
}

void ToMIRVisitor::visit(GreaterExpr *greaterExpr) {
    ASTVisitor::visit(greaterExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Ge(std::move(left), std::move(right)));
}

void ToMIRVisitor::visit(GreaterOrEqualExpr *lesserOrEqualExpr) {
    ASTVisitor::visit(lesserOrEqualExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Ge(std::move(left), std::move(right)));
}

void ToMIRVisitor::visit(LesserExpr *lesserExpr) {
    ASTVisitor::visit(lesserExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Le(std::move(left), right));
}

void ToMIRVisitor::visit(LesserOrEqualExpr *lesserOrEqualExpr) {
    ASTVisitor::visit(lesserOrEqualExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Le(left, right));
}

void ToMIRVisitor::visit(EqualOpExpr *equalOpExpr) {
    ASTVisitor::visit(equalOpExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32Eq(left, right));
}

void ToMIRVisitor::visit(NotEqualOpExpr *notEqualOpExpr) {
    ASTVisitor::visit(notEqualOpExpr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new MIRInt32NotEq(left, right));
}

void ToMIRVisitor::visit(IfExpression *ifExpression) {
    ifExpression->expression->accept(this);
    auto condition = popElements(this, 1);
    ifExpression->block->accept(this);
    auto block = popElements(this, 1);
    if (ifExpression->elseExpression) {

    } else {
        this->valueStack.push_back(new MIRIf(new MIRCondition(condition.back()), (MIRBlock*) block.back()));
    }
}

void ToMIRVisitor::visit(CodeBlock *codeBlock) {
    auto result = std::vector<MIRValue*>();
    for (auto& v: codeBlock->expressions) {
        v->accept(this);
        result.push_back(popElements(this, 1).back());
    }
    this->valueStack.push_back(new MIRBlock("block", std::move(result)));
}

void ToMIRVisitor::visit(TrueExpr *trueExpr) {
    this->valueStack.push_back(new MIRTrue());
}

void ToMIRVisitor::visit(FalseExpr *falseExpr) {
    this->valueStack.push_back(new MIRFalse());
}

void MIRFunction::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

std::vector<RocType *> MIRFunction::getArgumentTypes() {
    std::vector<RocType*> result;
    for (auto& p : this->parameters) {
        result.push_back(p->type->rocType);
    }
    return result;
}

std::string MIRFunction::getName() {
    return this->name;
}

MIRRawString::MIRRawString(std::string value) : value(std::move(value)) {
    this->type = new RocRawStringType(value.length());
}

void MIRRawString::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRConstantInt::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRLocalVariableAccess::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

MIRReturnValue::MIRReturnValue(MIRValue* value) {
    this->value = value;
    this->value->parent = this;
}

void MIRReturnValue::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRReturnVoidValue::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRMod::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt64Mod::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Mod::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt64Add::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Add::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt64Sub::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Sub::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Div::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt64Div::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Mul::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt64Mul::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRAnd::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIROr::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Eq::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32NotEq::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Gt::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Lt::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Ge::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Le::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRStringToRaw::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRToPtr::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRToWrapper::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRCastTo::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32Array::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32ArraySet::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRInt32ArrayGet::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void SmartTypeCaster::visit(MIRFunctionInstanceCall *node) {
    auto caller = node->caller;
    if (caller->getType()->isPrimitive()) {
        node->caller = new MIRToWrapper(caller);
    }
    SmartTypeCaster::visit((MIRFunctionCall*) node);
}

void SmartTypeCaster::visit(MIRFunctionCall *node) {
    for (const auto &item : node->arguments) item->accept(this);

    auto expectedTypes = node->getTargetCall()->getArgumentTypes();
    int i=0;
    for (auto& arg: node->arguments) {
        auto given = arg->getType();
        auto expected = expectedTypes.at(i);
        if (given->equals(expected)) {
            continue;
        }

        if (given->isPrimitive() && !expected->isPrimitive()) {
            given = new RocWrapperType(given);
            auto newValue = new MIRToWrapper(node->arguments.at(i));
            newValue->parent = node;
            node->arguments[i] = newValue;
        }

        if (!given->equals(expected)) {
            auto newValue = new MIRCastTo(node->arguments.at(i), expected->clone());
            newValue->parent = node;
            node->arguments[i] = newValue;
        }

        /*
        if (expected->isRawString() && given->typeEnum == TypeEnum::stringType) {
            auto newValue = new MIRStringToRaw(node->arguments.at(i));
            newValue->parent = node;
            node->arguments[i] = newValue;
        } else if (expected->isAny()) {
            auto newValue = new MIRToAny(node->arguments.at(i));
            newValue->parent = node;
            node->arguments[i] = newValue;
        }
*/
        i++;
    }
}

MIRIf::MIRIf(MIRCondition* condition, MIRBlock* block) : condition(condition), block(block) {
    this->type = new UnitRocType();
}

MIRIf::~MIRIf() {
    delete condition;
    delete block;
    delete type;
    delete endLabel;
    delete elseBlock;
}

void MIRIf::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visitIf(this);
}

MIRCondition::MIRCondition(MIRValue *expr) : expr(expr) {}

void MIRCondition::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRBinOpBase::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

void MIRValue::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visit(this);
}

MIRLabel::MIRLabel(int id, const std::string &name) : id(id), name(name) {}

void MIRElse::accept(MIRVisitor *mirVisitor) {
    mirVisitor->visitElse(this);
}
