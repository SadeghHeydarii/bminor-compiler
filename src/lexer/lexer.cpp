#include "lexer/lexer.hpp"

#include <unordered_map>
#include <utility>

namespace bminor {
namespace {

bool isAsciiLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isAsciiDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isIdentStart(char c) {
    return isAsciiLetter(c) || c == '_';
}

bool isIdentContinue(char c) {
    return isIdentStart(c) || isAsciiDigit(c);
}

bool isWhitespaceChar(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

TokenType lookupKeywordOrIdent(const std::string& lexeme) {
    static const std::unordered_map<std::string, TokenType> kKeywords = {
        {"array", TokenType::KW_ARRAY},
        {"boolean", TokenType::KW_BOOLEAN},
        {"char", TokenType::KW_CHAR},
        {"else", TokenType::KW_ELSE},
        {"false", TokenType::BOOL_LITERAL},
        {"for", TokenType::KW_FOR},
        {"function", TokenType::KW_FUNCTION},
        {"if", TokenType::KW_IF},
        {"integer", TokenType::KW_INTEGER},
        {"print", TokenType::KW_PRINT},
        {"return", TokenType::KW_RETURN},
        {"string", TokenType::KW_STRING},
        {"true", TokenType::BOOL_LITERAL},
        {"void", TokenType::KW_VOID},
        {"while", TokenType::KW_WHILE}
    };

    auto it = kKeywords.find(lexeme);
    if (it != kKeywords.end()) {
        return it->second;
    }
    return TokenType::IDENT;
}

TokenType lookupDelimiter(char c) {
    switch (c) {
        case '(': return TokenType::LPAREN;
        case ')': return TokenType::RPAREN;
        case '{': return TokenType::LBRACE;
        case '}': return TokenType::RBRACE;
        case '[': return TokenType::LBRACKET;
        case ']': return TokenType::RBRACKET;
        case ':': return TokenType::COLON;
        case ';': return TokenType::SEMI;
        case ',': return TokenType::COMMA;
        default:  return TokenType::INVALID;
    }
}

} // namespace

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source)), filename_(std::move(filename)) {}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

char Lexer::peek(std::size_t offset) const {
    const std::size_t idx = pos_ + offset;
    if (idx >= source_.size()) {
        return '\0';
    }
    return source_[idx];
}

