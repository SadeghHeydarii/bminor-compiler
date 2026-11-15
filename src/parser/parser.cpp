#include "parser/parser.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace bminor {

namespace {

char decodeEscapeChar(char escaped) {
    switch (escaped) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '0':  return '\0';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"':  return '"';
        default:   return escaped;
    }
}

std::string unescapeContent(const std::string& rawQuoted) {
    if (rawQuoted.size() < 2) {
        return {};
    }
    const std::string inner = rawQuoted.substr(1, rawQuoted.size() - 2);
    std::string out;
    out.reserve(inner.size());
    for (std::size_t i = 0; i < inner.size(); ++i) {
        if (inner[i] == '\\' && i + 1 < inner.size()) {
            out.push_back(decodeEscapeChar(inner[++i]));
        } else {
            out.push_back(inner[i]);
        }
    }
    return out;
}

} // namespace

Parser::Parser(const std::vector<Token>& tokens, std::string filename)
    : filename_(std::move(filename)) {
    tokens_.reserve(tokens.size() + 1);
    for (const auto& tok : tokens) {
        // Filter trivia and lexer-diagnosed INVALID tokens so parsing stays focused on structure.
        if (!isTrivia(tok.type) && tok.type != TokenType::INVALID) {
            tokens_.push_back(tok);
        }
    }
    if (tokens_.empty() || tokens_.back().type != TokenType::END_OF_FILE) {
        Token eofTok;
        eofTok.type = TokenType::END_OF_FILE;
        eofTok.location = tokens_.empty()
                              ? SourceLocation{filename_, 1, 1}
                              : tokens_.back().location;
        tokens_.push_back(eofTok);
    }
}

ParseResult Parser::parse() {
    ParseResult result;
    std::vector<std::unique_ptr<DeclNode>> decls;
    const SourceLocation programLoc = peek().location;

    while (!isAtEnd()) {
        const std::size_t startPos = current_;
        auto decl = parseDeclaration();
        if (decl) {
            decls.push_back(std::move(decl));
        }
        if (current_ == startPos && !isAtEnd()) {
            advance();
        }
        panicMode_ = false;
    }

    result.program = std::make_unique<ProgramNode>(programLoc, std::move(decls));
    result.diagnostics = std::move(diagnostics_);
    return result;
}

