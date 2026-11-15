#include "highlighter/highlighter.hpp"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace bminor {

namespace {

std::uint64_t encodeLocationKey(const SourceLocation& loc) {
    return (static_cast<std::uint64_t>(loc.line) << 32) |
           static_cast<std::uint64_t>(loc.column & 0xFFFFFFFFu);
}

bool isDelimiterToken(TokenType type) {
    switch (type) {
        case TokenType::LPAREN:
        case TokenType::RPAREN:
        case TokenType::LBRACE:
        case TokenType::RBRACE:
        case TokenType::LBRACKET:
        case TokenType::RBRACKET:
        case TokenType::COLON:
        case TokenType::SEMI:
        case TokenType::COMMA:
            return true;
        default:
            return false;
    }
}

/// Traverses the AST to classify tokens according to their syntactic/semantic role.
class AstHighlightVisitor final : public ASTVisitor {
public:
    std::unordered_map<std::uint64_t, HighlightCategory> roleByLocation;

    void visit(const ProgramNode& node) override {
        for (const auto& decl : node.declarations) {
            visitIfNonNull(decl.get());
        }
    }

    void visit(const AtomicTypeNode& node) override {
        mark(node.location, HighlightCategory::TypeName);
    }

    void visit(const VoidTypeNode& node) override {
        mark(node.location, HighlightCategory::TypeName);
    }

    void visit(const ArrayTypeNode& node) override {
        mark(node.location, HighlightCategory::Keyword);
        visitIfNonNull(node.size.get());
        visitIfNonNull(node.elementType.get());
    }

    void visit(const ExprInitializer& node) override {
        visitIfNonNull(node.expression.get());
    }

    void visit(const ArrayInitializer& node) override {
        for (const auto& elem : node.elements) {
            visitIfNonNull(elem.get());
        }
    }

    void visit(const IntLiteralExpr& node) override {
        mark(node.location, HighlightCategory::IntLiteral);
    }

    void visit(const BoolLiteralExpr& node) override {
        mark(node.location, HighlightCategory::BoolLiteral);
    }

    void visit(const CharLiteralExpr& node) override {
        mark(node.location, HighlightCategory::StringOrCharLiteral);
    }

    void visit(const StringLiteralExpr& node) override {
        mark(node.location, HighlightCategory::StringOrCharLiteral);
    }

    void visit(const IdentifierExpr& node) override {
        mark(node.location, HighlightCategory::VarIdentifier);
    }

    void visit(const AssignExpr& node) override {
        visitIfNonNull(node.target.get());
        visitIfNonNull(node.value.get());
    }

    void visit(const BinaryExpr& node) override {
        visitIfNonNull(node.left.get());
        visitIfNonNull(node.right.get());
    }

    void visit(const UnaryExpr& node) override {
        visitIfNonNull(node.operand.get());
    }

    void visit(const PostfixExpr& node) override {
        visitIfNonNull(node.operand.get());
    }

    void visit(const CallExpr& node) override {
        if (const auto* calleeIdent = dynamic_cast<const IdentifierExpr*>(node.callee.get())) {
            mark(calleeIdent->location, HighlightCategory::FuncIdentifier);
        } else {
            visitIfNonNull(node.callee.get());
        }
        for (const auto& arg : node.arguments) {
            visitIfNonNull(arg.get());
        }
    }

    void visit(const ArrayIndexExpr& node) override {
        visitIfNonNull(node.array.get());
        visitIfNonNull(node.index.get());
    }

    void visit(const ErrorExpr&) override {}

    void visit(const ParamNode& node) override {
        mark(node.nameLocation, HighlightCategory::VarIdentifier);
        visitIfNonNull(node.type.get());
    }

    void visit(const VarDecl& node) override {
        mark(node.nameLocation, HighlightCategory::VarIdentifier);
        visitIfNonNull(node.type.get());
        visitIfNonNull(node.initializer.get());
    }

    void visit(const FuncDecl& node) override {
        mark(node.nameLocation, HighlightCategory::FuncIdentifier);
        visitIfNonNull(node.returnType.get());
        for (const auto& param : node.params) {
            visitIfNonNull(param.get());
        }
        visitIfNonNull(node.body.get());
    }

