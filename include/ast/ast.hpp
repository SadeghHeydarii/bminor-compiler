#ifndef BMINOR_AST_AST_HPP
#define BMINOR_AST_AST_HPP

#include "common/source_location.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bminor {

class ASTVisitor;

/// Atomic built-in B-Minor value types.
enum class AtomicType {
    Integer,
    Boolean,
    Char,
    String
};

/// Returns the B-Minor source spelling of an atomic type.
std::string atomicTypeToString(AtomicType type);

/// Abstract base class for all B-Minor AST nodes.
class ASTNode {
public:
    SourceLocation location;
    /// Reserved for type information populated during Semantic Analysis; empty in Phase 1.
    std::optional<std::string> typeAnnotation;

    explicit ASTNode(SourceLocation loc) : location(std::move(loc)) {}
    virtual ~ASTNode() = default;

    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    virtual void accept(ASTVisitor& visitor) const = 0;
};

/// Base class for type annotations.
class TypeNode : public ASTNode {
public:
    using ASTNode::ASTNode;
};

/// Base class for variable initializers.
class InitializerNode : public ASTNode {
public:
    using ASTNode::ASTNode;
};

/// Base class for all expression nodes.
class ExprNode : public ASTNode {
public:
    using ASTNode::ASTNode;
};

/// Base class for all statement nodes.
class StmtNode : public ASTNode {
public:
    using ASTNode::ASTNode;
};

/// Base class for top-level and local declarations.
class DeclNode : public ASTNode {
public:
    using ASTNode::ASTNode;
};

// ---------------------------------------------------------------------------
// Type Nodes
// ---------------------------------------------------------------------------

class AtomicTypeNode final : public TypeNode {
public:
    AtomicType atomicType;

    AtomicTypeNode(SourceLocation loc, AtomicType type)
        : TypeNode(std::move(loc)), atomicType(type) {}

    void accept(ASTVisitor& visitor) const override;
};

