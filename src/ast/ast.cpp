#include "ast/ast.hpp"

#include <sstream>

namespace bminor {

std::string atomicTypeToString(AtomicType type) {
    switch (type) {
        case AtomicType::Integer: return "integer";
        case AtomicType::Boolean: return "boolean";
        case AtomicType::Char:    return "char";
        case AtomicType::String:  return "string";
    }
    return "unknown";
}

void AtomicTypeNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void VoidTypeNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ArrayTypeNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ExprInitializer::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ArrayInitializer::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IntLiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BoolLiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void CharLiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void StringLiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IdentifierExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void AssignExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BinaryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void PostfixExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ArrayIndexExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ErrorExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ParamNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void VarDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void FuncDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BlockStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IfStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ForStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void PrintStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void VarDeclStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ExprStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ErrorStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ProgramNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

namespace {

class AstDumper final : public ASTVisitor {
public:
    std::string str() const { return out_.str(); }

    void visit(const ProgramNode& node) override {
        writeLine("Program(" + locTag(node) + ")");
        for (std::size_t i = 0; i < node.declarations.size(); ++i) {
            visitChild("decl[" + std::to_string(i) + "]", node.declarations[i].get());
        }
    }

    void visit(const AtomicTypeNode& node) override {
        writeLine("AtomicType(" + atomicTypeToString(node.atomicType) + ", " + locTag(node) + ")");
    }

    void visit(const VoidTypeNode& node) override {
        writeLine("VoidType(" + locTag(node) + ")");
    }

    void visit(const ArrayTypeNode& node) override {
        writeLine("ArrayType(" + locTag(node) + ")");
        if (node.size) {
            visitChild("size", node.size.get());
        }
        visitChild("elementType", node.elementType.get());
    }

    void visit(const ExprInitializer& node) override {
        writeLine("ExprInitializer(" + locTag(node) + ")");
        visitChild("expr", node.expression.get());
    }

    void visit(const ArrayInitializer& node) override {
        writeLine("ArrayInitializer(" + locTag(node) + ")");
        for (std::size_t i = 0; i < node.elements.size(); ++i) {
            visitChild("elem[" + std::to_string(i) + "]", node.elements[i].get());
        }
    }

    void visit(const IntLiteralExpr& node) override {
        writeLine("IntLiteral(value=" + node.rawValue + ", " + locTag(node) + ")");
    }

    void visit(const BoolLiteralExpr& node) override {
        writeLine(std::string("BoolLiteral(value=") + (node.value ? "true" : "false") +
                  ", " + locTag(node) + ")");
    }

    void visit(const CharLiteralExpr& node) override {
        writeLine("CharLiteral(raw=" + node.rawValue + ", " + locTag(node) + ")");
    }

    void visit(const StringLiteralExpr& node) override {
        writeLine("StringLiteral(raw=" + node.rawValue + ", " + locTag(node) + ")");
    }

    void visit(const IdentifierExpr& node) override {
        writeLine("Identifier(name='" + node.name + "', " + locTag(node) + ")");
    }

    void visit(const AssignExpr& node) override {
        writeLine("AssignExpr(" + locTag(node) + ")");
        visitChild("target", node.target.get());
        visitChild("value", node.value.get());
    }

    void visit(const BinaryExpr& node) override {
        writeLine("BinaryExpr(op='" + node.op + "', " + locTag(node) + ")");
        visitChild("left", node.left.get());
        visitChild("right", node.right.get());
    }

    void visit(const UnaryExpr& node) override {
        writeLine("UnaryExpr(op='" + node.op + "', " + locTag(node) + ")");
        visitChild("operand", node.operand.get());
    }

    void visit(const PostfixExpr& node) override {
        writeLine("PostfixExpr(op='" + node.op + "', " + locTag(node) + ")");
        visitChild("operand", node.operand.get());
    }

    void visit(const CallExpr& node) override {
        std::string calleeDesc;
        if (const auto* ident = dynamic_cast<const IdentifierExpr*>(node.callee.get())) {
            calleeDesc = "callee='" + ident->name + "', ";
        }
        writeLine("CallExpr(" + calleeDesc + locTag(node) + ")");
        if (calleeDesc.empty()) {
            visitChild("callee", node.callee.get());
        }
        for (std::size_t i = 0; i < node.arguments.size(); ++i) {
            visitChild("args[" + std::to_string(i) + "]", node.arguments[i].get());
        }
    }

