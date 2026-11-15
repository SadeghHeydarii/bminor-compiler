#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace bminor;

namespace {

ParseResult parseSource(const std::string& src, LexResult* outLex = nullptr) {
    Lexer lexer(src, "test.bm");
    LexResult lexRes = lexer.tokenize();
    if (outLex != nullptr) {
        *outLex = lexRes;
    }
    Parser parser(lexRes.parserTokens, "test.bm");
    return parser.parse();
}

void testFactorialAstLocationsAndAnnotations() {
    const std::string src =
        "factorial: function integer ( n: integer ) = {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * factorial(n - 1);\n"
        "}\n";

    LexResult lexRes;
    ParseResult res = parseSource(src, &lexRes);
    assert(lexRes.diagnostics.empty());
    assert(!res.hasErrors());
    assert(res.program != nullptr);
    assert(!res.program->typeAnnotation.has_value());
    assert(res.program->declarations.size() == 1);

    const auto* fn = dynamic_cast<const FuncDecl*>(res.program->declarations[0].get());
    assert(fn != nullptr);
    assert(fn->name == "factorial");
    assert(!fn->isPrototype());
    assert(!fn->typeAnnotation.has_value());
    assert(fn->nameLocation.line == 1 && fn->nameLocation.column == 1);
    assert(fn->params.size() == 1);
    assert(fn->params[0]->name == "n");
    assert(fn->body != nullptr && fn->body->statements.size() == 2);

    const auto* retStmt = dynamic_cast<const ReturnStmt*>(fn->body->statements[1].get());
    assert(retStmt != nullptr);
    const auto* mulExpr = dynamic_cast<const BinaryExpr*>(retStmt->value.get());
    assert(mulExpr != nullptr && mulExpr->op == "*");
    assert(!mulExpr->typeAnnotation.has_value());

    const auto* leftVar = dynamic_cast<const IdentifierExpr*>(mulExpr->left.get());
    assert(leftVar != nullptr && leftVar->name == "n");
    assert(leftVar->location.line == 3 && leftVar->location.column == 12);

    const auto* callExpr = dynamic_cast<const CallExpr*>(mulExpr->right.get());
    assert(callExpr != nullptr);
    assert(callExpr->location.line == 3 && callExpr->location.column == 16);
    const auto* calleeIdent = dynamic_cast<const IdentifierExpr*>(callExpr->callee.get());
    assert(calleeIdent != nullptr && calleeIdent->name == "factorial");
    assert(callExpr->arguments.size() == 1);

    const auto* subExpr = dynamic_cast<const BinaryExpr*>(callExpr->arguments[0].get());
    assert(subExpr != nullptr && subExpr->op == "-");
    const auto* argLit = dynamic_cast<const IntLiteralExpr*>(subExpr->right.get());
    assert(argLit != nullptr && argLit->value == 1);
    assert(argLit->location.line == 3 && argLit->location.column == 30);

    const std::string dumped = dumpAst(*res.program);
    assert(dumped.find("CallExpr(callee='factorial', loc=3:16)") != std::string::npos);
}

void testAllDeclarationsTypesAndStatements() {
    const std::string src =
        "puts: function void ( s: string );\n"
        "mat: array [2] array [2] integer = { {1, 2}, {3, 4} };\n"
        "demo: function integer ( arr: array [] integer ) = {\n"
        "    i: integer = 0;\n"
        "    for (i = 0; i < 4; i++) {\n"
        "        print arr[i], '\\n';\n"
        "    }\n"
        "    for (;;) return 0;\n"
        "    if (true) if (false) return 1; else return 2;\n"
        "    return;\n"
        "}\n";

    ParseResult res = parseSource(src);
    assert(!res.hasErrors());
    assert(res.program != nullptr);
    assert(res.program->declarations.size() == 3);

    const auto* proto = dynamic_cast<const FuncDecl*>(res.program->declarations[0].get());
    assert(proto != nullptr && proto->isPrototype());
    assert(dynamic_cast<const VoidTypeNode*>(proto->returnType.get()) != nullptr);

    const auto* matDecl = dynamic_cast<const VarDecl*>(res.program->declarations[1].get());
    assert(matDecl != nullptr && matDecl->isGlobal);
    const auto* outerArr = dynamic_cast<const ArrayTypeNode*>(matDecl->type.get());
    assert(outerArr != nullptr && outerArr->size != nullptr);
    const auto* innerArr = dynamic_cast<const ArrayTypeNode*>(outerArr->elementType.get());
    assert(innerArr != nullptr && innerArr->size != nullptr);
    const auto* arrInit = dynamic_cast<const ArrayInitializer*>(matDecl->initializer.get());
    assert(arrInit != nullptr && arrInit->elements.size() == 2);

    const auto* demoFn = dynamic_cast<const FuncDecl*>(res.program->declarations[2].get());
    assert(demoFn != nullptr && demoFn->body != nullptr && demoFn->body->statements.size() == 5);
    assert(dynamic_cast<const VarDeclStmt*>(demoFn->body->statements[0].get()) != nullptr);

    const auto* forFull = dynamic_cast<const ForStmt*>(demoFn->body->statements[1].get());
    assert(forFull != nullptr && forFull->init != nullptr && forFull->condition != nullptr &&
           forFull->update != nullptr);

    const auto* forEmpty = dynamic_cast<const ForStmt*>(demoFn->body->statements[2].get());
    assert(forEmpty != nullptr && forEmpty->init == nullptr && forEmpty->condition == nullptr &&
           forEmpty->update == nullptr);

    // Dangling else must bind to the inner if ("if (false) ... else ...")
    const auto* outerIf = dynamic_cast<const IfStmt*>(demoFn->body->statements[3].get());
    assert(outerIf != nullptr && outerIf->elseBranch == nullptr);
    const auto* innerIf = dynamic_cast<const IfStmt*>(outerIf->thenBranch.get());
    assert(innerIf != nullptr && innerIf->elseBranch != nullptr);

    const auto* emptyRet = dynamic_cast<const ReturnStmt*>(demoFn->body->statements[4].get());
    assert(emptyRet != nullptr && emptyRet->value == nullptr);
}

void testOperatorPrecedenceAndAssociativity() {
    const std::string src =
        "calc: function void () = {\n"
        "    a = b = 1 + 2 * 3 - 4 ^ 2 ^ 1;\n"
        "    x = ! -y || z && f(1)[0]++;\n"
        "}\n";

    ParseResult res = parseSource(src);
    assert(!res.hasErrors());
    const auto* fn = dynamic_cast<const FuncDecl*>(res.program->declarations[0].get());
    assert(fn != nullptr && fn->body->statements.size() == 2);

    // Check right-associative '=': a = (b = ...)
    const auto* s0 = dynamic_cast<const ExprStmt*>(fn->body->statements[0].get());
    const auto* assignOuter = dynamic_cast<const AssignExpr*>(s0->expression.get());
    assert(assignOuter != nullptr);
    const auto* assignInner = dynamic_cast<const AssignExpr*>(assignOuter->value.get());
    assert(assignInner != nullptr);

    // Check right-associative '^' inside (1 + 2*3) - (4 ^ (2 ^ 1))
    const auto* sub = dynamic_cast<const BinaryExpr*>(assignInner->value.get());
    assert(sub != nullptr && sub->op == "-");
    const auto* powOuter = dynamic_cast<const BinaryExpr*>(sub->right.get());
    assert(powOuter != nullptr && powOuter->op == "^");
    const auto* powInner = dynamic_cast<const BinaryExpr*>(powOuter->right.get());
    assert(powInner != nullptr && powInner->op == "^");

    // Check chained postfix: (f(1))[0]++
    const auto* s1 = dynamic_cast<const ExprStmt*>(fn->body->statements[1].get());
    const auto* assign1 = dynamic_cast<const AssignExpr*>(s1->expression.get());
    const auto* orExpr = dynamic_cast<const BinaryExpr*>(assign1->value.get());
    assert(orExpr != nullptr && orExpr->op == "||");
    const auto* andExpr = dynamic_cast<const BinaryExpr*>(orExpr->right.get());
    assert(andExpr != nullptr && andExpr->op == "&&");
    const auto* postInc = dynamic_cast<const PostfixExpr*>(andExpr->right.get());
    assert(postInc != nullptr && postInc->op == "++");
    const auto* idxExpr = dynamic_cast<const ArrayIndexExpr*>(postInc->operand.get());
    assert(idxExpr != nullptr);
    const auto* callExpr = dynamic_cast<const CallExpr*>(idxExpr->array.get());
    assert(callExpr != nullptr);
}

void testPanicModeMissingExprAndMissingParen() {
    const std::string src =
        "main: function integer () = {\n"
        "    x: integer = ;\n"
        "    if (x > 0 {\n"
        "        print x;\n"
        "    }\n"
        "    a: integer = 1 + * 2;\n"
        "    y: integer = 42;\n"
        "    return y;\n"
        "}\n";

    ParseResult res = parseSource(src);
    assert(res.hasErrors());
    assert(res.diagnostics.size() == 3);
    assert(res.diagnostics[0].stage == DiagnosticStage::Syntactic);
    assert(res.diagnostics[0].message.find("expected expression, got ';'") != std::string::npos);
    assert(res.diagnostics[1].message.find("missing ')' before '{'") != std::string::npos);
    assert(res.diagnostics[2].message.find("expected expression, got '*'") != std::string::npos);

    // Ensure recovery preserved the function, the if-body, and subsequent valid declarations
    assert(res.program != nullptr && res.program->declarations.size() == 1);
    const auto* fn = dynamic_cast<const FuncDecl*>(res.program->declarations[0].get());
    assert(fn != nullptr && fn->body != nullptr && fn->body->statements.size() == 5);

    const auto* recoveredIf = dynamic_cast<const IfStmt*>(fn->body->statements[1].get());
    assert(recoveredIf != nullptr);
    const auto* ifBlock = dynamic_cast<const BlockStmt*>(recoveredIf->thenBranch.get());
    assert(ifBlock != nullptr && ifBlock->statements.size() == 1);
    assert(dynamic_cast<const PrintStmt*>(ifBlock->statements[0].get()) != nullptr);

    const auto* yStmt = dynamic_cast<const VarDeclStmt*>(fn->body->statements[3].get());
    assert(yStmt != nullptr && yStmt->decl != nullptr && yStmt->decl->name == "y");
    const auto* retStmt = dynamic_cast<const ReturnStmt*>(fn->body->statements[4].get());
    assert(retStmt != nullptr && retStmt->value != nullptr);
}

void testTopLevelRecoveryAndDegenerateInputs() {
    const std::string src =
        "bad_decl 123;\n"
        "x: integer = 5@;\n"
        "ok: function integer () = { return 7; }\n";

    LexResult lexRes;
    ParseResult res = parseSource(src, &lexRes);
    assert(!lexRes.diagnostics.empty()); // Lexical error from '@'
    assert(res.hasErrors());            // Syntax error from 'bad_decl 123;'
    assert(res.program != nullptr);
    // Both x (with '@' skipped) and ok() should still be parsed
    assert(res.program->declarations.size() == 2);
    const auto* varX = dynamic_cast<const VarDecl*>(res.program->declarations[0].get());
    const auto* fnOk = dynamic_cast<const FuncDecl*>(res.program->declarations[1].get());
    assert(varX != nullptr && varX->name == "x");
    assert(fnOk != nullptr && fnOk->name == "ok");

    // Empty and truncated inputs must never crash
    Parser emptyParser({}, "empty.bm");
    ParseResult emptyRes = emptyParser.parse();
    assert(!emptyRes.hasErrors() && emptyRes.program != nullptr &&
           emptyRes.program->declarations.empty());

    ParseResult truncatedRes = parseSource("f: function integer (");
    assert(truncatedRes.hasErrors());
    assert(truncatedRes.program != nullptr);
}

} // namespace

int main() {
    testFactorialAstLocationsAndAnnotations();
    testAllDeclarationsTypesAndStatements();
    testOperatorPrecedenceAndAssociativity();
    testPanicModeMissingExprAndMissingParen();
    testTopLevelRecoveryAndDegenerateInputs();

    std::cout << "All Parser & AST unit tests passed successfully.\n";
    return 0;
}
