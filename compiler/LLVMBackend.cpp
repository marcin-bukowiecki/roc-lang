//
// Created by Marcin Bukowiecki on 14.03.2021.
//

#include "llvm/ADT/APInt.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/IRBuilder.h>
#include "llvm/Support/raw_ostream.h"
#include <llvm/Support/FileSystem.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#include "LLVMBackend.h"
#include "Builtins.h"
#include "Types.h"
#include "Extensions.h"
#include "LLVMUtils.h"

using namespace llvm;

Type *ToLLVMVisitor::defineBuiltinStruct(TypeEnum typeEnum) const {
    std::vector<Type *> fieldTypes;
    fieldTypes.push_back(Type::getInt64Ty(*this->llvmContext));
    fieldTypes.push_back(Type::getInt64Ty(*this->llvmContext));

    switch (typeEnum) {
        case int32Type:
            fieldTypes.push_back(Type::getInt32Ty(*this->llvmContext));
            return StructType::create(*this->llvmContext, fieldTypes, "Int32TypeStruct");
        case stringType:
            fieldTypes.push_back(Type::getInt8PtrTy(*this->llvmContext));
            fieldTypes.push_back(Type::getInt32Ty(*this->llvmContext));
            return StructType::create(*this->llvmContext, fieldTypes, "StringTypeStruct");
        default:
            fieldTypes.push_back(Type::getInt32Ty(*this->llvmContext));
            return StructType::create(*this->llvmContext, fieldTypes, "TypeStruct");
    }
}

void ToLLVMVisitor::visit(MIRModule *mirModule) {
    defineIntType(this->rocLLVMContext);
    createAnyToString(this->rocLLVMContext);
    definePrintln(this->rocLLVMContext);
    definePrint(this->rocLLVMContext);

    defineInt32ToStringRaw(this->rocLLVMContext);

    //createFunctionDispatcher(&rocLlvmContext);
    //createFunctionDispatcherForAny(&rocLlvmContext);

    createInt32VTable(this->rocLLVMContext);
    createStringRawVTable(this->rocLLVMContext);

    for (auto &f :mirModule->functions) {
        Type *returnType;

        if (f->returnTypeDecl) {
            returnType = f->returnTypeDecl->rocType->getLLVMType(this->rocLLVMContext);
            if (f->returnTypeDecl->rocType->isStruct()) {
                returnType = returnType->getPointerTo();
            }
        } else {
            returnType = Type::getVoidTy(*this->llvmContext);
        }

        std::vector<Type *> parameterTypes;
        for (auto &p :f->parameters) {
            ToLLVMTypeVisitor toLlvmTypeVisitor(this->rocLLVMContext);
            p->type->rocType->visit(&toLlvmTypeVisitor);
            if (toLlvmTypeVisitor.result == nullptr) throw "type not solved";
            parameterTypes.push_back(toLlvmTypeVisitor.result);
        }
        auto *createdFunctionType = FunctionType::get(returnType, parameterTypes, false);
        auto *function = Function::Create(createdFunctionType, Function::ExternalLinkage, f->name, module);
        if (f->name == "main") {
            this->mainFunction = function;
        }
        f->llvmFunction = function;
    }
    for (auto &f :mirModule->functions) {
        f->accept(this);
    }
}

void ToLLVMVisitor::visit(MIRFunction *mirFunction) {
    this->compilationFunctionStack.push_back(mirFunction);
    int i = 0;
    for (auto &p: mirFunction->parameters) {
        mirFunction->localsMap.insert({i, this->compilationFunctionStack.back()->llvmFunction->getArg(i)});
        i++;
    }
    mirFunction->body->accept(this);
    this->compilationFunctionStack.pop_back();
}

void ToLLVMVisitor::visit(MIRBlock *mirBlock) {
    if (!mirBlock->blockInitialized) {
        this->currentBlock = BasicBlock::Create(*this->llvmContext,
                                                mirBlock->name,
                                                this->compilationFunctionStack.back()->llvmFunction);
    }

    if (this->compilationFunctionStack.back()->llvmFunction->getName() == "main") {
        auto initFT1 = FunctionType::get(this->rocLLVMContext->int64Type, {}, false);
        auto initF1 = module->getOrInsertFunction("Int32.vtable.init", initFT1);
        CallInst::Create(initF1, {}, "", this->currentBlock);

        auto initFT2 = FunctionType::get(this->rocLLVMContext->int64Type, {}, false);
        auto initF2 = module->getOrInsertFunction("StringRaw.vtable.init", initFT2);
        CallInst::Create(initF2, {}, "", this->currentBlock);
    }
    for (auto &expr: mirBlock->values) {
        expr->accept(this);
    }
}

