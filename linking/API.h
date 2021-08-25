//
// Created by Marcin Bukowiecki on 28.03.2021.
//
#pragma once
#ifndef ROC_LANG_API_H
#define ROC_LANG_API_H

typedef long long INT_64;
typedef long long ROC_PTR;

struct AnyRType {
    ROC_PTR vTable; //pointer to virtual table
    INT_64 typeId; //type id
    INT_64 refC; //reference counter
};

struct StringRawRType : public AnyRType {
    char* data; //array of chars
    int length; //length or array
};

struct StringRType : public AnyRType {
    char* data;
    int length;
};

struct FunctionEntry {
    INT_64 typeId; //type id (owner of function)
    ROC_PTR fPtr; //function pointer
    INT_64 fIdentifier; //function identifier
};

struct Int32RType : public AnyRType {
    int value;
};

struct ArrayRType : public AnyRType {
    int length;
};

struct Int32ArrayRType : public ArrayRType {
    int* elements;
};

extern "C" void addVTableMapping(long long typeId, long long vTablePtr);

extern "C" int myIntToString(char* buffer, const char* format, int s);

extern "C" ROC_PTR myStringRawRTypeToString(StringRawRType* stringRawRType);

extern "C" ROC_PTR myVTableFactory(int count, ...);

extern "C" ROC_PTR myGetFunctionPointer(AnyRType *anyRType, long long functionIdentifier);

extern "C" void myPrintln(int count, ...);

extern "C" void myPrint(int count, ...);

extern "C" ROC_PTR myInt32ToString(int n);

extern "C" void myInitRawString(StringRawRType* stringRawRType, char* rawString, int length);

extern "C" void myInitInt32(Int32RType* int32RType, int value);

extern "C" void myDecr(AnyRType *any);



extern "C" unsigned long RawStringHashCode(StringRawRType* t);

extern "C" bool RawStringEquals(StringRawRType* t, StringRawRType* other);

#endif //ROC_LANG_API_H
