#ifndef BMINOR_HIGHLIGHTER_HIGHLIGHTER_HPP
#define BMINOR_HIGHLIGHTER_HIGHLIGHTER_HPP

#include "ast/ast.hpp"
#include "common/diagnostic.hpp"
#include "common/source_location.hpp"
#include "lexer/token.hpp"

#include <string>
#include <vector>

namespace bminor {

/// Semantic highlighting categories matching Table 1 of the B-Minor specification.
enum class HighlightCategory {
    None,
    Keyword,
    TypeName,
    VarIdentifier,
    FuncIdentifier,
    IntLiteral,
    BoolLiteral,
    StringOrCharLiteral,
    Operator,
    Comment,
    Error
};

/// Source token paired with its AST-resolved highlighting category.
struct HighlightedToken {
    std::string lexeme;
    HighlightCategory category = HighlightCategory::None;
    TokenType tokenType = TokenType::INVALID;
    SourceLocation location;
};

/// Context-sensitive AST syntax highlighter.
/// Traverses the AST to distinguish function declarations/calls from variable identifiers
/// while preserving all source tokens (whitespace, comments, and invalid tokens).
class SyntaxHighlighter {
public:
    std::vector<HighlightedToken> highlight(const std::vector<Token>& allTokens,
                                            const ProgramNode* program,
                                            const std::vector<Diagnostic>& diagnostics = {}) const;
};

} // namespace bminor

#endif // BMINOR_HIGHLIGHTER_HIGHLIGHTER_HPP
