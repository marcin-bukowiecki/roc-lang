//
// Created by Marcin Bukowiecki on 28.03.2021.
//

#include <llvm/IR/IRBuilder.h>
#include "LLVMUtils.h"
#include "Types.h"
#include "LLVMBackend.h"
#include "Builtins.h"

using namespace llvm;

Value* getFieldFromStruct(LLVMContext *llvmContext,
                          Type *structType,
                          Value *ptrValue,
                          int fieldIndex,
                          Type *fieldType,
                          BasicBlock *place);

void createVTable(RocLLVMContext *rocLlvmContext,
                  RocType *rocType,
                  std::vector<RocType*> matchingTraits,
                  std::map<int, std::vector<VTableEntry*>> traitEntries);

/**
 * Helper function to create a C puts function call to do some logging stuff
 *
 * @param llvmContext LLVM context
 * @param module compiling LLVM module
 * @param text text to print by puts function
 * @param place where to insert this function call
 */
void createPuts(LLVMContext *llvmContext, Module *module, const std::string& text, BasicBlock *place) {
    FunctionType *ft = FunctionType::get(Type::getInt32Ty(*llvmContext), {
        Type::getInt8PtrTy(*llvmContext),
        }, false);
    auto putsFunc = module->getOrInsertFunction("puts", ft);
    IRBuilder<> builder(*llvmContext);
    Value *textValue = builder.CreateGlobalStringPtr(text, "", 0, module);
    CallInst::Create(putsFunc,
                     { textValue },
                     "puts",
                     place);
}

Value* newArray(RocLLVMContext *rocLLVMContext,
                Type *type,
                int size,
                BasicBlock *place,
                const std::string& name) {

    auto* llvmContext = rocLLVMContext->llvmContext;
    auto ref = callMalloc(llvmContext,
                          rocLLVMContext->module,
                                 size,
                                 place,
                                 name);
    auto* cast = BitCastInst::Create(Instruction::BitCast,
                                     ref,
                                     type->getPointerTo(),
                                     name + "-cast",
                                     place);
    return cast;
}

Value* castTo(Value* from, Type *to, BasicBlock *place) {
    return BitCastInst::Create(Instruction::BitCast, from, to, "cast-1", place);
}

Value* storeValue(Value* value, Value *to, BasicBlock *place) {
    return new StoreInst(value, to, false, place);
}

Value* getPointerTo(RocLLVMContext *rocLLVMContext, Value* from, BasicBlock *place, int index) {
    std::vector<Value*> gepArgs;
    gepArgs.push_back(ConstantInt::get(Type::getInt64Ty(*rocLLVMContext->llvmContext), index));
    auto* gep = GetElementPtrInst::Create(rocLLVMContext->int32Type,
                                                   from,
                                                   gepArgs,
                                                   "gep-1",
                                                   place);
    return gep;
}

Value* callMalloc(LLVMContext *llvmContext,
                  Module *module,
                  long long size,
                  BasicBlock *place,
                  const std::string& name) {

    IRBuilder<> builder(*llvmContext);
    builder.SetInsertPoint(place);
    auto* mallocFunType = FunctionType::get(Type::getInt8PtrTy(*llvmContext), {
            Type::getInt64Ty(*llvmContext),
    }, false);
    auto mallocFun = module->getOrInsertFunction("malloc", mallocFunType);
    auto* result = builder.CreateCall(
            mallocFun,
           {  ConstantInt::get(Type::getInt64Ty(*llvmContext), size) },
            name);
    return result;
}

void createFunctionDispatcherForAny(RocLLVMContext *rocLLVMContext) {

    auto* llvmContext = rocLLVMContext->llvmContext;
    auto* module = rocLLVMContext->module;

    IRBuilder<> builder(*llvmContext);

    auto functionType = FunctionType::get(Type::getInt64Ty(*llvmContext), {
        rocLLVMContext->anyTypeStructType->getPointerTo(), //any type ptr
        rocLLVMContext->int64Type, //target function index
    }, false);
    auto functionDispatcherFun = Function::Create(functionType,
                                                Function::ExternalLinkage,
                                                "AnyFunctionDispatcher",
                                                module);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", functionDispatcherFun);
    builder.SetInsertPoint(entry);

    auto* vtableField = getFieldFromStruct(llvmContext,
                       rocLLVMContext->anyTypeStructType,
                       functionDispatcherFun->getArg(0),
                       0,
                       rocLLVMContext->functionEntryType->getPointerTo()->getPointerTo(),
                       entry);

    auto targetFunctionCall = module->getOrInsertFunction("FunctionDispatcher", FunctionType::get(Type::getInt64Ty(*llvmContext),
                                                                                                  { vtableField->getType(), rocLLVMContext->int32Type, rocLLVMContext->int64Type },
                                                                                                  false));
    auto* result = CallInst::Create(targetFunctionCall,
                     { vtableField,
                       ConstantInt::get(Type::getInt32Ty(*llvmContext), 1),
                       functionDispatcherFun->getArg(1)
                       },
                     "FunctionDispatcher",
                     entry);
    ReturnInst::Create(*llvmContext, result, entry);
}

