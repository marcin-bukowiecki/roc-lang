//
// Created by Marcin Bukowiecki on 19.03.2021.
//
#include "Builtins.h"
#include "LLVMUtils.h"
#include "LLVMBackend.h"
#include <llvm/IR/IRBuilder.h>

BuiltinFunctionResolver::BuiltinFunctionResolver() {
    auto bf = new BuiltinFunction("println",
                                  {
                                          new RocAnyType()
                                  },
                                  new UnitRocType());
    addFunction(bf);

    bf = new BuiltinFunction("ccall",
                                  {
                                          new RocRawStringType(-1),
                                          new RocAnyType()
                                  },
                                  new RocAnyType(),
                                  true);
    addFunction(bf);
}

void defineAnyType(RocLLVMContext *rocLlvmContext) {
    auto *functionEntryType = StructType::create(*rocLlvmContext->llvmContext, {
            rocLlvmContext->int64Type, //typeId
            rocLlvmContext->int64Type, //fPtr
            rocLlvmContext->int64Type, //fIdentifier
    }, "FunctionEntry");
    rocLlvmContext->functionEntryType = functionEntryType;

    auto *anyTypeStructType = StructType::create(*rocLlvmContext->llvmContext, {
            rocLlvmContext->int64Type, //vtablePtr
            rocLlvmContext->int64Type, //typeId
            rocLlvmContext->int64Type, //refC
    }, "AnyType");
    rocLlvmContext->anyTypeStructType = anyTypeStructType;
}

void defineStringRawType(RocLLVMContext *rocLlvmContext) {
    auto *stringRawStructType = StructType::create(*rocLlvmContext->llvmContext, {
            rocLlvmContext->int64Type, //vtablePtr
            rocLlvmContext->int64Type, //typeId
            rocLlvmContext->int64Type, //refC
            PointerType::get(rocLlvmContext->int8Type, 0), //string content ptr
            rocLlvmContext->int32Type, //length
    }, "StringRawType");
    rocLlvmContext->stringRawType = stringRawStructType;
}

void defineStringType(RocLLVMContext *rocLlvmContext) {
    auto *stringStructType = StructType::create(*rocLlvmContext->llvmContext, {
            rocLlvmContext->int64Type, //vtablePtr
            rocLlvmContext->int64Type, //typeId
            rocLlvmContext->int64Type, //refC
            PointerType::get(rocLlvmContext->int8Type, 0), //string content ptr
            rocLlvmContext->int32Type, //length
    }, "StringType");
    rocLlvmContext->stringType = stringStructType;
}

void createAnyToString(RocLLVMContext *rocLlvmContext) {
    auto *module = rocLlvmContext->module;
    auto *llvmContext = rocLlvmContext->llvmContext;
    auto toStringFunType = FunctionType::get(rocLlvmContext->stringType->getPointerTo(), {
            PointerType::get(rocLlvmContext->anyTypeStructType, 0),
    }, false);
    auto toStringFun = Function::Create(toStringFunType, Function::ExternalLinkage, "Any.toString.0", module);

    IRBuilder<> Builder(*llvmContext);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", toStringFun);
    Builder.SetInsertPoint(entry);

    ReturnInst::Create(*llvmContext, ConstantPointerNull::get(PointerType::get(rocLlvmContext->stringType, 0)), entry);
}

