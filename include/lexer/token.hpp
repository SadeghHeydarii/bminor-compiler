#ifndef BMINOR_LEXER_TOKEN_HPP
#define BMINOR_LEXER_TOKEN_HPP

#include "common/source_location.hpp"

#include <cstddef>
#include <string>

namespace bminor {

/// All token types defined in the B-Minor lexical specification.
enum class TokenType {
    // Identifiers & Literals
    IDENT,
    INT_LITERAL,
    BOOL_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,

    // Keywords (excluding boolean literals true/false which map to BOOL_LITERAL)
    KW_ARRAY,
    KW_BOOLEAN,
    KW_CHAR,
    KW_ELSE,
    KW_FOR,
    KW_FUNCTION,
    KW_IF,
    KW_INTEGER,
    KW_PRINT,
    KW_RETURN,
    KW_STRING,
    KW_VOID,
    KW_WHILE,

    // Operators
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_PERCENT,
    OP_CARET,
    OP_INC,
    OP_DEC,
    OP_NOT,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_EQ,
    OP_NEQ,
    OP_AND,
    OP_OR,
    OP_ASSIGN,

    // Delimiters
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COLON,
    SEMI,
    COMMA,

    // Trivia (preserved in allTokens for exact highlighting/reconstruction)
    COMMENT_LINE,
    COMMENT_BLOCK,
    WHITESPACE,

    // Special
    INVALID,
    END_OF_FILE
};

/// Single lexical token with exact matched lexeme, source position, and byte span.
struct Token {
    TokenType type = TokenType::INVALID;
    std::string lexeme;
    SourceLocation location;
    std::size_t startOffset = 0;
    std::size_t length = 0;
};

/// Returns true if the token type is a B-Minor keyword (structural or type keyword).
bool isKeyword(TokenType type);

/// Returns true if the token type is an atomic or array type keyword (integer, boolean, char, string).
bool isTypeKeyword(TokenType type);

/// Returns true if the token type is whitespace or a comment.
bool isTrivia(TokenType type);

/// Returns a concise readable token label matching the specification style (e.g., "KW(if)", "OP(<=)", "EOF").
std::string formatToken(const Token& token);

} // namespace bminor

#endif // BMINOR_LEXER_TOKEN_HPP