char Lexer::advance() {
    const char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

SourceLocation Lexer::currentLocation() const {
    return SourceLocation{filename_, line_, column_};
}

void Lexer::recordError(std::vector<Diagnostic>& diagnostics,
                        const SourceLocation& loc,
                        const std::string& message) {
    diagnostics.push_back(Diagnostic{DiagnosticStage::Lexical, loc, message});
}

LexResult Lexer::tokenize() {
    LexResult result;
    pos_ = 0;
    line_ = 1;
    column_ = 1;

    while (!isAtEnd()) {
        Token tok = scanNextToken(result.diagnostics);
        if (!isTrivia(tok.type)) {
            result.parserTokens.push_back(tok);
        }
        result.allTokens.push_back(std::move(tok));
    }

    Token eofTok{TokenType::END_OF_FILE, "", currentLocation(), pos_, 0};
    result.allTokens.push_back(eofTok);
    result.parserTokens.push_back(std::move(eofTok));
    return result;
}

Token Lexer::scanNextToken(std::vector<Diagnostic>& diagnostics) {
    const char c = peek();

    if (isWhitespaceChar(c)) {
        return scanWhitespace();
    }
    if (isIdentStart(c)) {
        return scanIdentifierOrKeyword(diagnostics);
    }
    if (isAsciiDigit(c)) {
        return scanInteger();
    }
    if (c == '"') {
        return scanString(diagnostics);
    }
    if (c == '\'') {
        return scanChar(diagnostics);
    }
    if (c == '/' && peek(1) == '/') {
        return scanLineComment();
    }
    if (c == '/' && peek(1) == '*') {
        return scanBlockComment(diagnostics);
    }
    return scanOperatorOrInvalid(diagnostics);
}

Token Lexer::scanWhitespace() {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    while (!isAtEnd() && isWhitespaceChar(peek())) {
        advance();
    }
    const std::size_t len = pos_ - startPos;
    return Token{TokenType::WHITESPACE, source_.substr(startPos, len), startLoc, startPos, len};
}

Token Lexer::scanIdentifierOrKeyword(std::vector<Diagnostic>& diagnostics) {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    while (!isAtEnd() && isIdentContinue(peek())) {
        advance();
    }
    const std::size_t len = pos_ - startPos;
    std::string lexeme = source_.substr(startPos, len);

    if (len > kMaxIdentifierLength) {
        recordError(diagnostics, startLoc, "Identifier exceeds maximum length of 256 characters");
        return Token{TokenType::INVALID, std::move(lexeme), startLoc, startPos, len};
    }

    const TokenType type = lookupKeywordOrIdent(lexeme);
    return Token{type, std::move(lexeme), startLoc, startPos, len};
}

Token Lexer::scanInteger() {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    while (!isAtEnd() && isAsciiDigit(peek())) {
        advance();
    }
    const std::size_t len = pos_ - startPos;
    return Token{TokenType::INT_LITERAL, source_.substr(startPos, len), startLoc, startPos, len};
}

Token Lexer::scanString(std::vector<Diagnostic>& diagnostics) {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    advance(); // consume opening '"'

    std::size_t decodedLen = 0;
    bool terminated = false;

    while (!isAtEnd()) {
        const char c = peek();
        if (c == '\n' || c == '\r') {
            break;
        }
        if (c == '"') {
            advance();
            terminated = true;
            break;
        }
        if (c == '\\') {
            advance(); // consume '\'
            if (isAtEnd() || peek() == '\n' || peek() == '\r') {
                break;
            }
            advance(); // consume escaped char
            ++decodedLen;
            continue;
        }
        advance();
        ++decodedLen;
    }

    const std::size_t len = pos_ - startPos;
    std::string lexeme = source_.substr(startPos, len);

    if (!terminated) {
        recordError(diagnostics, startLoc, "Unterminated string literal");
        return Token{TokenType::INVALID, std::move(lexeme), startLoc, startPos, len};
    }
    if (decodedLen > kMaxStringLength) {
        recordError(diagnostics, startLoc, "String literal exceeds maximum length of 256 characters");
        return Token{TokenType::INVALID, std::move(lexeme), startLoc, startPos, len};
    }
    return Token{TokenType::STRING_LITERAL, std::move(lexeme), startLoc, startPos, len};
}

Token Lexer::scanChar(std::vector<Diagnostic>& diagnostics) {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    advance(); // consume opening '\''

    if (isAtEnd() || peek() == '\n' || peek() == '\r') {
        const std::size_t len = pos_ - startPos;
        recordError(diagnostics, startLoc, "Unterminated character literal");
        return Token{TokenType::INVALID, source_.substr(startPos, len), startLoc, startPos, len};
    }

    if (peek() == '\'') {
        advance();
        const std::size_t len = pos_ - startPos;
        recordError(diagnostics, startLoc, "Empty character literal");
        return Token{TokenType::INVALID, source_.substr(startPos, len), startLoc, startPos, len};
    }

    // Consume one logical character (either escaped or single ASCII byte).
    if (peek() == '\\') {
        advance();
        if (isAtEnd() || peek() == '\n' || peek() == '\r') {
            const std::size_t len = pos_ - startPos;
            recordError(diagnostics, startLoc, "Unterminated character literal");
            return Token{TokenType::INVALID, source_.substr(startPos, len), startLoc, startPos, len};
        }
        advance();
    } else {
        advance();
    }

    if (!isAtEnd() && peek() == '\'') {
        advance();
        const std::size_t len = pos_ - startPos;
        return Token{TokenType::CHAR_LITERAL, source_.substr(startPos, len), startLoc, startPos, len};
    }

    // Recover across remaining characters on the same line until closing quote or newline.
    bool closed = false;
    while (!isAtEnd() && peek() != '\n' && peek() != '\r') {
        if (peek() == '\'') {
            advance();
            closed = true;
            break;
        }
        if (peek() == '\\' && peek(1) != '\0' && peek(1) != '\n' && peek(1) != '\r') {
            advance();
        }
        advance();
    }

    const std::size_t len = pos_ - startPos;
    recordError(diagnostics,
                startLoc,
                closed ? "Character literal must contain a single character"
                       : "Unterminated character literal");
    return Token{TokenType::INVALID, source_.substr(startPos, len), startLoc, startPos, len};
}

Token Lexer::scanLineComment() {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    advance(); // '/'
    advance(); // '/'
    while (!isAtEnd() && peek() != '\n' && peek() != '\r') {
        advance();
    }
    const std::size_t len = pos_ - startPos;
    return Token{TokenType::COMMENT_LINE, source_.substr(startPos, len), startLoc, startPos, len};
}

Token Lexer::scanBlockComment(std::vector<Diagnostic>& diagnostics) {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    advance(); // '/'
    advance(); // '*'

    bool closed = false;
    while (!isAtEnd()) {
        if (peek() == '*' && peek(1) == '/') {
            advance();
            advance();
            closed = true;
            break;
        }
        advance();
    }

    const std::size_t len = pos_ - startPos;
    std::string lexeme = source_.substr(startPos, len);
    if (!closed) {
        recordError(diagnostics, startLoc, "Unterminated block comment");
        return Token{TokenType::INVALID, std::move(lexeme), startLoc, startPos, len};
    }
    return Token{TokenType::COMMENT_BLOCK, std::move(lexeme), startLoc, startPos, len};
}

Token Lexer::scanOperatorOrInvalid(std::vector<Diagnostic>& diagnostics) {
    const std::size_t startPos = pos_;
    const SourceLocation startLoc = currentLocation();
    const char c = advance();

    const TokenType delim = lookupDelimiter(c);
    if (delim != TokenType::INVALID) {
        return Token{delim, std::string(1, c), startLoc, startPos, 1};
    }

    auto makeTwoChar = [&](TokenType type) {
        advance();
        return Token{type, source_.substr(startPos, 2), startLoc, startPos, 2};
    };

    switch (c) {
        case '+':
            return (peek() == '+') ? makeTwoChar(TokenType::OP_INC)
                                   : Token{TokenType::OP_PLUS, "+", startLoc, startPos, 1};
        case '-':
            return (peek() == '-') ? makeTwoChar(TokenType::OP_DEC)
                                   : Token{TokenType::OP_MINUS, "-", startLoc, startPos, 1};
        case '*':
            return Token{TokenType::OP_STAR, "*", startLoc, startPos, 1};
        case '/':
            return Token{TokenType::OP_SLASH, "/", startLoc, startPos, 1};
        case '%':
            return Token{TokenType::OP_PERCENT, "%", startLoc, startPos, 1};
        case '^':
            return Token{TokenType::OP_CARET, "^", startLoc, startPos, 1};
        case '<':
            return (peek() == '=') ? makeTwoChar(TokenType::OP_LE)
                                   : Token{TokenType::OP_LT, "<", startLoc, startPos, 1};
        case '>':
            return (peek() == '=') ? makeTwoChar(TokenType::OP_GE)
                                   : Token{TokenType::OP_GT, ">", startLoc, startPos, 1};
        case '=':
            return (peek() == '=') ? makeTwoChar(TokenType::OP_EQ)
                                   : Token{TokenType::OP_ASSIGN, "=", startLoc, startPos, 1};
        case '!':
            return (peek() == '=') ? makeTwoChar(TokenType::OP_NEQ)
                                   : Token{TokenType::OP_NOT, "!", startLoc, startPos, 1};
        case '&':
            if (peek() == '&') {
                return makeTwoChar(TokenType::OP_AND);
            }
            recordError(diagnostics, startLoc, "Unexpected character '&' (did you mean '&&'?)");
            return Token{TokenType::INVALID, "&", startLoc, startPos, 1};
        case '|':
            if (peek() == '|') {
                return makeTwoChar(TokenType::OP_OR);
            }
            recordError(diagnostics, startLoc, "Unexpected character '|' (did you mean '||'?)");
            return Token{TokenType::INVALID, "|", startLoc, startPos, 1};
        default:
            recordError(diagnostics, startLoc, std::string("Unexpected character '") + c + "'");
            return Token{TokenType::INVALID, std::string(1, c), startLoc, startPos, 1};
    }
}

} // namespace bminor