const Token& Parser::peek(std::size_t offset) const {
    const std::size_t idx = current_ + offset;
    if (idx >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[idx];
}

const Token& Parser::previous() const {
    if (current_ == 0) {
        return tokens_.front();
    }
    return tokens_[current_ - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        ++current_;
    }
    return previous();
}

std::string Parser::describeGotToken(const Token& token) {
    if (token.type == TokenType::END_OF_FILE) {
        return "EOF";
    }
    return "'" + token.lexeme + "'";
}

void Parser::reportError(const SourceLocation& loc, const std::string& message) {
    if (panicMode_) {
        return;
    }
    diagnostics_.push_back(Diagnostic{DiagnosticStage::Syntactic, loc, message});
    panicMode_ = true;
}

bool Parser::expect(TokenType expected, const std::string& expectedDesc) {
    if (match(expected)) {
        return true;
    }
    const Token& tok = peek();
    reportError(tok.location, "expected " + expectedDesc + ", got " + describeGotToken(tok));
    return false;
}

bool Parser::expectClosingParenBeforeBlockOrStmt() {
    if (match(TokenType::RPAREN)) {
        return true;
    }
    const Token& tok = peek();
    if (tok.type == TokenType::LBRACE) {
        reportError(tok.location, "missing ')' before '{'");
        // Clear panic mode right before '{' so the following block body is parsed intact.
        panicMode_ = false;
        return false;
    }
    reportError(tok.location, "expected ')', got " + describeGotToken(tok));
    syncUntilClosingParenOrStmt();
    return false;
}

bool Parser::expectStmtEnd() {
    if (match(TokenType::SEMI)) {
        panicMode_ = false;
        return true;
    }
    const Token& tok = peek();
    reportError(tok.location, "expected ';', got " + describeGotToken(tok));
    synchronizeStmt();
    return false;
}

bool Parser::isStmtSyncPoint() const {
    switch (peek().type) {
        case TokenType::SEMI:
        case TokenType::LBRACE:
        case TokenType::RBRACE:
        case TokenType::KW_IF:
        case TokenType::KW_FOR:
        case TokenType::KW_RETURN:
        case TokenType::KW_PRINT:
        case TokenType::END_OF_FILE:
            return true;
        case TokenType::IDENT:
            return peek(1).type == TokenType::COLON;
        default:
            return false;
    }
}

void Parser::synchronizeStmt() {
    while (!isAtEnd()) {
        if (match(TokenType::SEMI)) {
            panicMode_ = false;
            return;
        }
        if (isStmtSyncPoint()) {
            panicMode_ = false;
            return;
        }
        advance();
    }
    panicMode_ = false;
}

void Parser::syncUntilClosingParenOrStmt() {
    while (!isAtEnd()) {
        if (match(TokenType::RPAREN)) {
            panicMode_ = false;
            return;
        }
        if (isStmtSyncPoint()) {
            panicMode_ = false;
            return;
        }
        advance();
    }
    panicMode_ = false;
}

void Parser::synchronizeDecl() {
    int braceDepth = 0;
    while (!isAtEnd()) {
        const TokenType t = peek().type;
        if (t == TokenType::LBRACE) {
            ++braceDepth;
            advance();
            continue;
        }
        if (t == TokenType::RBRACE) {
            advance();
            if (braceDepth > 0) {
                --braceDepth;
            }
            if (braceDepth == 0) {
                panicMode_ = false;
                return;
            }
            continue;
        }
        if (braceDepth == 0) {
            if (t == TokenType::SEMI) {
                advance();
                panicMode_ = false;
                return;
            }
            if (t == TokenType::IDENT && peek(1).type == TokenType::COLON) {
                panicMode_ = false;
                return;
            }
        }
        advance();
    }
    panicMode_ = false;
}

// ---------------------------------------------------------------------------
// Declarations & Types
// ---------------------------------------------------------------------------

std::unique_ptr<DeclNode> Parser::parseDeclaration() {
    if (!check(TokenType::IDENT)) {
        const Token& tok = peek();
        reportError(tok.location, "expected declaration, got " + describeGotToken(tok));
        synchronizeDecl();
        return nullptr;
    }

    const Token nameTok = advance();
    if (!expect(TokenType::COLON, "':'")) {
        synchronizeDecl();
        return nullptr;
    }

    if (check(TokenType::KW_FUNCTION)) {
        return parseFunctionTail(nameTok);
    }
    return parseVarTail(nameTok, /*isGlobal=*/true);
}

std::unique_ptr<FuncDecl> Parser::parseFunctionTail(const Token& nameTok) {
    advance(); // Consume 'function'

    auto retType = parseReturnType();
    if (!expect(TokenType::LPAREN, "'('")) {
        syncUntilClosingParenOrStmt();
    }

    std::vector<std::unique_ptr<ParamNode>> params;
    if (!check(TokenType::RPAREN) && !check(TokenType::LBRACE) &&
        !check(TokenType::SEMI) && !check(TokenType::OP_ASSIGN) && !isAtEnd()) {
        params = parseParamList();
    }

    if (!match(TokenType::RPAREN)) {
        const Token& tok = peek();
        if (tok.type == TokenType::LBRACE) {
            reportError(tok.location, "missing ')' before '{'");
            panicMode_ = false;
        } else {
            reportError(tok.location, "expected ')', got " + describeGotToken(tok));
            while (!isAtEnd() && !check(TokenType::RPAREN) && !check(TokenType::OP_ASSIGN) &&
                   !check(TokenType::SEMI) && !check(TokenType::LBRACE)) {
                advance();
            }
            match(TokenType::RPAREN);
            panicMode_ = false;
        }
    }

    std::unique_ptr<BlockStmt> body;
    if (match(TokenType::OP_ASSIGN)) {
        if (check(TokenType::LBRACE)) {
            body = parseBlock();
        } else {
            const Token& tok = peek();
            reportError(tok.location, "expected '{', got " + describeGotToken(tok));
            synchronizeDecl();
        }
    } else if (match(TokenType::SEMI)) {
        // Function prototype ending in ';'
    } else if (check(TokenType::LBRACE)) {
        // Recover cleanly when '=' was omitted before function body '{ ... }'
        const Token& tok = peek();
        reportError(tok.location, "expected '=' or ';' before '{'");
        panicMode_ = false;
        body = parseBlock();
    } else {
        const Token& tok = peek();
        reportError(tok.location, "expected '=' or ';', got " + describeGotToken(tok));
        synchronizeDecl();
    }

    return std::make_unique<FuncDecl>(nameTok.location, nameTok.lexeme, nameTok.location,
                                      std::move(retType), std::move(params), std::move(body));
}

std::unique_ptr<VarDecl> Parser::parseVarTail(const Token& nameTok, bool isGlobal) {
    auto varType = parseType();

    std::unique_ptr<InitializerNode> init;
    if (match(TokenType::OP_ASSIGN)) {
        init = parseInitializer();
    }

    if (isGlobal) {
        if (!match(TokenType::SEMI)) {
            const Token& tok = peek();
            reportError(tok.location, "expected ';', got " + describeGotToken(tok));
            synchronizeDecl();
        } else {
            panicMode_ = false;
        }
    } else {
        expectStmtEnd();
    }

    return std::make_unique<VarDecl>(nameTok.location, nameTok.lexeme, nameTok.location,
                                     std::move(varType), std::move(init), isGlobal);
}

std::vector<std::unique_ptr<ParamNode>> Parser::parseParamList() {
    std::vector<std::unique_ptr<ParamNode>> params;
    auto first = parseParam();
    if (first) {
        params.push_back(std::move(first));
    }
    while (match(TokenType::COMMA)) {
        auto nextParam = parseParam();
        if (nextParam) {
            params.push_back(std::move(nextParam));
        } else {
            break;
        }
    }
    return params;
}

std::unique_ptr<ParamNode> Parser::parseParam() {
    if (!check(TokenType::IDENT)) {
        const Token& tok = peek();
        reportError(tok.location, "expected parameter name, got " + describeGotToken(tok));
        return nullptr;
    }
    const Token nameTok = advance();
    if (!expect(TokenType::COLON, "':'")) {
        return nullptr;
    }
    auto type = parseType();
    return std::make_unique<ParamNode>(nameTok.location, nameTok.lexeme, nameTok.location,
                                       std::move(type));
}

std::unique_ptr<TypeNode> Parser::parseReturnType() {
    if (check(TokenType::KW_VOID)) {
        const Token tok = advance();
        return std::make_unique<VoidTypeNode>(tok.location);
    }
    return parseType();
}

std::unique_ptr<TypeNode> Parser::parseType() {
    const Token& tok = peek();
    switch (tok.type) {
        case TokenType::KW_INTEGER:
            advance();
            return std::make_unique<AtomicTypeNode>(tok.location, AtomicType::Integer);
        case TokenType::KW_BOOLEAN:
            advance();
            return std::make_unique<AtomicTypeNode>(tok.location, AtomicType::Boolean);
        case TokenType::KW_CHAR:
            advance();
            return std::make_unique<AtomicTypeNode>(tok.location, AtomicType::Char);
        case TokenType::KW_STRING:
            advance();
            return std::make_unique<AtomicTypeNode>(tok.location, AtomicType::String);
        case TokenType::KW_ARRAY:
            return parseArrayType();
        default:
            reportError(tok.location, "expected type name, got " + describeGotToken(tok));
            return nullptr;
    }
}

std::unique_ptr<TypeNode> Parser::parseArrayType() {
    const Token arrTok = advance(); // Consume 'array'
    expect(TokenType::LBRACKET, "'['");

    std::unique_ptr<ExprNode> size;
    if (!check(TokenType::RBRACKET) && !isAtEnd()) {
        size = parseExpr();
    }
    expect(TokenType::RBRACKET, "']'");

    auto elemType = parseType();
    return std::make_unique<ArrayTypeNode>(arrTok.location, std::move(size), std::move(elemType));
}

std::unique_ptr<InitializerNode> Parser::parseInitializer() {
    if (match(TokenType::LBRACE)) {
        const SourceLocation braceLoc = previous().location;
        std::vector<std::unique_ptr<InitializerNode>> elements;
        if (!check(TokenType::RBRACE)) {
            elements.push_back(parseInitializer());
            while (match(TokenType::COMMA)) {
                elements.push_back(parseInitializer());
            }
        }
        expect(TokenType::RBRACE, "'}'");
        return std::make_unique<ArrayInitializer>(braceLoc, std::move(elements));
    }

    auto expr = parseExpr();
    return std::make_unique<ExprInitializer>(expr->location, std::move(expr));
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    const Token lbraceTok = peek();
    if (!expect(TokenType::LBRACE, "'{'")) {
        return std::make_unique<BlockStmt>(lbraceTok.location,
                                           std::vector<std::unique_ptr<StmtNode>>{});
    }

    std::vector<std::unique_ptr<StmtNode>> stmts;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        const std::size_t startPos = current_;
        auto stmt = parseStmt();
        if (stmt) {
            stmts.push_back(std::move(stmt));
        }
        if (current_ == startPos && !isAtEnd() && !check(TokenType::RBRACE)) {
            advance();
        }
        panicMode_ = false;
    }

    expect(TokenType::RBRACE, "'}'");
    return std::make_unique<BlockStmt>(lbraceTok.location, std::move(stmts));
}