void ToLLVMVisitor::visit(MIRFunctionInstanceCall *mirFunctionCall) {
    mirFunctionCall->caller->accept(this);
    for (auto &arg :mirFunctionCall->arguments) {
        arg->accept(this);
    }
    //instance call transformed to function call where 1st argument is the calling ref
    std::vector<Value *> values(mirFunctionCall->arguments.size() + 1);
    auto callerRef = this->valueStack.back();
    this->valueStack.pop_back();
    values[0] = callerRef;
    for (int i = 1; i < mirFunctionCall->arguments.size(); ++i) {
        values[1 + mirFunctionCall->arguments.size() - i] = this->valueStack.back();
        this->valueStack.pop_back();
    }
    auto *cast = BitCastInst::Create(Instruction::BitCast,
                                     callerRef,
                                     this->rocLLVMContext->anyTypeStructType->getPointerTo(),
                                     "cast-to-any",
                                     this->currentBlock);
    auto f = this->module->getOrInsertFunction("myGetFunctionPointer", FunctionType::get(rocLLVMContext->int64Type, {
            rocLLVMContext->anyTypeStructType->getPointerTo(),
            rocLLVMContext->int64Type
    }, false));
    //call instance method
    auto vtableMap = CallInst::Create(f,
                                      {
                                              cast, ConstantInt::get(rocLLVMContext->int64Type,
                                                                          roc::methods::getMethodId(
                                                                                  mirFunctionCall->name))},
                                      "call-myGetFunctionPointer",
                                      this->currentBlock);

    std::vector<Type *> argumentTypes;
    auto callerType = mirFunctionCall->caller->getType();
    if (callerType->isStruct()) {
        callerType = callerType->getPtrType(); //for structs we are passing pointers
    }
    argumentTypes.push_back(callerType->getLLVMType(this->rocLLVMContext));
    for (auto &arg :mirFunctionCall->arguments) {
        auto rt = arg->getType();
        ToLLVMTypeVisitor toLlvmTypeVisitor(this->rocLLVMContext);
        rt->visit(&toLlvmTypeVisitor);
        argumentTypes.push_back(toLlvmTypeVisitor.result);
    }

    FunctionType *ft = FunctionType::get(
            mirFunctionCall->getTargetCall()->getReturnType()->getLLVMType(this->rocLLVMContext),
            argumentTypes, false);
    auto ptr = CastInst::Create(llvm::Instruction::IntToPtr,
                                vtableMap,
                                ft->getPointerTo(),
                                "",
                                this->currentBlock);
    auto value = CallInst::Create(ft,
                                  ptr,
                                  values,
                                  mirFunctionCall->getTargetCall()->getRealName(), this->currentBlock);
    this->valueStack.pop_back();

    if (!ft->getReturnType()->isVoidTy()) {
        this->valueStack.push_back(value);
    }
}

void ToLLVMVisitor::visit(MIRCCall *mircCall) {
    ToLLVMVisitor::visit((MIRFunctionCall*) mircCall);
}

void ToLLVMVisitor::visit(MIRFunctionCall *mirFunctionCall) {
    for (auto &arg :mirFunctionCall->arguments) arg->accept(this);

    auto numberOfArguments = mirFunctionCall->arguments.size();
    std::vector<Value *> values(numberOfArguments);
    for (int i = 0; i < numberOfArguments; ++i) {
        values[numberOfArguments - 1 - i] = this->valueStack.back();
        this->valueStack.pop_back();
    }

    std::vector<Type *> argumentTypes;
    for (auto &arg :mirFunctionCall->arguments) {
        auto llvmType = arg->getType()->getLLVMType(this->rocLLVMContext);
        if (arg->getType()->isRawString()) {
            argumentTypes.push_back(llvmType->getPointerTo());
            continue;
        }
        if (llvmType->isStructTy()) {
            argumentTypes.push_back(llvmType->getPointerTo());
        } else {
            argumentTypes.push_back(llvmType);
        }
    }

    auto rt = mirFunctionCall->getType()->getLLVMType(this->rocLLVMContext);
    FunctionType *ft = FunctionType::get(rt, argumentTypes, false);
    auto functionToCall = this->module->getOrInsertFunction(mirFunctionCall->getName(), ft);

    std::string name = ft->getReturnType()->isVoidTy() ? "" : "call" + mirFunctionCall->getName();

    auto value = CallInst::Create(functionToCall,
                                  values,
                                  name,
                                  this->currentBlock);

    if (!ft->getReturnType()->isVoidTy()) {
        this->valueStack.push_back(value);
    }
}