Value* getFieldFromStruct(LLVMContext *llvmContext,
                          Type *structType,
                          Value *ptrValue,
                          int fieldIndex,
                          Type *fieldType,
                          BasicBlock *place) {

    std::vector<Value*> gepArgs;
    gepArgs.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 0));
    gepArgs.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), fieldIndex));
    auto* gep = GetElementPtrInst::Create(structType,
                                           ptrValue,
                                           gepArgs,
                                           "",
                                           place);
    auto* loadInst = new LoadInst(fieldType, gep, "", place);
    return loadInst;
}

void createGetTypeIdFunction(RocLLVMContext *rocLlvmContext, RocType *rocType) {
    auto* llvmContext = rocLlvmContext->llvmContext;
    auto* module = rocLlvmContext->module;

    IRBuilder<> builder(*llvmContext);

    auto functionType = FunctionType::get(Type::getInt32Ty(*llvmContext), {
            rocLlvmContext->int32StructType
        }, false);
    auto function = Function::Create(functionType,
                                                  Function::ExternalLinkage,
                                                  rocType->prettyName() + ".typeId.0",
                                                  module);

    auto callee = module->getOrInsertFunction(rocType->prettyName() + ".typeId.0", functionType);
    rocLlvmContext->definedLLVMFunctions.insert({rocType->prettyName() + ".typeId.0", callee});

    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", function);
    builder.SetInsertPoint(entry);
    ReturnInst::Create(*llvmContext, ConstantInt::get(Type::getInt32Ty(*llvmContext), rocType->typeId()), entry);
}

void createInt32VTable(RocLLVMContext *rocLlvmContext) {
    auto typeId = std::unique_ptr<VTableEntry>(new BuiltinVTableEntry(
            "typeId",
            rocLlvmContext->findFunction("Int32.typeId.0")
            ));
    auto toString = std::unique_ptr<VTableEntry>(new BuiltinVTableEntry(
            "toString",
            rocLlvmContext->findFunction("Int32.toString.0")
            ));

    std::map<int, std::vector<VTableEntry*>> traitEntries;

    std::vector<RocType*> matchingTraits;
    auto anyType = std::make_unique<RocAnyType>();
    matchingTraits.push_back(anyType.get());
    traitEntries.insert({
       anyType->typeId(),
       {
           typeId.get(),
           toString.get(),
       }
    });

    auto int32Type = std::make_unique<RocInt32Type>();
    createVTable(rocLlvmContext, int32Type.get(), matchingTraits, traitEntries);
}

void createStringRawVTable(RocLLVMContext *rocLlvmContext) {
    defineStringRawToStringRaw(rocLlvmContext);
    defineStringRawTypeId(rocLlvmContext);

    auto typeId = std::unique_ptr<VTableEntry>(new BuiltinVTableEntry(
            "typeId",
            rocLlvmContext->findFunction("StringRaw.typeId.0")
            ));
    auto toString = std::unique_ptr<VTableEntry>(new BuiltinVTableEntry(
            "toString",
            rocLlvmContext->findFunction("StringRaw.toString.0")
            ));

    std::map<int, std::vector<VTableEntry*>> traitEntries;
    std::vector<RocType*> matchingTraits;
    auto anyType = std::make_unique<RocAnyType>();
    matchingTraits.push_back(anyType.get());
    traitEntries.insert({
                                anyType->typeId(),
                                {
                                        typeId.get(),
                                        toString.get(),
                                }
                        });

    auto t = std::make_unique<RocRawStringType>(-1);
    createVTable(rocLlvmContext, t.get(), matchingTraits, traitEntries);
}

