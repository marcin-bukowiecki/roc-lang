//
// Created by Marcin Bukowiecki on 2019-09-19.
//
#include "Lexer.h"
#include <string>
#include <utility>
#include <fstream>

inline Literal* isNumber(int offset, const std::string& text);

inline void checkContent(Lexer* lexer) {
    if (lexer->content.empty()) {
        lexer->current = -1;
    } else {
        lexer->current = lexer->content.at(0);
    }
}

Lexer::Lexer(const std::string& filePath) {
    std::string line;
    std::ifstream scorchFile(filePath);
    std::string acc;

    if (scorchFile.is_open())
    {
        while (getline(scorchFile, line))
        {
            if (!acc.empty()) acc+='\n';
            acc+=line;
        }
        scorchFile.close();
    }

    this->content = acc;
    this->filePath = filePath;
    checkContent(this);
}

Lexer::Lexer(std::string content, std::string filePath) {
    this->content = std::move(content);
    this->filePath = std::move(filePath);
    checkContent(this);
}

char Lexer::nextChar() {
    if ((this->pos + 1) >= this->content.size()) {
        this->current = -1;
        return -1;
    }
	this->offset++;
	char ch = this->content.at(++this->pos);
	this->current = ch;
	return ch;
}

static Token* resolveToken(int offset, const std::string& acc, bool isDigit) {
	if (acc == "fun") {
		return new FunKeyword(offset);

	} else if (acc == "for") {
        return new ForKeyword(offset);

    } else if (acc == "met") {
        return new MetKeyword(offset);

    } else if (acc == "while") {
        return new WhileKeyword(offset);

    } else if (acc == "ret") {
		return new RetKeyword(offset);

    } else if (acc == "var") {
        return new VarKeyword(offset);

    } else if (acc == "val") {
        return new ValKeyword(offset);

    } else if (acc == "in") {
        return new InKeyword(offset);

    } else if (acc == "is") {
        return new IsKeyword(offset);

    } else if (acc == "break") {
        return new BreakKeyword(offset);

    } else if (acc == "continue") {
        return new ContinueKeyword(offset);

	} else if (acc == "if") {
        return new IfKeyword(offset);

    } else if (acc == "else") {
        return new ElseKeyword(offset);

	} else if (acc == "lam") {
        return new LamKeyword(offset);

    } else if (acc == "or") {
        return new OrKeyword(offset);

    } else if (acc == "and") {
        return new AndKeyword(offset);

    } else if (acc == "import") {
        return new ImportKeyword(offset);

    } else if (acc == "true") {
        return new TrueKeyword(offset);

    } else if (acc == "false") {
        return new FalseKeyword(offset);

    } else if (acc == "struct") {
        return new StructKeyword(offset);

    } else if (acc == "trait") {
        return new TraitKeyword(offset);

    } else if (acc == "Null") {
        return new NullToken(offset);

    } else if (acc == "varargs") {
        return new VarargsKeyword(offset);

    } else if (acc == "package") {
        return new PackageKeyword(offset);

    } else {
	    if (isDigit) {
            return isNumber(offset, acc);
	    } else {
            return new Literal(offset, acc);
	    }
	}
}

Token* Lexer::peekNext() {
    if (peeked) {
        return this->peekedToken;
    }
    auto next = getNextToken();
    while (next->isWhitespace()) {
        next = getNextToken();
    }
    this->peekedToken = next;
    this->peeked = true;
    return this->peekedToken;
}