void ToLLVMVisitor::visit(MIRRawString *mirRawString) {
    IRBuilder<> builder(*this->llvmContext);
    Value *helloWorldStr = builder.CreateGlobalStringPtr(mirRawString->getText() + "\n", "s1", 0, this->module);
    this->valueStack.push_back(helloWorldStr);
}

ToLLVMVisitor::ToLLVMVisitor(LLVMContext *llvmContext, Module *module) : module(module), llvmContext(llvmContext) {
    this->rocLLVMContext = new RocLLVMContext(this->llvmContext, this->module);
}

Type *ToLLVMVisitor::createBaseType() {
    auto *baseType = StructType::create(*this->llvmContext);

    //typeId()
    auto typeIdFT = FunctionType::get(Type::getInt32Ty(*this->llvmContext), {}, false);
    auto typeIdF = this->module->getOrInsertFunction("typeId", typeIdFT);


    return nullptr;
}

void ToLLVMVisitor::visit(MIRConstantInt *mirConstantInt) {
    this->valueStack.push_back(ConstantInt::get(Type::getInt32Ty(*this->llvmContext), mirConstantInt->value));
}

void ToLLVMVisitor::visit(MIRReturnValue *mirReturnValue) {
    mirReturnValue->value->accept(this);
    auto returnInst = ReturnInst::Create(*this->llvmContext, this->valueStack.back(), this->currentBlock);
    this->valueStack.pop_back();
    this->valueStack.push_back(returnInst);
}

void ToLLVMVisitor::visit(MIRReturnVoidValue *mirReturnValue) {
    this->valueStack.push_back(ReturnInst::Create(*this->llvmContext, this->currentBlock));
}


ToLLVMTypeVisitor::ToLLVMTypeVisitor(RocLLVMContext *rocLLVMContext) {
    this->llvmContext = rocLLVMContext->llvmContext;
    this->rocLLVMContext = rocLLVMContext;
}

void ToLLVMTypeVisitor::visit(UnitRocType *type) {
    this->result = Type::getVoidTy(*this->llvmContext);
}

void ToLLVMTypeVisitor::visit(RocInt32Type *type) {
    this->result = Type::getInt32Ty(*this->llvmContext);
}

void ToLLVMTypeVisitor::visit(RocInt64Type *type) {
    this->result = Type::getInt64Ty(*this->llvmContext);
}

void ToLLVMTypeVisitor::visit(RocFloat64Type *type) {
    this->result = Type::getDoubleTy(*this->llvmContext);
}

void ToLLVMTypeVisitor::visit(RocRawStringType *type) {
    //this->result = ArrayType::get(Type::getInt8Ty(*this->llvmContext), type->length);
    this->result = Type::getInt8Ty(*this->llvmContext)->getPointerTo();
}

void ToLLVMTypeVisitor::visit(RocStringType *type) {
    this->result = this->rocLLVMContext->stringType->getPointerTo();
}

void ToLLVMTypeVisitor::visit(RocAnyType *type) {
    this->result = this->rocLLVMContext->anyTypeStructType->getPointerTo();
}