    void visit(const BlockStmt& node) override {
        for (const auto& stmt : node.statements) {
            visitIfNonNull(stmt.get());
        }
    }

    void visit(const IfStmt& node) override {
        mark(node.location, HighlightCategory::Keyword);
        visitIfNonNull(node.condition.get());
        visitIfNonNull(node.thenBranch.get());
        visitIfNonNull(node.elseBranch.get());
    }

    void visit(const ForStmt& node) override {
        mark(node.location, HighlightCategory::Keyword);
        visitIfNonNull(node.init.get());
        visitIfNonNull(node.condition.get());
        visitIfNonNull(node.update.get());
        visitIfNonNull(node.body.get());
    }

    void visit(const ReturnStmt& node) override {
        mark(node.location, HighlightCategory::Keyword);
        visitIfNonNull(node.value.get());
    }

    void visit(const PrintStmt& node) override {
        mark(node.location, HighlightCategory::Keyword);
        for (const auto& expr : node.expressions) {
            visitIfNonNull(expr.get());
        }
    }

    void visit(const VarDeclStmt& node) override {
        visitIfNonNull(node.decl.get());
    }

    void visit(const ExprStmt& node) override {
        visitIfNonNull(node.expression.get());
    }

    void visit(const ErrorStmt&) override {}

private:
    void mark(const SourceLocation& loc, HighlightCategory category) {
        roleByLocation[encodeLocationKey(loc)] = category;
    }

    void visitIfNonNull(const ASTNode* node) {
        if (node != nullptr) {
            node->accept(*this);
        }
    }
};

HighlightCategory classifyLexicalFallback(const Token& token) {
    if (token.type == TokenType::WHITESPACE || isDelimiterToken(token.type)) {
        return HighlightCategory::None;
    }
    if (token.type == TokenType::INVALID) {
        return HighlightCategory::Error;
    }
    if (token.type == TokenType::COMMENT_LINE || token.type == TokenType::COMMENT_BLOCK) {
        return HighlightCategory::Comment;
    }
    if (isTypeKeyword(token.type) || token.type == TokenType::KW_VOID) {
        return HighlightCategory::TypeName;
    }
    if (isKeyword(token.type)) {
        return HighlightCategory::Keyword;
    }

    switch (token.type) {
        case TokenType::INT_LITERAL:
            return HighlightCategory::IntLiteral;
        case TokenType::BOOL_LITERAL:
            return HighlightCategory::BoolLiteral;
        case TokenType::CHAR_LITERAL:
        case TokenType::STRING_LITERAL:
            return HighlightCategory::StringOrCharLiteral;
        case TokenType::IDENT:
            return HighlightCategory::VarIdentifier;
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
        case TokenType::OP_ASSIGN:
            return HighlightCategory::Operator;
        default:
            return HighlightCategory::None;
    }
}

} // namespace

std::vector<HighlightedToken> SyntaxHighlighter::highlight(
    const std::vector<Token>& allTokens,
    const ProgramNode* program,
    const std::vector<Diagnostic>& diagnostics) const {
    AstHighlightVisitor astVisitor;
    if (program != nullptr) {
        program->accept(astVisitor);
    }

    std::unordered_set<std::uint64_t> errorLocations;
    for (const auto& diag : diagnostics) {
        errorLocations.insert(encodeLocationKey(diag.location));
    }

    std::vector<HighlightedToken> output;
    output.reserve(allTokens.size());

    for (const auto& tok : allTokens) {
        if (tok.type == TokenType::END_OF_FILE) {
            continue;
        }

        const std::uint64_t key = encodeLocationKey(tok.location);
        HighlightCategory cat = HighlightCategory::None;

        if (tok.type == TokenType::INVALID ||
            (!isTrivia(tok.type) && errorLocations.find(key) != errorLocations.end())) {
            cat = HighlightCategory::Error;
        } else {
            const auto astIt = astVisitor.roleByLocation.find(key);
            if (astIt != astVisitor.roleByLocation.end()) {
                cat = astIt->second;
            } else {
                cat = classifyLexicalFallback(tok);
            }
        }

        output.push_back(HighlightedToken{tok.lexeme, cat, tok.type, tok.location});
    }

    return output;
}

} // namespace bminor
