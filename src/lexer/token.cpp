#include "lexer/token.hpp"

#include <unordered_map>

namespace bminor {

bool isKeyword(TokenType type) {
    switch (type) {
        case TokenType::KW_ARRAY:
        case TokenType::KW_BOOLEAN:
        case TokenType::KW_CHAR:
        case TokenType::KW_ELSE:
        case TokenType::KW_FOR:
        case TokenType::KW_FUNCTION:
        case TokenType::KW_IF:
        case TokenType::KW_INTEGER:
        case TokenType::KW_PRINT:
        case TokenType::KW_RETURN:
        case TokenType::KW_STRING:
        case TokenType::KW_VOID:
        case TokenType::KW_WHILE:
            return true;
        default:
            return false;
    }
}

bool isTypeKeyword(TokenType type) {
    switch (type) {
        case TokenType::KW_BOOLEAN:
        case TokenType::KW_CHAR:
        case TokenType::KW_INTEGER:
        case TokenType::KW_STRING:
            return true;
        default:
            return false;
    }
}

bool isTrivia(TokenType type) {
    return type == TokenType::WHITESPACE ||
           type == TokenType::COMMENT_LINE ||
           type == TokenType::COMMENT_BLOCK;
}

std::string formatToken(const Token& token) {
    if (isKeyword(token.type)) {
        return "KW(" + token.lexeme + ")";
    }

    switch (token.type) {
        case TokenType::IDENT:
            return "IDENT(" + token.lexeme + ")";
        case TokenType::INT_LITERAL:
            return "INT(" + token.lexeme + ")";
        case TokenType::BOOL_LITERAL:
            return "BOOL(" + token.lexeme + ")";
        case TokenType::CHAR_LITERAL:
            return "CHAR(" + token.lexeme + ")";
        case TokenType::STRING_LITERAL:
            return "STRING(" + token.lexeme + ")";
        case TokenType::OP_PLUS:
        case TokenType::OP_MINUS:
        case TokenType::OP_STAR:
        case TokenType::OP_SLASH:
        case TokenType::OP_PERCENT:
        case TokenType::OP_CARET:
        case TokenType::OP_INC:
        case TokenType::OP_DEC:
        case TokenType::OP_NOT:
        case TokenType::OP_LT:
        case TokenType::OP_LE:
        case TokenType::OP_GT:
        case TokenType::OP_GE:
        case TokenType::OP_EQ:
        case TokenType::OP_NEQ:
        case TokenType::OP_AND:
        case TokenType::OP_OR:
            return "OP(" + token.lexeme + ")";
        case TokenType::OP_ASSIGN:
            return "ASSIGN";
        case TokenType::LPAREN:
            return "LPAREN";
        case TokenType::RPAREN:
            return "RPAREN";
        case TokenType::LBRACE:
            return "LBRACE";
        case TokenType::RBRACE:
            return "RBRACE";
        case TokenType::LBRACKET:
            return "LBRACKET";
        case TokenType::RBRACKET:
            return "RBRACKET";
        case TokenType::COLON:
            return "COLON";
        case TokenType::SEMI:
            return "SEMI";
        case TokenType::COMMA:
            return "COMMA";
        case TokenType::COMMENT_LINE:
            return "COMMENT_LINE";
        case TokenType::COMMENT_BLOCK:
            return "COMMENT_BLOCK";
        case TokenType::WHITESPACE:
            return "WHITESPACE";
        case TokenType::INVALID:
            return "INVALID('" + token.lexeme + "')";
        case TokenType::END_OF_FILE:
            return "EOF";
        default:
            return "UNKNOWN";
    }
}

} // namespace bminor
