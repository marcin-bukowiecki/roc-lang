//
// Created by Marcin Bukowiecki on 06.04.2021.
//
#include "Catch.h"
#include "../compiler/RocCompiler.h"
#include "../parser/Parser.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>

bool arrayEquals(const int* arr, int size, const int* expected) {
    for (int i = 0; i < size; ++i) {
        if (arr[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

TEST_CASE("Return array 1", "[returnArray1]") {
    auto result = RocCompiler::compile("fun test() -> []Int32 {\n"
                                       "  ret [1,2];\n"
                                       "}\n test()", "Test1");
    if (result) {
        auto ref = (int* (*)()) result->EE->getFunctionAddress("test");
        REQUIRE(arrayEquals(ref(), 2, new int[] {1,2}));
    } else {
        REQUIRE(false);
    }
}