class VoidTypeNode final : public TypeNode {
public:
    explicit VoidTypeNode(SourceLocation loc) : TypeNode(std::move(loc)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ArrayTypeNode final : public TypeNode {
public:
    std::unique_ptr<ExprNode> size; // Optional: omitted in function parameters
    std::unique_ptr<TypeNode> elementType;

    ArrayTypeNode(SourceLocation loc,
                  std::unique_ptr<ExprNode> arraySize,
                  std::unique_ptr<TypeNode> elemType)
        : TypeNode(std::move(loc)),
          size(std::move(arraySize)),
          elementType(std::move(elemType)) {}

    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Initializer Nodes
// ---------------------------------------------------------------------------

class ExprInitializer final : public InitializerNode {
public:
    std::unique_ptr<ExprNode> expression;

    ExprInitializer(SourceLocation loc, std::unique_ptr<ExprNode> expr)
        : InitializerNode(std::move(loc)), expression(std::move(expr)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ArrayInitializer final : public InitializerNode {
public:
    std::vector<std::unique_ptr<InitializerNode>> elements;

    ArrayInitializer(SourceLocation loc,
                     std::vector<std::unique_ptr<InitializerNode>> elems)
        : InitializerNode(std::move(loc)), elements(std::move(elems)) {}

    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Leaf Expression Nodes
// ---------------------------------------------------------------------------

class IntLiteralExpr final : public ExprNode {
public:
    std::string rawValue;
    long long value = 0;
    bool overflowed = false;

    IntLiteralExpr(SourceLocation loc, std::string raw, long long val, bool isOverflow = false)
        : ExprNode(std::move(loc)),
          rawValue(std::move(raw)),
          value(val),
          overflowed(isOverflow) {}

    void accept(ASTVisitor& visitor) const override;
};

class BoolLiteralExpr final : public ExprNode {
public:
    bool value = false;

    BoolLiteralExpr(SourceLocation loc, bool val)
        : ExprNode(std::move(loc)), value(val) {}

    void accept(ASTVisitor& visitor) const override;
};

class CharLiteralExpr final : public ExprNode {
public:
    std::string rawValue;
    char value = '\0';

    CharLiteralExpr(SourceLocation loc, std::string raw, char val)
        : ExprNode(std::move(loc)), rawValue(std::move(raw)), value(val) {}

    void accept(ASTVisitor& visitor) const override;
};

class StringLiteralExpr final : public ExprNode {
public:
    std::string rawValue;
    std::string value;

    StringLiteralExpr(SourceLocation loc, std::string raw, std::string val)
        : ExprNode(std::move(loc)), rawValue(std::move(raw)), value(std::move(val)) {}

    void accept(ASTVisitor& visitor) const override;
};

/// Variable identifier expression (distinguished structurally from function names).
class IdentifierExpr final : public ExprNode {
public:
    std::string name;

    IdentifierExpr(SourceLocation loc, std::string identName)
        : ExprNode(std::move(loc)), name(std::move(identName)) {}

    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Internal Expression Nodes
// ---------------------------------------------------------------------------

class AssignExpr final : public ExprNode {
public:
    std::unique_ptr<ExprNode> target;
    std::unique_ptr<ExprNode> value;

    AssignExpr(SourceLocation loc,
               std::unique_ptr<ExprNode> lhs,
               std::unique_ptr<ExprNode> rhs)
        : ExprNode(std::move(loc)), target(std::move(lhs)), value(std::move(rhs)) {}

    void accept(ASTVisitor& visitor) const override;
};

class BinaryExpr final : public ExprNode {
public:
    std::string op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;

    BinaryExpr(SourceLocation loc,
               std::string oper,
               std::unique_ptr<ExprNode> lhs,
               std::unique_ptr<ExprNode> rhs)
        : ExprNode(std::move(loc)),
          op(std::move(oper)),
          left(std::move(lhs)),
          right(std::move(rhs)) {}

    void accept(ASTVisitor& visitor) const override;
};

class UnaryExpr final : public ExprNode {
public:
    std::string op;
    std::unique_ptr<ExprNode> operand;

    UnaryExpr(SourceLocation loc, std::string oper, std::unique_ptr<ExprNode> expr)
        : ExprNode(std::move(loc)), op(std::move(oper)), operand(std::move(expr)) {}

    void accept(ASTVisitor& visitor) const override;
};

class PostfixExpr final : public ExprNode {
public:
    std::string op;
    std::unique_ptr<ExprNode> operand;

    PostfixExpr(SourceLocation loc, std::string oper, std::unique_ptr<ExprNode> expr)
        : ExprNode(std::move(loc)), op(std::move(oper)), operand(std::move(expr)) {}

    void accept(ASTVisitor& visitor) const override;
};

/// Function call node; callee IdentifierExpr represents a callable name, not a variable.
class CallExpr final : public ExprNode {
public:
    std::unique_ptr<ExprNode> callee;
    std::vector<std::unique_ptr<ExprNode>> arguments;

    CallExpr(SourceLocation loc,
             std::unique_ptr<ExprNode> calleeExpr,
             std::vector<std::unique_ptr<ExprNode>> args)
        : ExprNode(std::move(loc)),
          callee(std::move(calleeExpr)),
          arguments(std::move(args)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ArrayIndexExpr final : public ExprNode {
public:
    std::unique_ptr<ExprNode> array;
    std::unique_ptr<ExprNode> index;

    ArrayIndexExpr(SourceLocation loc,
                   std::unique_ptr<ExprNode> arrayExpr,
                   std::unique_ptr<ExprNode> indexExpr)
        : ExprNode(std::move(loc)),
          array(std::move(arrayExpr)),
          index(std::move(indexExpr)) {}

    void accept(ASTVisitor& visitor) const override;
};

/// Placeholder node inserted when panic-mode recovery repairs a malformed expression.
class ErrorExpr final : public ExprNode {
public:
    explicit ErrorExpr(SourceLocation loc) : ExprNode(std::move(loc)) {}

    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Declarations & Parameters
// ---------------------------------------------------------------------------

class ParamNode final : public ASTNode {
public:
    std::string name;
    SourceLocation nameLocation;
    std::unique_ptr<TypeNode> type;

    ParamNode(SourceLocation loc,
              std::string paramName,
              SourceLocation nameLoc,
              std::unique_ptr<TypeNode> paramType)
        : ASTNode(std::move(loc)),
          name(std::move(paramName)),
          nameLocation(std::move(nameLoc)),
          type(std::move(paramType)) {}

    void accept(ASTVisitor& visitor) const override;
};

class VarDecl final : public DeclNode {
public:
    std::string name;
    SourceLocation nameLocation;
    std::unique_ptr<TypeNode> type;
    std::unique_ptr<InitializerNode> initializer; // Optional
    bool isGlobal = false;

    VarDecl(SourceLocation loc,
            std::string varName,
            SourceLocation nameLoc,
            std::unique_ptr<TypeNode> varType,
            std::unique_ptr<InitializerNode> init,
            bool globalDecl)
        : DeclNode(std::move(loc)),
          name(std::move(varName)),
          nameLocation(std::move(nameLoc)),
          type(std::move(varType)),
          initializer(std::move(init)),
          isGlobal(globalDecl) {}

    void accept(ASTVisitor& visitor) const override;
};

class BlockStmt;

class FuncDecl final : public DeclNode {
public:
    std::string name;
    SourceLocation nameLocation;
    std::unique_ptr<TypeNode> returnType;
    std::vector<std::unique_ptr<ParamNode>> params;
    std::unique_ptr<BlockStmt> body; // Null for function prototypes ending in ';'

    FuncDecl(SourceLocation loc,
             std::string funcName,
             SourceLocation nameLoc,
             std::unique_ptr<TypeNode> retType,
             std::vector<std::unique_ptr<ParamNode>> paramList,
             std::unique_ptr<BlockStmt> funcBody)
        : DeclNode(std::move(loc)),
          name(std::move(funcName)),
          nameLocation(std::move(nameLoc)),
          returnType(std::move(retType)),
          params(std::move(paramList)),
          body(std::move(funcBody)) {}

    bool isPrototype() const { return body == nullptr; }

    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Statement Nodes
// ---------------------------------------------------------------------------

class BlockStmt final : public StmtNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;

    BlockStmt(SourceLocation loc, std::vector<std::unique_ptr<StmtNode>> stmts)
        : StmtNode(std::move(loc)), statements(std::move(stmts)) {}

    void accept(ASTVisitor& visitor) const override;
};

class IfStmt final : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> thenBranch;
    std::unique_ptr<StmtNode> elseBranch; // Optional

    IfStmt(SourceLocation loc,
           std::unique_ptr<ExprNode> cond,
           std::unique_ptr<StmtNode> thenStmt,
           std::unique_ptr<StmtNode> elseStmt)
        : StmtNode(std::move(loc)),
          condition(std::move(cond)),
          thenBranch(std::move(thenStmt)),
          elseBranch(std::move(elseStmt)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ForStmt final : public StmtNode {
public:
    std::unique_ptr<ExprNode> init;      // Optional
    std::unique_ptr<ExprNode> condition; // Optional
    std::unique_ptr<ExprNode> update;    // Optional
    std::unique_ptr<StmtNode> body;

    ForStmt(SourceLocation loc,
            std::unique_ptr<ExprNode> initExpr,
            std::unique_ptr<ExprNode> condExpr,
            std::unique_ptr<ExprNode> updateExpr,
            std::unique_ptr<StmtNode> bodyStmt)
        : StmtNode(std::move(loc)),
          init(std::move(initExpr)),
          condition(std::move(condExpr)),
          update(std::move(updateExpr)),
          body(std::move(bodyStmt)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ReturnStmt final : public StmtNode {
public:
    std::unique_ptr<ExprNode> value; // Optional

    ReturnStmt(SourceLocation loc, std::unique_ptr<ExprNode> expr)
        : StmtNode(std::move(loc)), value(std::move(expr)) {}

    void accept(ASTVisitor& visitor) const override;
};

class PrintStmt final : public StmtNode {
public:
    std::vector<std::unique_ptr<ExprNode>> expressions;

    PrintStmt(SourceLocation loc, std::vector<std::unique_ptr<ExprNode>> exprs)
        : StmtNode(std::move(loc)), expressions(std::move(exprs)) {}

    void accept(ASTVisitor& visitor) const override;
};

class VarDeclStmt final : public StmtNode {
public:
    std::unique_ptr<VarDecl> decl;

    VarDeclStmt(SourceLocation loc, std::unique_ptr<VarDecl> varDecl)
        : StmtNode(std::move(loc)), decl(std::move(varDecl)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ExprStmt final : public StmtNode {
public:
    std::unique_ptr<ExprNode> expression;

    ExprStmt(SourceLocation loc, std::unique_ptr<ExprNode> expr)
        : StmtNode(std::move(loc)), expression(std::move(expr)) {}

    void accept(ASTVisitor& visitor) const override;
};

/// Placeholder node inserted when panic-mode recovery repairs a malformed statement.
class ErrorStmt final : public StmtNode {
public:
    explicit ErrorStmt(SourceLocation loc) : StmtNode(std::move(loc)) {}

    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// Program Root
// ---------------------------------------------------------------------------

class ProgramNode final : public ASTNode {
public:
    std::vector<std::unique_ptr<DeclNode>> declarations;

    ProgramNode(SourceLocation loc, std::vector<std::unique_ptr<DeclNode>> decls)
        : ASTNode(std::move(loc)), declarations(std::move(decls)) {}

    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------------
// AST Visitor & Formatting
// ---------------------------------------------------------------------------

/// Visitor interface for traversing any B-Minor AST structure.
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(const ProgramNode& node) = 0;
    virtual void visit(const AtomicTypeNode& node) = 0;
    virtual void visit(const VoidTypeNode& node) = 0;
    virtual void visit(const ArrayTypeNode& node) = 0;
    virtual void visit(const ExprInitializer& node) = 0;
    virtual void visit(const ArrayInitializer& node) = 0;
    virtual void visit(const IntLiteralExpr& node) = 0;
    virtual void visit(const BoolLiteralExpr& node) = 0;
    virtual void visit(const CharLiteralExpr& node) = 0;
    virtual void visit(const StringLiteralExpr& node) = 0;
    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const AssignExpr& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const PostfixExpr& node) = 0;
    virtual void visit(const CallExpr& node) = 0;
    virtual void visit(const ArrayIndexExpr& node) = 0;
    virtual void visit(const ErrorExpr& node) = 0;
    virtual void visit(const ParamNode& node) = 0;
    virtual void visit(const VarDecl& node) = 0;
    virtual void visit(const FuncDecl& node) = 0;
    virtual void visit(const BlockStmt& node) = 0;
    virtual void visit(const IfStmt& node) = 0;
    virtual void visit(const ForStmt& node) = 0;
    virtual void visit(const ReturnStmt& node) = 0;
    virtual void visit(const PrintStmt& node) = 0;
    virtual void visit(const VarDeclStmt& node) = 0;
    virtual void visit(const ExprStmt& node) = 0;
    virtual void visit(const ErrorStmt& node) = 0;
};

/// Formats an AST subtree into a readable indented tree string with source locations.
std::string dumpAst(const ASTNode& root);

} // namespace bminor

#endif // BMINOR_AST_AST_HPP
