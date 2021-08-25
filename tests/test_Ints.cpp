//
// Created by Marcin Bukowiecki on 29.03.2021.
//
#include "Catch.h"
#include "../compiler/RocCompiler.h"
#include "../parser/Parser.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>

TEST_CASE("Return Int 1", "[returnInt1]") {
    auto result = RocCompiler::compile("fun test() -> Int32 {\n"
                                       "  ret 3;\n"
                                       "}\n test()", "Test1");
    if (result) {
        auto ref = (int (*)()) result->EE->getFunctionAddress("test");
        REQUIRE(ref() == 3);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Div Ints 1", "[divInts1]") {
    auto result = RocCompiler::compile("fun test() -> Float64 {\n"
                                       "  ret 8 / 2;\n"
                                       "}\n test()", "Test1");
    if (result) {
        auto ref = (double (*)()) result->EE->getFunctionAddress("test");
        REQUIRE(ref() == 4.0);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Div Ints 2", "[divInts2]") {
    auto result = RocCompiler::compile("fun test(a Int32) -> Float64 {\n"
                                       "  ret 9 / a;\n"
                                       "}\n test(3)", "Test1");
    if (result) {
        auto ref = (double (*)(int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(3) == 3.0);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Div Ints 3", "[divInts3]") {
    auto result = RocCompiler::compile("fun test(a Int32, b Int32) -> Float64 {\n"
                                       "  ret a / b;\n"
                                       "}\n test(60, 2)", "Test1");
    if (result) {
        auto ref = (double (*)(int, int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(60, 2) == 30.0);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Mul Ints 1", "[mulInts1]") {
    auto result = RocCompiler::compile("fun test() -> Int32 {\n"
                                       "  ret 8 * 2;\n"
                                       "}\n test()", "Test1");
    if (result) {
        auto ref = (int (*)()) result->EE->getFunctionAddress("test");
        REQUIRE(ref() == 16);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Mul Ints 2", "[mulInts2]") {
    auto result = RocCompiler::compile("fun test(a Int32) -> Int32 {\n"
                                       "  ret 9 * a;\n"
                                       "}\n test(3)", "Test1");
    if (result) {
        auto ref = (int (*)(int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(3) == 27);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Mul Ints 3", "[mulInts3]") {
    auto result = RocCompiler::compile("fun test(a Int32, b Int32) -> Int32 {\n"
                                       "  ret a * b;\n"
                                       "}\n test(60, 2)", "Test1");
    if (result) {
        auto ref = (int (*)(int, int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(60, 2) == 120);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Add Ints 1", "[addInts1]") {
    auto result = RocCompiler::compile("fun test() -> Int32 {\n"
                                       "  ret 1 + 3;\n"
                                       "}\n test()", "Test1");
    if (result) {
        auto ref = (int (*)()) result->EE->getFunctionAddress("test");
        REQUIRE(ref() == 4);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Add Ints 2", "[addInts2]") {
    auto result = RocCompiler::compile("fun test(a Int32) -> Int32 {\n"
                                       "  ret 1 + a;\n"
                                       "}\n test(3)", "Test1");
    if (result) {
        auto ref = (int (*)(int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(3) == 4);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Add Ints 3", "[addInts3]") {
    auto result = RocCompiler::compile("fun test(a Int32, b Int32) -> Int32 {\n"
                                       "  ret a + b;\n"
                                       "}\n test(1, 3)", "Test1");
    if (result) {
        auto ref = (int (*)(int, int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(12, 56) == 68);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Add Ints 4", "[addInts4]") {
    try {
        RocCompiler::compile("fun test(a Int32, b Int32) -> Int32 {\n"
                                           "  ret \"a\" + b;\n"
                                           "}\n test(1, 3)", "Test1");
    } catch (SyntaxException &ex) {
        REQUIRE(ex.message == "Invalid operation");
    }
}

TEST_CASE("Sub Ints 1", "[subInts1]") {
    auto result = RocCompiler::compile("fun test() -> Int32 {\n"
                                       "  ret 1 - 3;\n"
                                       "}\n test()", "Test1");
    if (result) {
        auto ref = (int (*)()) result->EE->getFunctionAddress("test");
        REQUIRE(ref() == -2);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Sub Ints 2", "[subInts2]") {
    auto result = RocCompiler::compile("fun test(a Int32) -> Int32 {\n"
                                       "  ret 1 - a;\n"
                                       "}\n test(3)", "Test1");
    if (result) {
        auto ref = (int (*)(int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(3) == -2);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Sub Ints 3", "[subInts3]") {
    auto result = RocCompiler::compile("fun test(a Int32) -> Int32 {\n"
                                       "  ret 1 - a + 5;\n"
                                       "}\n test(3)", "Test1");
    if (result) {
        auto ref = (int (*)(int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(3) == 3);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Print Int 1", "[printInt1]") {
    auto result = RocCompiler::compile("fun test(a Int32) -> Int32 {\n"
                                       "  println(a.toString());\n"
                                       "  ret 1;\n"
                                       "}\n test(123)", "Test1");
    if (result) {
        auto ref = (int (*)()) result->EE->getFunctionAddress("main");
        REQUIRE(ref() == 0);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Comparing ints 1", "[comparingInts1]") {
    auto result = RocCompiler::compile("package main\n fun test(a i32) -> Bool {\n"
                                       "  ret a == 67;\n"
                                       "}\n test(67)", "Test1");
    if (result) {
        auto ref = (bool (*)(int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(67) == true);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Comparing ints 2", "[comparingInts2]") {
    auto result = RocCompiler::compile("fun test(a Int32, b Int32) -> Bool {\n"
                                       "  ret a == b;\n"
                                       "}", "Test1");
    if (result) {
        auto ref = (bool (*)(int, int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(67, 78) == false);
        REQUIRE(ref(78, 78) == true);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Comparing ints 3", "[comparingInts3]") {
    auto result = RocCompiler::compile("fun test(a Int32, b Int32) -> Bool {\n"
                                       "  ret a == b and a == 78;\n"
                                       "}", "Test1");
    if (result) {
        auto ref = (bool (*)(int, int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(67, 78) == false);
        REQUIRE(ref(78, 78) == true);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Comparing ints 4", "[comparingInts4]") {
    auto result = RocCompiler::compile("fun test(a Int32, b Int32) -> Bool {\n"
                                       "  ret a == b and a != 78;\n"
                                       "}", "Test1");
    if (result) {
        auto ref = (bool (*)(int, int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(67, 78) == false);
        REQUIRE(ref(79, 79) == true);
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Comparing ints 5", "[comparingInts5]") {
    auto result = RocCompiler::compile("fun test(a Int32, b Int32) -> Bool {\n"
                                       "  if a == b then \n"
                                       "    ret true\n"
                                       "  end\n"
                                       "  ret false"
                                       "}", "Test1");
    if (result) {
        auto ref = (bool (*)(int, int)) result->EE->getFunctionAddress("test");
        REQUIRE(ref(78, 78) == true);
        REQUIRE(ref(79, 78) == false);
    } else {
        REQUIRE(false);
    }
}