void ToLLVMVisitor::visit(MIRInt64Add *mirInt64Add) {
    MIRVisitor::visit(mirInt64Add);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateAdd(left, right, "int64Add", this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt32Add *mirInt32Add) {
    MIRVisitor::visit(mirInt32Add);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateAdd(left, right, "int32Add", this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt64Sub *mirInt64Sub) {
    MIRVisitor::visit(mirInt64Sub);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateSub(left, right, "int64Sub", this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt32Sub *mirInt32Sub) {
    MIRVisitor::visit(mirInt32Sub);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateSub(left, right, "int32Sub", this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt64Div *mirInt64Div) {
    MIRVisitor::visit(mirInt64Div);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    auto result = BinaryOperator::CreateSDiv(left, right, "mirInt64Div", this->currentBlock);
    this->valueStack.push_back(CastInst::Create(llvm::Instruction::SIToFP,
                                                result,
                                                mirInt64Div->type->getLLVMType(this->rocLLVMContext),
                                                "",
                                                this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt32Div *mirInt32Div) {
    MIRVisitor::visit(mirInt32Div);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    auto result = BinaryOperator::CreateSDiv(left, right, "int32Div", this->currentBlock);
    this->valueStack.push_back(CastInst::Create(llvm::Instruction::SIToFP,
                                                result,
                                                mirInt32Div->type->getLLVMType(this->rocLLVMContext),
                                                "",
                                                this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt64Mul *mirInt64Mul) {
    MIRVisitor::visit(mirInt64Mul);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateMul(left, right, "int64Mul", this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt32Mul *mirInt32Mul) {
    MIRVisitor::visit(mirInt32Mul);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateMul(left, right, "int32Mul", this->currentBlock));
}

void ToLLVMVisitor::visit(MIRLocalVariableAccess *localAccess) {
    this->valueStack.push_back(this->compilationFunctionStack.back()->localsMap.find(localAccess->index)->second);
}

void ToLLVMVisitor::visit(MIRCastTo *mirCastTo) {
    mirCastTo->from->accept(this);
    auto type = mirCastTo->from->getType();
    auto to = mirCastTo->targetType;
    if (type->isPrimitive()) {

    } else {
        auto *cast = BitCastInst::Create(Instruction::BitCast,
                                         this->valueStack.back(),
                                         this->rocLLVMContext->anyTypeStructType->getPointerTo(),
                                         "cast-to-any",
                                         this->currentBlock);
        this->valueStack.pop_back();
        this->valueStack.push_back(cast);
    }
}

void ToLLVMVisitor::visit(MIRToWrapper *mirToWrapper) {
    mirToWrapper->expr->accept(this);
    auto value = this->valueStack.back();
    this->valueStack.pop_back();
    auto givenType = mirToWrapper->expr->getType();
    if (givenType->isRawString()) {
        this->valueStack.push_back(newRocRawStruct(this, value, ((RocRawStringType *) givenType)->length));
    } else if (givenType->typeEnum == TypeEnum::int32Type) {
        this->valueStack.push_back(newRocInt32Struct(this, value));
    } else {
        throw "Unsupported type";
    }
}

void ToLLVMVisitor::visit(MIRStringToRaw *mirStringToRaw) {
    mirStringToRaw->expr->accept(this);

    /*
    std::vector<Value*> gep1Args;
    gep1Args.push_back(ConstantInt::get(Type::getInt64Ty(*llvmContext), 0));
    gep1Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 2));
    auto gep1 = GetElementPtrInst::Create(this->rocLLVMContext->stringRawType,
                                          this->valueStack.back(),
                                          gep1Args,
                                          "gep1", this->currentBlock);
    auto* loadInst = new LoadInst(Type::getInt8PtrTy(*this->rocLLVMContext->llvmContext),
                                  gep1,
                                  "get-raw-string",
                                  this->currentBlock);

    this->valueStack.pop_back();
    this->valueStack.push_back(loadInst);*/
}

void ToLLVMVisitor::visit(MIRInt32Array *mirInt32Array) {
    Value *gepRef;
    if (mirInt32Array->allocationSpace == roc::AllocationSpace::StackAllocation) {
        gepRef = new AllocaInst(this->rocLLVMContext->int32Type,
                                0,
                                ConstantInt::get(this->rocLLVMContext->int32Type, mirInt32Array->elements.size()),
                                "",
                                this->currentBlock);
        this->valueStack.push_back(gepRef);
    } else {
        auto mallocResult = callMalloc(this->rocLLVMContext->llvmContext,
                                       this->module,
                                       mirInt32Array->elements.size() * 4,
                                       this->currentBlock, "");
        gepRef = castTo(mallocResult, rocLLVMContext->int32Type->getPointerTo(), this->currentBlock);
        this->valueStack.push_back(gepRef);
    }

    int i = 0;
    for (const auto &item : mirInt32Array->elements) {
        item->accept(this);
        auto value = this->popLast();
        auto ptr = getPointerTo(rocLLVMContext, gepRef, this->currentBlock, i);
        storeValue(value, ptr, this->currentBlock);
        i++;
    }
}

void ToLLVMVisitor::visit(MIRInt32ArraySet *mirInt32ArraySet) {

}

void ToLLVMVisitor::visit(MIRInt32ArrayGet *mirInt32ArrayGet) {

}

void ToLLVMVisitor::visit(MIRAnd *mirAnd) {
    MIRVisitor::visit(mirAnd);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateAnd(left, right, "and", this->currentBlock));
}

void ToLLVMVisitor::visit(MIROr *mirOr) {
    MIRVisitor::visit(mirOr);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(BinaryOperator::CreateOr(left, right, "or", this->currentBlock));
}

void ToLLVMVisitor::visit(MIRInt32Eq *mirInt32Eq) {
    MIRVisitor::visit(mirInt32Eq);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new ICmpInst(*this->currentBlock, ICmpInst::ICMP_EQ, left, right, "eq"));
}

void ToLLVMVisitor::visit(MIRInt32NotEq *mirInt32NotEq) {
    MIRVisitor::visit(mirInt32NotEq);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new ICmpInst(*this->currentBlock, ICmpInst::ICMP_NE, left, right, "not eq"));
}

void ToLLVMVisitor::visit(MIRInt32Lt *mirInt32Ls) {
    MIRVisitor::visit(mirInt32Ls);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new ICmpInst(*this->currentBlock, ICmpInst::ICMP_SLT, left, right, "lesser"));
}

void ToLLVMVisitor::visit(MIRInt32Gt *mirInt32Gt) {
    MIRVisitor::visit(mirInt32Gt);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new ICmpInst(*this->currentBlock, ICmpInst::ICMP_SGT, left, right, "greater"));
}

void ToLLVMVisitor::visit(MIRInt32Le *mirInt32Le) {
    MIRVisitor::visit(mirInt32Le);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new ICmpInst(*this->currentBlock, ICmpInst::ICMP_SLE, left, right, "lesser or equal"));
}

void ToLLVMVisitor::visit(MIRInt32Ge *mirInt32Ge) {
    MIRVisitor::visit(mirInt32Ge);
    auto right = this->valueStack.back();
    this->valueStack.pop_back();
    auto left = this->valueStack.back();
    this->valueStack.pop_back();
    this->valueStack.push_back(new ICmpInst(*this->currentBlock, ICmpInst::ICMP_UGE, left, right, "greater or equal"));
}

void ToLLVMVisitor::visitTrue(MIRTrue *mirTrue) {
    this->valueStack.push_back(ConstantInt::get(Type::getInt1Ty(*this->llvmContext), 1));
}

void ToLLVMVisitor::visitFalse(MIRFalse *mirFalse) {
    this->valueStack.push_back(ConstantInt::get(Type::getInt1Ty(*this->llvmContext), 0));
}

void ToLLVMVisitor::visitIf(MIRIf *mirIf) {
    mirIf->condition->accept(this);
    auto condition = this->valueStack.back();
    this->valueStack.pop_back();
    auto *startBlock = BasicBlock::Create(*this->llvmContext,
                                          "if-start",
                                          this->compilationFunctionStack.back()->llvmFunction);
    auto *endBlock = BasicBlock::Create(*this->llvmContext,
                                        "if-end",
                                        this->compilationFunctionStack.back()->llvmFunction);
    this->valueStack.push_back(BranchInst::Create(startBlock, endBlock, condition, this->currentBlock));


    this->currentBlock = startBlock;
    mirIf->block->blockInitialized = true;
    mirIf->block->accept(this);

    if (!mirIf->jumpOver) {
        //BranchInst::Create(endBlock, this->currentBlock);
    }

    this->currentBlock = endBlock;
    if (mirIf->hasNextBlock()) {
        mirIf->elseBlock->accept(this);
    }
}

void ToLLVMVisitor::visit(MIRToPtr *mirToPtr) {
    mirToPtr->expr->accept(this);
    auto value = this->valueStack.back();
    this->valueStack.pop_back();
    auto *ptr = BitCastInst::Create(Instruction::BitCast,
                                    value,
                                    mirToPtr->getType()->getLLVMType(this->rocLLVMContext),
                                    "to-ptr",
                                    this->currentBlock);
    this->valueStack.push_back(ptr);
}

RocLLVMContext::RocLLVMContext(LLVMContext *llvmContext, Module *module) : llvmContext(llvmContext), module(module) {
    this->voidType = Type::getVoidTy(*llvmContext);

    this->int8Type = Type::getInt8Ty(*llvmContext);
    this->int32Type = Type::getInt32Ty(*llvmContext);
    this->int64Type = Type::getInt64Ty(*llvmContext);

    defineAnyType(this);
    defineStringRawType(this);
    defineStringType(this);
}

FunctionCallee RocLLVMContext::findFunction(const std::string &functionName) {
    auto it = this->definedLLVMFunctions.find(functionName);
    if (it == this->definedLLVMFunctions.end()) {
        throw std::exception(("Could not find defined function: " + functionName).c_str());
    } else {
        return it->second;
    }
}