Token* Lexer::getNextToken() {
    if (peeked) {
        return checkPeeked();
    }
	std::string acc;
	bool isDigit = false;
	Token* token;
	char next;
	int startOffset = this->offset;

	while (true) {
		switch (this->current)
		{
		    case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (acc.empty() && !isDigit) {
                    isDigit = true;
                }
                acc += this->current;
                break;
            case 0:
                nextChar();
                continue;
            case -1:
                if (!acc.empty()) {
                    token = resolveToken(startOffset, acc, isDigit);
                    goto loopEnd;
                }
                eofT->setStartOffset(startOffset);
                token = eofT;
                goto loopEnd;
            case ' ':
                if (acc.empty()) {
                    token = spaceT;
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '!':
                if (acc.empty()) {
                    next = nextChar();
                    if (next == '=') {
                        token = new NotEqualOp(startOffset);
                        nextChar();
                    } else {
                        token = new NotOp(startOffset);
                    }
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '<':
                if (acc.empty()) {
                    next = nextChar();
                    if (next == '=') {
                        token = new LesserOrEqual(startOffset);
                        nextChar();
                    } else {
                        token = new Lesser(startOffset);
                    }
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '>':
                if (acc.empty()) {
                    next = nextChar();
                    if (next == '=') {
                        token = new GreaterOrEqual(startOffset);
                        nextChar();
                    } else {
                        token = new Greater(startOffset);
                    }
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '-':
                if (acc.empty()) {
                    next = nextChar();
                    if (next == '>') {
                        token = new Arrow(startOffset);
                        nextChar();
                    }
                    else {
                        token = new Sub(startOffset);
                    }
                }
                else {
                    if (isDigit && acc.at(acc.size()-1) == 'e') {
                        acc += this->current;
                        break;
                    } else {
                        token = resolveToken(startOffset, acc, isDigit);
                    }
                }
                goto loopEnd;
            case '\n':
                if (acc.empty()) {
                    token = new NewLine(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '.':
                if (acc.empty()) {
                    token = new Dot(startOffset);
                    nextChar();
                } else {
                    if (isDigit) {
                        acc += this->current;
                        break;
                    }
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case ';':
                if (acc.empty()) {
                    token = new Semicolon(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '{':
                if (acc.empty()) {
                    token = new LeftCurl(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '}':
                if (acc.empty()) {
                    token = new RightCurl(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '=':
                if (acc.empty()) {
                    next = nextChar();
                    if (next == '=') {
                        token = new EqualOp(startOffset);
                        nextChar();
                    } else {
                        token = new AssignOp(startOffset);
                    }
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '(':
                if (acc.empty()) {
                    token = new LeftParenthesis(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case ')':
                if (acc.empty()) {
                    token = new RightParenthesis(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '[':
                if (acc.empty()) {
                    token = new LeftBracket(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case ']':
                if (acc.empty()) {
                    token = new RightBracket(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case ',':
                if (acc.empty()) {
                    token = new Comma(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case ':':
                if (acc.empty()) {
                    token = new Colon(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '"':
                if (acc.empty()) {
                    token = new QuotionMark(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '/':
                if (acc.empty()) {
                    token = new Div(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '\\':
                if (acc.empty()) {
                    token = new InverseDiv(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '*':
                if (acc.empty()) {
                    token = new Mul(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '%':
                if (acc.empty()) {
                    token = new Mod(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '^':
                if (acc.empty()) {
                    token = new Pow(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            case '+':
                if (acc.empty()) {
                    token = new Add(startOffset);
                    nextChar();
                }
                else {
                    token = resolveToken(startOffset, acc, isDigit);
                }
                goto loopEnd;
            default:
                acc += this->current;
            }
        nextChar();
	}

loopEnd:
	return token;
}

Token* Lexer::checkPeeked() {
    auto tmp = this->peekedToken;
    this->peeked = false;
    this->peekedToken = nullptr;
    this->currentToken = tmp;
    return tmp;
}

Token* Lexer::nextTokenWithWS() {
    if (peeked) {
        return checkPeeked();
    }
    auto t = getNextToken();
    this->currentToken = t;
    return t;
}

Token* Lexer::nextToken() {
    if (peeked) {
        return checkPeeked();
    }
    auto t = getNextToken();
    while (t->isWhitespace()) {
        t = getNextToken();
    }
    this->currentToken = t;
    return t;
}

Token* Lexer::nextTokenSkipNL() {
    auto t = nextToken();
    while (t->isNewLine()) {
        delete t;
        t = getNextToken();
    }
    return t;
}

bool Lexer::hasNext() const {
	switch (this->current)
	{
	case -1: //end of file
		return false;
	default:
		return true;
	}
}

class unsupportedNumberFormat: public std::exception
{
public:
    std::string message;

    explicit unsupportedNumberFormat(const std::string& text) {
        this->message = "Unsupported number format: " + text;
    }

    const char* what() const noexcept override
    {
        return message.c_str();
    }
};

inline Literal* isNumber(int offset, const std::string& text) {
    size_t size = text.size();
    bool allNumbers = true;
    char ch;
    bool hadDot = false;

    for (size_t i = 0; i < size; i++) {
        ch = text.at(i);

        if (ch == 'e') {
            if (i == (text.size() - 1)) {
                throw unsupportedNumberFormat(text);
            }
            i++; //skip -
            continue;
        }

        if (!std::isalnum(ch)) {
            if (i == (size - 1)) {
                if (ch == 'd' || ch == '.') {
                    return new DoubleNumber(offset, text, std::stod(text));
                } else {
                    throw unsupportedNumberFormat(text);
                }
            }
            else {
                if (ch == '.') {
                    hadDot = true;
                    continue;
                }
                throw unsupportedNumberFormat(text);
            }
        } else {
            if (!std::isdigit(ch)) {
                allNumbers = false;
                break;
            }
        }
    }

    if (allNumbers) {
        if (hadDot) {
            return new DoubleNumber(offset, text, std::stod(text));
        }
        //TODO float int etc.
        return new IntNumber(offset, text, strtol(text.c_str(), nullptr, 0));
    }

    throw unsupportedNumberFormat(text);
}