void createVTable(RocLLVMContext *rocLlvmContext,
                  RocType *rocType,
                  std::vector<RocType*> matchingTraits,
                  std::map<int, std::vector<VTableEntry*>> traitEntries) {

    auto llvmContext = rocLlvmContext->llvmContext;
    auto module = rocLlvmContext->module;

    IRBuilder<> builder(*llvmContext);

    auto vtableGlobal = module->getOrInsertGlobal(
            rocType->prettyName() + ".vtable.traits",
            rocLlvmContext->int64Type //unordered map
            );
    GlobalVariable *gv = module->getNamedGlobal(rocType->prettyName() + ".vtable.traits");
    gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
    gv->setAlignment(MaybeAlign(8));
    gv->setInitializer(ConstantInt::get(Type::getInt64Ty(*llvmContext), 0)); //init to zero
    auto functionType = FunctionType::get(
            rocLlvmContext->int64Type,
            { },
            false);
    auto vtableGlobalInit = Function::Create(functionType,
                                             Function::ExternalLinkage,
                                             rocType->prettyName() + ".vtable.init",
                                             module);
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entrypoint", vtableGlobalInit);
    builder.SetInsertPoint(entry);

    std::vector<Value*> fPointers;
    fPointers.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 2)); //number of pointers
    int k = 0;
    for (auto& rt : matchingTraits) {
        auto entries = traitEntries.find(rt->typeId())->second;
        int i = 0;
        for (auto& e : entries) {
            FunctionCallee f = e->getFunctionCallee();
            auto typeId = rt->typeId();
            auto fPtr = CastInst::CreatePointerCast(f.getCallee(), rocLlvmContext->int64Type, "", entry);
            auto fIdentifier = e->getFunctionId();

            auto structRef = callMalloc(rocLlvmContext->llvmContext,
                                        module,
                                        8 + 8 + 8,
                                        entry,
                                        "struct-malloc");
            auto* cast = BitCastInst::Create(Instruction::BitCast,
                                              structRef,
                                              rocLlvmContext->functionEntryType->getPointerTo(),
                                              "",
                                              entry);
            //Set type id
            std::vector<Value*> gep1Args;
            gep1Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 0));
            gep1Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 0));
            auto* gep1 = GetElementPtrInst::Create(rocLlvmContext->functionEntryType,
                                                   cast,
                                                   gep1Args,
                                                   "",
                                                   entry);
            new StoreInst(ConstantInt::get(Type::getInt64Ty(*llvmContext), typeId),
                          gep1,
                          false,
                          entry);

            //fPtr
            std::vector<Value*> gep2Args;
            gep2Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 0));
            gep2Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 1));
            auto* gep2 = GetElementPtrInst::Create(rocLlvmContext->functionEntryType,
                                                   cast,
                                                   gep2Args,
                                                   "",
                                                   entry);
            new StoreInst(fPtr,
                          gep2,
                          false,
                          entry);

            //fIdentifier
            std::vector<Value*> gep3Args;
            gep3Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 0));
            gep3Args.push_back(ConstantInt::get(Type::getInt32Ty(*llvmContext), 2));
            auto* gep3 = GetElementPtrInst::Create(rocLlvmContext->functionEntryType,
                                                   cast,
                                                   gep3Args,
                                                   "",
                                                   entry);
            new StoreInst(ConstantInt::get(Type::getInt64Ty(*llvmContext), fIdentifier),
                          gep3,
                          false,
                          entry);

            fPointers.push_back(cast);
            i++;
        }
        k++;
    }

    auto* ft = FunctionType::get(Type::getInt64Ty(*llvmContext), {
            rocLlvmContext->int32Type,
    }, true);
    auto f = module->getOrInsertFunction("myVTableFactory", ft);
    auto vtableMap = builder.CreateCall(f, fPointers, "call-myVTableFactory");
    new StoreInst(vtableMap, vtableGlobal, false, entry);

    auto vtableSetterFT = FunctionType::get(rocLlvmContext->voidType, {
            rocLlvmContext->int64Type,
            rocLlvmContext->int64Type,
    }, false);
    auto vtableSetter = module->getOrInsertFunction("addVTableMapping", vtableSetterFT);

    //Return statement
    auto* loadInst = new LoadInst(rocLlvmContext->int64Type,
                                  vtableGlobal,
                                  "get-global",
                                  entry);
    builder.CreateCall(vtableSetter, {
        ConstantInt::get(Type::getInt64Ty(*llvmContext), rocType->typeId()),
        loadInst }, "");
    ReturnInst::Create(*llvmContext, loadInst, entry);
}

Value* newRocRawStruct(ToLLVMVisitor *visitor, Value* rawString, int length) {
    auto rawStrAlloc = new AllocaInst(visitor->rocLLVMContext->stringRawType,
                                      0,
                                      "new-raw-string",
                                      visitor->currentBlock);
    auto ft = FunctionType::get(Type::getVoidTy(*visitor->llvmContext), {
            visitor->rocLLVMContext->stringRawType->getPointerTo(),
            Type::getInt8PtrTy(*visitor->llvmContext),
            Type::getInt32Ty(*visitor->llvmContext),
    }, false);
    auto f = visitor->module->getOrInsertFunction("myInitRawString", ft);
    CallInst::Create(f, {
        rawStrAlloc,
                          rawString,
                          ConstantInt::get(Type::getInt32Ty(*visitor->llvmContext), length)
                          }, "", visitor->currentBlock);
    return rawStrAlloc;
}

Value* newRocInt32Struct(ToLLVMVisitor *visitor, Value* value) {
    auto alloc = new AllocaInst(visitor->rocLLVMContext->int32StructType,
                                      0,
                                      "new-int-32",
                                      visitor->currentBlock);
    auto ft = FunctionType::get(Type::getVoidTy(*visitor->llvmContext), {
            visitor->rocLLVMContext->int32StructType->getPointerTo(),
            Type::getInt32Ty(*visitor->llvmContext),
    }, false);
    auto f = visitor->module->getOrInsertFunction("myInitInt32", ft);
    CallInst::Create(f, { alloc, value }, "", visitor->currentBlock);
    return alloc;
}