void defineIntType(RocLLVMContext *rocLlvmContext) {
    auto *module = rocLlvmContext->module;

    auto *llvmContext = rocLlvmContext->llvmContext;

    auto *int32StructType = StructType::create(*llvmContext, {
            rocLlvmContext->int64Type, //type vtablePtr
            rocLlvmContext->int64Type, //typeId
            rocLlvmContext->int64Type, //refC
            rocLlvmContext->int32Type, //value
    }, "Int32Type");
    rocLlvmContext->int32StructType = int32StructType;

    auto int32Type = std::make_unique<RocInt32Type>();
    createGetTypeIdFunction(rocLlvmContext, int32Type.get());

    auto toStringFunType = FunctionType::get(rocLlvmContext->stringRawType->getPointerTo(), {
            PointerType::get(int32StructType, 0),
    }, false);
    auto toStringFun = Function::Create(toStringFunType, Function::ExternalLinkage, "Int32.toString.0", module);
    auto callee = module->getOrInsertFunction("Int32.toString.0", toStringFunType);
    rocLlvmContext->definedLLVMFunctions.insert({"Int32.toString.0", callee});

    IRBuilder<> Builder(*llvmContext);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", toStringFun);
    Builder.SetInsertPoint(entry);

    auto *mallocFunType = FunctionType::get(Type::getInt8PtrTy(*llvmContext), {
            Type::getInt64Ty(*llvmContext),
    }, false);
    auto mallocFun = module->getOrInsertFunction("malloc", mallocFunType);
    auto *charArray = Builder.CreateCall(mallocFun, {ConstantInt::get(Type::getInt64Ty(*llvmContext), 12)},
                                         "call-malloc");

    auto *stringStructAllocation = Builder.CreateCall(mallocFun, {
            ConstantInt::get(Type::getInt64Ty(*llvmContext), 36)
    }, "call-malloc-for-string-struct");
    auto *stringStructReference = BitCastInst::Create(Instruction::BitCast,
                                                      stringStructAllocation,
                                                      rocLlvmContext->stringRawType->getPointerTo(),
                                                      "struct-struct-ref",
                                                      entry);


    std::vector<Value *> gep1Args;
    gep1Args.push_back(ConstantInt::get(Type::getInt64Ty(*llvmContext), 0));
    gep1Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 3));
    auto gep1 = GetElementPtrInst::Create(int32StructType,
                                          toStringFun->getArg(0),
                                          gep1Args,
                                          "gep1", entry);
    auto *loadInst = new LoadInst(rocLlvmContext->int32Type, gep1, "get-int32-value", entry);
    auto *sprintfFT = FunctionType::get(Type::getInt32Ty(*llvmContext), {
            Type::getInt8PtrTy(*llvmContext),
            Type::getInt8PtrTy(*llvmContext),
            rocLlvmContext->int32Type,
    }, false);
    auto sprintfFun = module->getOrInsertFunction("myIntToString", sprintfFT);
    auto dFormatString = Builder.CreateGlobalStringPtr("%d");
    auto *stringLength = Builder.CreateCall(sprintfFun, {charArray, dFormatString, loadInst}, "call-sprintf");

    std::vector<Value *> gepArgs;
    gepArgs.push_back(ConstantInt::get(Type::getInt64Ty(*llvmContext), 24));
    auto *charArrayGep = GetElementPtrInst::Create(Type::getInt8Ty(*llvmContext),
                                                   stringStructAllocation,
                                                   gepArgs,
                                                   "charArrayGep", entry);
    auto *charArrayGepCast = BitCastInst::Create(Instruction::BitCast, charArrayGep,
                                                 Type::getInt8PtrTy(*llvmContext)->getPointerTo(), "", entry);
    new StoreInst(charArray, charArrayGepCast, false, entry);


    std::vector<Value *> gepArgs2;
    gepArgs2.push_back(ConstantInt::get(Type::getInt64Ty(*llvmContext), 32));
    auto *lengthGep = GetElementPtrInst::Create(Type::getInt8Ty(*llvmContext),
                                                stringStructAllocation,
                                                gepArgs2,
                                                "lengthGep", entry);
    auto *lengthGepCast = BitCastInst::Create(Instruction::BitCast, lengthGep,
                                              Type::getInt32Ty(*llvmContext)->getPointerTo(), "", entry);
    new StoreInst(stringLength, lengthGepCast, false, entry);

    ReturnInst::Create(*llvmContext, stringStructReference, entry);
}

void definePrintln(RocLLVMContext *rocLlvmContext) {
    auto *module = rocLlvmContext->module;
    auto *llvmContext = rocLlvmContext->llvmContext;
    auto *printlnFT = FunctionType::get(Type::getVoidTy(*rocLlvmContext->llvmContext), {
            rocLlvmContext->anyTypeStructType->getPointerTo(),
    }, false);
    auto *printlnF = Function::Create(printlnFT, Function::ExternalLinkage, "println", module);

    IRBuilder<> Builder(*llvmContext);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", printlnF);
    Builder.SetInsertPoint(entry);

    auto *ft = FunctionType::get(Type::getVoidTy(*llvmContext), {
            Type::getInt32Ty(*llvmContext),
    }, true);
    auto f = module->getOrInsertFunction("myPrintln", ft);
    Builder.CreateCall(f, {
            ConstantInt::get(Type::getInt32Ty(*llvmContext), 1),
            printlnF->getArg(0),
    }, "");

    ReturnInst::Create(*llvmContext, entry);
}

/**
 * Defines print(any *Any) function
 *
 * @param rocLlvmContext
 */
