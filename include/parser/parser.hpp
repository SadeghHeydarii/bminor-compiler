#ifndef BMINOR_PARSER_PARSER_HPP
#define BMINOR_PARSER_PARSER_HPP

#include "ast/ast.hpp"
#include "common/diagnostic.hpp"
#include "lexer/token.hpp"

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace bminor {

/// Result returned by parsing a B-Minor token stream.
struct ParseResult {
    std::unique_ptr<ProgramNode> program;
    std::vector<Diagnostic> diagnostics;

    bool hasErrors() const { return !diagnostics.empty(); }
};

/// LL(1)/LL(2) Recursive-Descent Parser for the B-Minor grammar with panic-mode recovery.
class Parser {
public:
    /// Constructs a parser from lexer output (typically LexResult::parserTokens).
    explicit Parser(const std::vector<Token>& tokens, std::string filename = "<input>");

    /// Parses the entire program into a ProgramNode AST and accumulates syntax diagnostics.
    ParseResult parse();

private:
    std::vector<Token> tokens_;
    std::string filename_;
    std::size_t current_ = 0;
    std::vector<Diagnostic> diagnostics_;
    bool panicMode_ = false;

    // Token inspection and consumption
    const Token& peek(std::size_t offset = 0) const;
    const Token& previous() const;
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& advance();

    // Error reporting and panic-mode synchronization
    void reportError(const SourceLocation& loc, const std::string& message);
    bool expect(TokenType expected, const std::string& expectedDesc);
    bool expectClosingParenBeforeBlockOrStmt();
    bool expectStmtEnd();
    void synchronizeDecl();
    void synchronizeStmt();
    void syncUntilClosingParenOrStmt();
    bool isStmtSyncPoint() const;
    static std::string describeGotToken(const Token& token);

    // Grammar rules: declarations & types
    std::unique_ptr<DeclNode> parseDeclaration();
    std::unique_ptr<FuncDecl> parseFunctionTail(const Token& nameTok);
    std::unique_ptr<VarDecl> parseVarTail(const Token& nameTok, bool isGlobal);
    std::vector<std::unique_ptr<ParamNode>> parseParamList();
    std::unique_ptr<ParamNode> parseParam();
    std::unique_ptr<TypeNode> parseReturnType();
    std::unique_ptr<TypeNode> parseType();
    std::unique_ptr<TypeNode> parseArrayType();
    std::unique_ptr<InitializerNode> parseInitializer();

    // Grammar rules: statements
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<StmtNode> parseStmt();
    std::unique_ptr<StmtNode> parseIfStmt();
    std::unique_ptr<StmtNode> parseForStmt();
    std::unique_ptr<StmtNode> parseReturnStmt();
    std::unique_ptr<StmtNode> parsePrintStmt();
    std::unique_ptr<StmtNode> parseLocalVarDeclStmt();
    std::unique_ptr<StmtNode> parseExprStmt();

    // Grammar rules: expressions
    std::vector<std::unique_ptr<ExprNode>> parseExprList();
    std::unique_ptr<ExprNode> parseExpr();
    std::unique_ptr<ExprNode> parseAssign();
    std::unique_ptr<ExprNode> parseLogicalOr();
    std::unique_ptr<ExprNode> parseLogicalAnd();
    std::unique_ptr<ExprNode> parseComparison();
    std::unique_ptr<ExprNode> parseAdditive();
    std::unique_ptr<ExprNode> parseMultiplicative();
    std::unique_ptr<ExprNode> parsePower();
    std::unique_ptr<ExprNode> parseUnary();
    std::unique_ptr<ExprNode> parsePostfix();
    std::unique_ptr<ExprNode> parsePrimary();

    template <typename NextLevelFn>
    std::unique_ptr<ExprNode> parseBinaryLeft(NextLevelFn nextLevel,
                                              std::initializer_list<TokenType> ops);

    // Literal helpers
    std::unique_ptr<ExprNode> buildIntLiteral(const Token& tok);
    std::unique_ptr<ExprNode> buildCharLiteral(const Token& tok);
    std::unique_ptr<ExprNode> buildStringLiteral(const Token& tok);
};

} // namespace bminor

#endif // BMINOR_PARSER_PARSER_HPP