    void visit(const ArrayIndexExpr& node) override {
        writeLine("ArrayIndexExpr(" + locTag(node) + ")");
        visitChild("array", node.array.get());
        visitChild("index", node.index.get());
    }

    void visit(const ErrorExpr& node) override {
        writeLine("ErrorExpr(" + locTag(node) + ")");
    }

    void visit(const ParamNode& node) override {
        writeLine("Param(name='" + node.name + "', " + locTag(node) + ")");
        visitChild("type", node.type.get());
    }

    void visit(const VarDecl& node) override {
        writeLine(std::string("VarDecl(name='") + node.name +
                  "', scope=" + (node.isGlobal ? "global" : "local") +
                  ", " + locTag(node) + ")");
        visitChild("type", node.type.get());
        if (node.initializer) {
            visitChild("init", node.initializer.get());
        }
    }

    void visit(const FuncDecl& node) override {
        writeLine(std::string("FuncDecl(name='") + node.name +
                  "', kind=" + (node.isPrototype() ? "prototype" : "definition") +
                  ", " + locTag(node) + ")");
        visitChild("returnType", node.returnType.get());
        for (std::size_t i = 0; i < node.params.size(); ++i) {
            visitChild("param[" + std::to_string(i) + "]", node.params[i].get());
        }
        if (node.body) {
            visitChild("body", node.body.get());
        }
    }

    void visit(const BlockStmt& node) override {
        writeLine("BlockStmt(" + locTag(node) + ")");
        for (std::size_t i = 0; i < node.statements.size(); ++i) {
            visitChild("stmt[" + std::to_string(i) + "]", node.statements[i].get());
        }
    }

    void visit(const IfStmt& node) override {
        writeLine("IfStmt(" + locTag(node) + ")");
        visitChild("cond", node.condition.get());
        visitChild("then", node.thenBranch.get());
        if (node.elseBranch) {
            visitChild("else", node.elseBranch.get());
        }
    }

    void visit(const ForStmt& node) override {
        writeLine("ForStmt(" + locTag(node) + ")");
        if (node.init) {
            visitChild("init", node.init.get());
        }
        if (node.condition) {
            visitChild("cond", node.condition.get());
        }
        if (node.update) {
            visitChild("update", node.update.get());
        }
        visitChild("body", node.body.get());
    }

    void visit(const ReturnStmt& node) override {
        writeLine("ReturnStmt(" + locTag(node) + ")");
        if (node.value) {
            visitChild("value", node.value.get());
        }
    }

    void visit(const PrintStmt& node) override {
        writeLine("PrintStmt(" + locTag(node) + ")");
        for (std::size_t i = 0; i < node.expressions.size(); ++i) {
            visitChild("expr[" + std::to_string(i) + "]", node.expressions[i].get());
        }
    }

    void visit(const VarDeclStmt& node) override {
        if (node.decl) {
            node.decl->accept(*this);
        }
    }

    void visit(const ExprStmt& node) override {
        writeLine("ExprStmt(" + locTag(node) + ")");
        visitChild("expr", node.expression.get());
    }

    void visit(const ErrorStmt& node) override {
        writeLine("ErrorStmt(" + locTag(node) + ")");
    }

private:
    std::ostringstream out_;
    std::size_t indent_ = 0;
    std::string pendingLabel_;

    static std::string locTag(const ASTNode& node) {
        std::string tag = "loc=" + node.location.toLineColString();
        if (node.typeAnnotation.has_value()) {
            tag += ", type=" + *node.typeAnnotation;
        }
        return tag;
    }

    void writeLine(const std::string& text) {
        for (std::size_t i = 0; i < indent_; ++i) {
            out_ << "  ";
        }
        if (!pendingLabel_.empty()) {
            out_ << pendingLabel_ << ": ";
            pendingLabel_.clear();
        }
        out_ << text << '\n';
    }

    void visitChild(const std::string& label, const ASTNode* child) {
        ++indent_;
        pendingLabel_ = label;
        if (child != nullptr) {
            child->accept(*this);
        } else {
            writeLine("<null>");
        }
        pendingLabel_.clear();
        --indent_;
    }
};

} // namespace

std::string dumpAst(const ASTNode& root) {
    AstDumper dumper;
    root.accept(dumper);
    return dumper.str();
}

} // namespace bminor