void definePrint(RocLLVMContext *rocLlvmContext) {
    auto *module = rocLlvmContext->module;
    auto *llvmContext = rocLlvmContext->llvmContext;
    auto *printlnFT = FunctionType::get(Type::getVoidTy(*rocLlvmContext->llvmContext), {
            PointerType::get(rocLlvmContext->anyTypeStructType, 0),
    }, true);
    auto *printlnF = Function::Create(printlnFT, Function::ExternalLinkage, "print", module);

    IRBuilder<> Builder(*llvmContext);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", printlnF);
    Builder.SetInsertPoint(entry);

    auto *ft = FunctionType::get(Type::getVoidTy(*llvmContext), {
            Type::getInt32Ty(*llvmContext),
    }, true);
    auto f = module->getOrInsertFunction("myPrint", ft);
    Builder.CreateCall(f, {
            ConstantInt::get(Type::getInt32Ty(*llvmContext), 1),
            printlnF->getArg(0),
    }, "");

    ReturnInst::Create(*llvmContext, entry);
}

/**
 * Creates call to int32ToString(int n) function
 */
void defineInt32ToStringRaw(RocLLVMContext *rocLlvmContext) {
    auto *module = rocLlvmContext->module;
    auto *llvmContext = rocLlvmContext->llvmContext;
    auto *int32ToStringFT = FunctionType::get(rocLlvmContext->stringRawType->getPointerTo(), {
            rocLlvmContext->int32Type,
    }, false);
    auto *int32ToStringF = Function::Create(int32ToStringFT,
                                            Function::ExternalLinkage,
                                            "int32ToString",
                                            module);

    IRBuilder<> Builder(*llvmContext);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", int32ToStringF);
    Builder.SetInsertPoint(entry);

    auto *ft = FunctionType::get(Type::getInt64Ty(*llvmContext), {
            Type::getInt32Ty(*llvmContext),
    }, false);
    auto f = module->getOrInsertFunction("myInt32ToString", ft);
    auto result = Builder.CreateCall(f, {
            int32ToStringF->getArg(0),
    }, "");

    auto ptr = CastInst::Create(llvm::Instruction::IntToPtr, result, rocLlvmContext->stringRawType->getPointerTo(),
                                "", entry);

    ReturnInst::Create(*llvmContext, ptr, entry);
}

/**
 * Creates call to stringRawRTypeToString(StringRawRType* stringRawRType) function
 *
 * @param rocLlvmContext RocLLVMContext context
 */
void defineStringRawToStringRaw(RocLLVMContext *rocLlvmContext) {
    auto *module = rocLlvmContext->module;
    auto *llvmContext = rocLlvmContext->llvmContext;
    auto *ft = FunctionType::get(rocLlvmContext->stringRawType->getPointerTo(), {
            rocLlvmContext->stringRawType->getPointerTo(),
    }, false);
    auto *f = Function::Create(ft, Function::ExternalLinkage, "StringRaw.toString.0", module);

    IRBuilder<> Builder(*llvmContext);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", f);
    Builder.SetInsertPoint(entry);

    auto *targetFt = FunctionType::get(Type::getInt64Ty(*llvmContext), {
            rocLlvmContext->stringRawType->getPointerTo(),
    }, false);
    auto targetF = module->getOrInsertFunction("myStringRawRTypeToString", targetFt);
    auto result = Builder.CreateCall(targetF, {f->getArg(0),}, "");

    auto ptr = CastInst::Create(llvm::Instruction::IntToPtr,
                                result,
                                rocLlvmContext->stringRawType->getPointerTo(),
                                "",
                                entry);
    rocLlvmContext->definedLLVMFunctions.insert({"StringRaw.toString.0", f});
    ReturnInst::Create(*llvmContext, ptr, entry);
}

/**
 * Creates call to typeId() function
 *
 * @param rocLlvmContext RocLLVMContext context
 */
void defineStringRawTypeId(RocLLVMContext *rocLlvmContext) {
    auto *module = rocLlvmContext->module;
    auto *llvmContext = rocLlvmContext->llvmContext;
    auto *ft = FunctionType::get(rocLlvmContext->int32Type, {
            rocLlvmContext->stringRawType->getPointerTo(),
    }, false);
    auto *f = Function::Create(ft, Function::ExternalLinkage, "StringRaw.typeId.0", module);

    IRBuilder<> Builder(*llvmContext);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", f);
    Builder.SetInsertPoint(entry);
    rocLlvmContext->definedLLVMFunctions.insert({"StringRaw.typeId.0", f});
    ReturnInst::Create(*llvmContext,
                       ConstantInt::get(Type::getInt32Ty(*llvmContext), 2),
                       entry);
}