std::unique_ptr<StmtNode> Parser::parseStmt() {
    switch (peek().type) {
        case TokenType::KW_IF:     return parseIfStmt();
        case TokenType::KW_FOR:    return parseForStmt();
        case TokenType::KW_RETURN: return parseReturnStmt();
        case TokenType::KW_PRINT:  return parsePrintStmt();
        case TokenType::LBRACE:    return parseBlock();
        case TokenType::IDENT:
            // LL(2) check distinguishes local variable declaration ("x: type") from expression stmt
            if (peek(1).type == TokenType::COLON) {
                return parseLocalVarDeclStmt();
            }
            return parseExprStmt();
        default:
            return parseExprStmt();
    }
}

std::unique_ptr<StmtNode> Parser::parseIfStmt() {
    const Token ifTok = advance(); // Consume 'if'
    expect(TokenType::LPAREN, "'('");
    auto cond = parseExpr();
    expectClosingParenBeforeBlockOrStmt();

    auto thenBranch = parseStmt();
    std::unique_ptr<StmtNode> elseBranch;
    if (match(TokenType::KW_ELSE)) {
        elseBranch = parseStmt();
    }

    return std::make_unique<IfStmt>(ifTok.location, std::move(cond),
                                    std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<StmtNode> Parser::parseForStmt() {
    const Token forTok = advance(); // Consume 'for'
    expect(TokenType::LPAREN, "'('");

    std::unique_ptr<ExprNode> init;
    if (!check(TokenType::SEMI)) {
        init = parseExpr();
    }
    if (!expect(TokenType::SEMI, "';'")) {
        while (!isAtEnd() && !check(TokenType::SEMI) && !check(TokenType::RPAREN) &&
               !check(TokenType::LBRACE)) {
            advance();
        }
        match(TokenType::SEMI);
        panicMode_ = false;
    }

    std::unique_ptr<ExprNode> cond;
    if (!check(TokenType::SEMI) && !check(TokenType::RPAREN) && !check(TokenType::LBRACE)) {
        cond = parseExpr();
    }
    if (!expect(TokenType::SEMI, "';'")) {
        while (!isAtEnd() && !check(TokenType::SEMI) && !check(TokenType::RPAREN) &&
               !check(TokenType::LBRACE)) {
            advance();
        }
        match(TokenType::SEMI);
        panicMode_ = false;
    }

    std::unique_ptr<ExprNode> update;
    if (!check(TokenType::RPAREN) && !check(TokenType::LBRACE) && !check(TokenType::SEMI)) {
        update = parseExpr();
    }
    expectClosingParenBeforeBlockOrStmt();

    auto body = parseStmt();
    return std::make_unique<ForStmt>(forTok.location, std::move(init), std::move(cond),
                                     std::move(update), std::move(body));
}

std::unique_ptr<StmtNode> Parser::parseReturnStmt() {
    const Token retTok = advance(); // Consume 'return'
    std::unique_ptr<ExprNode> value;
    if (!check(TokenType::SEMI) && !check(TokenType::RBRACE) && !isAtEnd()) {
        value = parseExpr();
    }
    expectStmtEnd();
    return std::make_unique<ReturnStmt>(retTok.location, std::move(value));
}

std::unique_ptr<StmtNode> Parser::parsePrintStmt() {
    const Token printTok = advance(); // Consume 'print'
    std::vector<std::unique_ptr<ExprNode>> exprs;
    if (!check(TokenType::SEMI) && !check(TokenType::RBRACE) && !isAtEnd()) {
        exprs = parseExprList();
    }
    expectStmtEnd();
    return std::make_unique<PrintStmt>(printTok.location, std::move(exprs));
}

std::unique_ptr<StmtNode> Parser::parseLocalVarDeclStmt() {
    const Token nameTok = advance(); // Consume IDENT
    expect(TokenType::COLON, "':'");
    auto decl = parseVarTail(nameTok, /*isGlobal=*/false);
    const SourceLocation loc = decl->location;
    return std::make_unique<VarDeclStmt>(loc, std::move(decl));
}

std::unique_ptr<StmtNode> Parser::parseExprStmt() {
    const SourceLocation startLoc = peek().location;
    if (match(TokenType::SEMI)) {
        // Unexpected stray semicolon as statement
        reportError(startLoc, "expected statement, got ';'");
        panicMode_ = false;
        return std::make_unique<ErrorStmt>(startLoc);
    }

    auto expr = parseExpr();
    const bool hadError = panicMode_;
    expectStmtEnd();

    if (hadError && dynamic_cast<const ErrorExpr*>(expr.get()) != nullptr) {
        return std::make_unique<ErrorStmt>(startLoc);
    }
    return std::make_unique<ExprStmt>(expr->location, std::move(expr));
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

std::vector<std::unique_ptr<ExprNode>> Parser::parseExprList() {
    std::vector<std::unique_ptr<ExprNode>> exprs;
    exprs.push_back(parseExpr());
    while (match(TokenType::COMMA)) {
        exprs.push_back(parseExpr());
    }
    return exprs;
}

template <typename NextLevelFn>
std::unique_ptr<ExprNode> Parser::parseBinaryLeft(NextLevelFn nextLevel,
                                                  std::initializer_list<TokenType> ops) {
    auto expr = (this->*nextLevel)();
    while (true) {
        bool matched = false;
        Token opTok;
        for (const TokenType opType : ops) {
            if (check(opType)) {
                opTok = advance();
                matched = true;
                break;
            }
        }
        if (!matched) {
            break;
        }
        auto rhs = (this->*nextLevel)();
        const SourceLocation loc = expr->location;
        expr = std::make_unique<BinaryExpr>(loc, opTok.lexeme, std::move(expr), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseExpr() {
    return parseAssign();
}

std::unique_ptr<ExprNode> Parser::parseAssign() {
    auto lhs = parseLogicalOr();
    if (match(TokenType::OP_ASSIGN)) {
        auto rhs = parseAssign();
        const SourceLocation loc = lhs->location;
        return std::make_unique<AssignExpr>(loc, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<ExprNode> Parser::parseLogicalOr() {
    return parseBinaryLeft(&Parser::parseLogicalAnd, {TokenType::OP_OR});
}

std::unique_ptr<ExprNode> Parser::parseLogicalAnd() {
    return parseBinaryLeft(&Parser::parseComparison, {TokenType::OP_AND});
}

std::unique_ptr<ExprNode> Parser::parseComparison() {
    return parseBinaryLeft(&Parser::parseAdditive, {
        TokenType::OP_LT,
        TokenType::OP_LE,
        TokenType::OP_GT,
        TokenType::OP_GE,
        TokenType::OP_EQ,
        TokenType::OP_NEQ
    });
}

std::unique_ptr<ExprNode> Parser::parseAdditive() {
    return parseBinaryLeft(&Parser::parseMultiplicative, {TokenType::OP_PLUS, TokenType::OP_MINUS});
}

std::unique_ptr<ExprNode> Parser::parseMultiplicative() {
    return parseBinaryLeft(&Parser::parsePower,
                           {TokenType::OP_STAR, TokenType::OP_SLASH, TokenType::OP_PERCENT});
}

std::unique_ptr<ExprNode> Parser::parsePower() {
    auto base = parseUnary();
    if (match(TokenType::OP_CARET)) {
        auto exponent = parsePower();
        const SourceLocation loc = base->location;
        return std::make_unique<BinaryExpr>(loc, "^", std::move(base), std::move(exponent));
    }
    return base;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
    if (check(TokenType::OP_MINUS) || check(TokenType::OP_NOT)) {
        const Token opTok = advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(opTok.location, opTok.lexeme, std::move(operand));
    }
    return parsePostfix();
}

std::unique_ptr<ExprNode> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::LBRACKET)) {
            auto index = parseExpr();
            expect(TokenType::RBRACKET, "']'");
            const SourceLocation loc = expr->location;
            expr = std::make_unique<ArrayIndexExpr>(loc, std::move(expr), std::move(index));
        } else if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<ExprNode>> args;
            if (!check(TokenType::RPAREN) && !check(TokenType::SEMI) && !isAtEnd()) {
                args = parseExprList();
            }
            expect(TokenType::RPAREN, "')'");
            const SourceLocation loc = expr->location;
            expr = std::make_unique<CallExpr>(loc, std::move(expr), std::move(args));
        } else if (check(TokenType::OP_INC) || check(TokenType::OP_DEC)) {
            const Token opTok = advance();
            const SourceLocation loc = expr->location;
            expr = std::make_unique<PostfixExpr>(loc, opTok.lexeme, std::move(expr));
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    const Token& tok = peek();

    switch (tok.type) {
        case TokenType::IDENT: {
            const Token identTok = advance();
            return std::make_unique<IdentifierExpr>(identTok.location, identTok.lexeme);
        }
        case TokenType::INT_LITERAL:
            return buildIntLiteral(advance());
        case TokenType::BOOL_LITERAL: {
            const Token boolTok = advance();
            return std::make_unique<BoolLiteralExpr>(boolTok.location, boolTok.lexeme == "true");
        }
        case TokenType::CHAR_LITERAL:
            return buildCharLiteral(advance());
        case TokenType::STRING_LITERAL:
            return buildStringLiteral(advance());
        case TokenType::LPAREN: {
            advance(); // Consume '('
            auto inner = parseExpr();
            expect(TokenType::RPAREN, "')'");
            return inner;
        }
        default:
            reportError(tok.location, "expected expression, got " + describeGotToken(tok));
            return std::make_unique<ErrorExpr>(tok.location);
    }
}

std::unique_ptr<ExprNode> Parser::buildIntLiteral(const Token& tok) {
    long long val = 0;
    bool overflowed = false;
    try {
        std::size_t pos = 0;
        val = std::stoll(tok.lexeme, &pos, 10);
        if (pos != tok.lexeme.size()) {
            overflowed = true;
        }
    } catch (const std::out_of_range&) {
        overflowed = true;
        val = std::numeric_limits<long long>::max();
    } catch (const std::invalid_argument&) {
        overflowed = true;
    }
    return std::make_unique<IntLiteralExpr>(tok.location, tok.lexeme, val, overflowed);
}

std::unique_ptr<ExprNode> Parser::buildCharLiteral(const Token& tok) {
    const std::string unescaped = unescapeContent(tok.lexeme);
    const char ch = unescaped.empty() ? '\0' : unescaped.front();
    return std::make_unique<CharLiteralExpr>(tok.location, tok.lexeme, ch);
}

std::unique_ptr<ExprNode> Parser::buildStringLiteral(const Token& tok) {
    return std::make_unique<StringLiteralExpr>(tok.location, tok.lexeme, unescapeContent(tok.lexeme));
}

} // namespace bminor
