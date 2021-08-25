#include <stdio.h>
#include <unordered_map>
#include <cstdarg>
#include <iostream>
#include "API.h"
#include <map>
#include <string_view>
#include <cstring>

int toStringId = 0;
int typeIdId = 1;

std::map<INT_64 /* typeId */, ROC_PTR /* vTable ptr */>* vTableMappings = new std::map<INT_64, ROC_PTR>();

void addVTableMapping(INT_64 typeId, ROC_PTR vTablePtr) {
    vTableMappings->insert({typeId, vTablePtr});
}

int myIntToString(char* buffer, const char* format, int n) {
    return sprintf(buffer, format, n);
}

ROC_PTR myStringRawRTypeToString(StringRawRType* stringRawRType) {
    return (ROC_PTR) stringRawRType;
}

ROC_PTR myVTableFactory(int count, ...) {
    va_list args;
    va_start(args, count);
    auto vtable = new std::unordered_map<INT_64 /* function Identifier */, FunctionEntry*>();
    for (int i = 0; i < count; ++i) {
        auto entry = va_arg(args, FunctionEntry*);
        vtable->insert({entry->fIdentifier, entry});
    }
    va_end(args);
    return (ROC_PTR) vtable;
}

ROC_PTR myGetFunctionPointer(AnyRType *anyRType, INT_64 functionIdentifier) {
    auto vTableIt = vTableMappings->find(anyRType->typeId);
    if (vTableIt == vTableMappings->end()) {
        throw std::exception("vTable not initialized for type: " + anyRType->typeId);
    }
    //auto vTable = (std::unordered_map<INT_64 /* function Identifier */, FunctionEntry*>*) anyRType->vTable;
    auto vTable = (std::unordered_map<INT_64, FunctionEntry*>*) vTableIt->second;
    auto it = vTable->find(functionIdentifier);
    if (it == vTable->end()) {
        throw std::exception("Could not find function");
    } else {
        auto entry = it->second;
        return entry->fPtr;
    }
}

void myPrintln(int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        auto anyT = va_arg(args, AnyRType*);
        auto fPtr = myGetFunctionPointer(anyT, toStringId);
        auto fn = (StringRawRType* (*)(AnyRType*)) fPtr;
        std::cout << fn(anyT)->data << " ";
    }
    std::cout << std::endl;
    va_end(args);
}

void myPrint(int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        auto anyT = va_arg(args, AnyRType*);
        auto fPtr = myGetFunctionPointer(anyT, toStringId);
        auto fn = (StringRawRType* (*)(AnyRType*)) fPtr;
        std::cout << fn(anyT)->data;
    }
    va_end(args);
}

long long myInt32ToString(int n) {
    auto result = new StringRawRType();
    auto buffer = new char[12];
    auto length = sprintf(buffer, "%d", n);
    result->length = length;
    result->data = buffer;
    result->refC = 1;
    result->typeId = 2;
    result->vTable = vTableMappings->find(2)->second;
    return (ROC_PTR) result;
}

ROC_PTR myWrapCharPtr(char* rawString) {
    auto result = new StringRawRType();
    result->data = rawString;
    result->refC = 1;
    result->typeId = 2;
    return (ROC_PTR) result;
}

void myInitRawString(StringRawRType* stringRawRType, char* rawString, int length) {
    stringRawRType->data = rawString;
    stringRawRType->typeId = 2;
    stringRawRType->refC = 1;
    stringRawRType->length = length;
}

void myInitInt32(Int32RType* int32RType, int value) {
    int32RType->value = value;
    int32RType->typeId = 4;
    int32RType->refC = 1;
}

void myDecr(AnyRType* any) {
    any->refC = any->refC - 1;
    if (any->refC <= 0) {
        delete any;
    }
}


//djb2 algorithm
unsigned long RawStringHashCode(StringRawRType* t) {
    char* str = t->data;
    unsigned long hash = 5381;
    for (int i = 0; i < t->length; ++i) {
        auto c = (unsigned char) str[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

bool RawStringEquals(StringRawRType* t, StringRawRType* other) {
    if (t->length != other->length) return false;
    for (int i = 0; i < t->length; ++i) {
        if (t->data[i] != other->data[i]) {
            return false;
        }
    }
    return true;
}