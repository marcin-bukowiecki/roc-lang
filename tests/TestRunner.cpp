//
// Created by Marcin on 30.04.2021.
//

#ifndef ROCTESTS_TESTRUNNER_CPP
#define ROCTESTS_TESTRUNNER_CPP
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include "Catch.h"
#include "../compiler/RocCompiler.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <string>
#include <fstream>
#include <iostream>

#ifdef SANDBOX_DIR
#define SANDBOX_DIR_PATH SANDBOX_DIR
#else
#define SANDBOX_DIR_PATH ""
#endif

#ifdef SDK_DIR
#define SDK_DIR_PATH SDK_DIR
#else
#define SDK_DIR_PATH ""
#endif

#include <filesystem> // C++17 standard header file name
#include <experimental/filesystem> // Header file for pre-standard implementation
using namespace std::experimental::filesystem::v1;

class TestRunner {
public:

    static std::string getPath() {
        std::string path(SANDBOX_DIR_PATH);
        return path + "/runner";
    }

    static std::string getSdkPath() {
        std::string path(SDK_DIR);
        return path;
    }

    static void initMain(RocCompilationResult* result) {
        auto main = (int (*)()) result->EE->getFunctionAddress("main");
        main();
    }

    static bool runAll(std::string mainPath) {
        bool allPassed = true;
        for (const auto & entry : directory_iterator(mainPath)) {
            const auto& entryPath = entry.path();
            if (is_directory(entryPath)) {
                auto testName = entryPath.filename();
                std::cout << "Running test for: " << testName << std::endl;
                for (const auto & p : directory_iterator(mainPath + "/" + testName.string())) {
                    auto strPath = p.path().string();
                    auto compilationResult = RocCompiler::compile(strPath);
                    if (compilationResult == nullptr) {
                        std::cout << "FAILED: " + p.path().filename().string() << std::endl;
                        allPassed = false;
                        continue;
                    }
                    initMain(compilationResult);

                    auto ref = (bool (*)()) compilationResult->EE->getFunctionAddress("box");

                    if (ref == nullptr) {
                        std::cerr << "box() not found";
                        continue;
                    }
                    if (ref()) {
                        std::cout << "PASSED: " + p.path().filename().string() << std::endl;
                    } else {
                        std::cout << "FAILED: " + p.path().filename().string() << std::endl;
                        allPassed = false;
                    }
                }
            }
        }
        return allPassed;
    }

    static bool runDedicated(const std::string& path) {
        auto mainPath = getPath();
        auto compilationResult = RocCompiler::compile(mainPath + path);
        if (compilationResult == nullptr) {
            std::cout << "FAILED: " + path << std::endl;
            return false;
        } else {
            initMain(compilationResult);
            validateBox(compilationResult, path);
        }
    }

    static bool validateBox(RocCompilationResult* compilationResult, const std::string& path) {
        auto ref = (bool (*)()) compilationResult->EE->getFunctionAddress("box");
        if (ref()) {
            std::cout << "PASSED: " + path << std::endl;
            return true;
        } else {
            std::cout << "FAILED: " + path << std::endl;
            return false;
        }
    }
};

TEST_CASE("testRunner", "[testRunner]") {
    TestRunner testRunner;
    REQUIRE(testRunner.runAll(TestRunner::getPath()));
}

TEST_CASE("runDedicated", "[runDedicated]") {
    TestRunner testRunner;
    REQUIRE(testRunner.runDedicated("/ints/ints.roc"));
}

TEST_CASE("compileSdk", "[compileSdk]") {
    TestRunner testRunner;
    REQUIRE(testRunner.runAll(TestRunner::getSdkPath()));
}

#endif //ROCTESTS_TESTRUNNER_CPP
