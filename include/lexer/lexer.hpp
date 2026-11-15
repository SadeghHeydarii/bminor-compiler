#ifndef BMINOR_LEXER_LEXER_HPP
#define BMINOR_LEXER_LEXER_HPP

#include "common/diagnostic.hpp"
#include "lexer/token.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace bminor {

/// Result of lexing a source buffer, preserving both full source tokens and parser-ready tokens.
struct LexResult {
    std::vector<Token> allTokens;
    std::vector<Token> parserTokens;
    std::vector<Diagnostic> diagnostics;
};

/// Manual-DFA lexical analyzer for the B-Minor programming language.
class Lexer {
public:
    static constexpr std::size_t kMaxIdentifierLength = 256;
    static constexpr std::size_t kMaxStringLength = 256;

    Lexer(std::string source, std::string filename = "<input>");

    /// Scans the entire source input, recovering from all lexical errors without aborting.
    LexResult tokenize();

private:
    std::string source_;
    std::string filename_;
    std::size_t pos_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;

    bool isAtEnd() const;
    char peek(std::size_t offset = 0) const;
    char advance();
    SourceLocation currentLocation() const;

    Token scanNextToken(std::vector<Diagnostic>& diagnostics);
    Token scanWhitespace();
    Token scanIdentifierOrKeyword(std::vector<Diagnostic>& diagnostics);
    Token scanInteger();
    Token scanString(std::vector<Diagnostic>& diagnostics);
    Token scanChar(std::vector<Diagnostic>& diagnostics);
    Token scanLineComment();
    Token scanBlockComment(std::vector<Diagnostic>& diagnostics);
    Token scanOperatorOrInvalid(std::vector<Diagnostic>& diagnostics);

    static void recordError(std::vector<Diagnostic>& diagnostics,
                            const SourceLocation& loc,
                            const std::string& message);
};

} // namespace bminor

#endif // BMINOR_LEXER_LEXER_HPP
