//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#pragma once
#ifndef ROC_LANG_LEXER_H
#define ROC_LANG_LEXER_H

#include "Token.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

class Lexer {
private:
    Token* getNextToken();
public:
	int pos = 0;
	int offset = 0;
	char current = 0;
	bool peeked = false;
	Token* peekedToken = nullptr;
	std::string content;
    std::string filePath;
	Token* currentToken = nullptr;
    Eof* eofT = new Eof(0);
    Space* spaceT = new Space(0);

    explicit Lexer(const std::string& filePath);

	Lexer(std::string content, std::string filePath);

	~Lexer() {
        //delete eofT;
        //delete spaceT;
        //delete newLineT;
	}

    Token* nextTokenWithWS();

	Token* nextToken();

    Token* nextTokenSkipNL();

	char nextChar();

	bool hasNext() const;

	Token* peekNext();

	Token* checkPeeked();
};

#endif
