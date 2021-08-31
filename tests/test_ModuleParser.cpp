//
// Created by Marcin Bukowiecki on 21.03.2021.
//
#define CATCH_CONFIG_MAIN
#include "Catch.h"
#include "../parser/Parser.h"
#include <memory>
#include "ExprHandler.h"

#ifdef SANDBOX_DIR
    #define SANDBOX_DIR_PATH SANDBOX_DIR
#else
    #define SANDBOX_DIR_PATH ""
#endif

void printExceptions(ModuleParser *moduleParser) {
    for (const auto &item : moduleParser->syntaxExceptions) {
        item.printMessage();
    }
}

TEST_CASE("println", "[println]") {
    std::string path(SANDBOX_DIR_PATH);
    Lexer lexer(path + "/parser/Module1.roc");
    auto pc = std::make_unique<ParseContext>(&lexer);
    ModuleParser moduleParser(pc.get());
    moduleParser.parse();
    if (!moduleParser.syntaxExceptions.empty()) {
        printExceptions(&moduleParser);
        FAIL("Exceptions during parsing");
    }
    REQUIRE("println(\"Hello world\")\n" == pc->moduleDeclarations.back().get()->getText());
}

TEST_CASE("println;", "[println;]") {
    std::string path(SANDBOX_DIR_PATH);
    Lexer lexer(path + "/parser/Module2.roc");
    auto pc = std::make_unique<ParseContext>(&lexer);
    ModuleParser moduleParser(pc.get());
    moduleParser.parse();
    if (!moduleParser.syntaxExceptions.empty()) {
        printExceptions(&moduleParser);
        FAIL("Exceptions during parsing");
    }
    REQUIRE("println(\"Hello world\")\n" == pc->moduleDeclarations.back().get()->getText());
}

TEST_CASE("println fun", "[printlnFun]") {
    std::string path(SANDBOX_DIR_PATH);
    Lexer lexer(path + "/parser/Module3.roc");
    auto pc = std::make_unique<ParseContext>(&lexer);
    ModuleParser moduleParser(pc.get());
    moduleParser.parse();
    if (!moduleParser.syntaxExceptions.empty()) {
        printExceptions(&moduleParser);
        FAIL("Exceptions during parsing");
    }
    REQUIRE(moduleParser.syntaxExceptions.empty());
}

TEST_CASE("println fun with exception 1", "[printlnFunWithException1]") {
    std::string path(SANDBOX_DIR_PATH);
    Lexer lexer(path + "/parser/Module4.roc");
    auto pc = std::make_unique<ParseContext>(&lexer);
    ModuleParser moduleParser(pc.get());
    moduleParser.parse();
    REQUIRE(!moduleParser.syntaxExceptions.empty());
}

TEST_CASE("println fun with exception 2", "[printlnFunWithException2]") {
    std::string path(SANDBOX_DIR_PATH);
    Lexer lexer(path + "/parser/Module5.roc");
    auto pc = std::make_unique<ParseContext>(&lexer);
    ModuleParser moduleParser(pc.get());
    moduleParser.parse();
    REQUIRE(!moduleParser.syntaxExceptions.empty());
}

TEST_CASE("expected arrow 1", "[expectedArrow1]") {
    std::string path(SANDBOX_DIR_PATH);
    Lexer lexer(path + "/parser/Module7.roc");
    auto pc = std::make_unique<ParseContext>(&lexer);
    ModuleParser moduleParser(pc.get());
    moduleParser.parse();
    REQUIRE(!moduleParser.syntaxExceptions.empty());
}