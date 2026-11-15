#include "lexer/lexer.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string joinParserTokens(const std::vector<bminor::Token>& tokens) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            oss << ' ';
        }
        oss << bminor::formatToken(tokens[i]);
    }
    return oss.str();
}

std::string reconstructSource(const std::vector<bminor::Token>& allTokens) {
    std::string out;
    for (const auto& tok : allTokens) {
        out += tok.lexeme;
    }
    return out;
}

void testFactorialSnippetFromSpec() {
    const std::string src =
        "factorial: function integer ( n: integer ) = {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * factorial(n - 1);\n"
        "}\n";

    bminor::Lexer lexer(src, "factorial.bminor");
    bminor::LexResult res = lexer.tokenize();

    assert(res.diagnostics.empty());
    const std::string expected =
        "IDENT(factorial) COLON KW(function) KW(integer) LPAREN IDENT(n) COLON "
        "KW(integer) RPAREN ASSIGN LBRACE KW(if) LPAREN IDENT(n) OP(<=) INT(1) "
        "RPAREN KW(return) INT(1) SEMI KW(return) IDENT(n) OP(*) IDENT(factorial) "
        "LPAREN IDENT(n) OP(-) INT(1) RPAREN SEMI RBRACE EOF";
    assert(joinParserTokens(res.parserTokens) == expected);
    assert(reconstructSource(res.allTokens) == src);

    // Check 1-based line:column locations matching AST example positions on line 3.
    bool sawReturnN = false;
    for (const auto& tok : res.parserTokens) {
        if (tok.lexeme == "n" && tok.location.line == 3 && tok.location.column == 12) {
            sawReturnN = true;
        }
    }
    assert(sawReturnN);
}

void testAllKeywordsAndPriority() {
    const std::string src =
        "array boolean char else false for function if integer "
        "print return string true void while "
        "return1 _if while_loop trueValue falseFlag";

    bminor::Lexer lexer(src);
    bminor::LexResult res = lexer.tokenize();

    assert(res.diagnostics.empty());
    const std::vector<bminor::TokenType> expectedTypes = {
        bminor::TokenType::KW_ARRAY,
        bminor::TokenType::KW_BOOLEAN,
        bminor::TokenType::KW_CHAR,
        bminor::TokenType::KW_ELSE,
        bminor::TokenType::BOOL_LITERAL,
        bminor::TokenType::KW_FOR,
        bminor::TokenType::KW_FUNCTION,
        bminor::TokenType::KW_IF,
        bminor::TokenType::KW_INTEGER,
        bminor::TokenType::KW_PRINT,
        bminor::TokenType::KW_RETURN,
        bminor::TokenType::KW_STRING,
        bminor::TokenType::BOOL_LITERAL,
        bminor::TokenType::KW_VOID,
        bminor::TokenType::KW_WHILE,
        bminor::TokenType::IDENT,
        bminor::TokenType::IDENT,
        bminor::TokenType::IDENT,
        bminor::TokenType::IDENT,
        bminor::TokenType::IDENT,
        bminor::TokenType::END_OF_FILE
    };

    assert(res.parserTokens.size() == expectedTypes.size());
    for (std::size_t i = 0; i < expectedTypes.size(); ++i) {
        assert(res.parserTokens[i].type == expectedTypes[i]);
    }
    assert(bminor::isTypeKeyword(bminor::TokenType::KW_INTEGER));
    assert(bminor::isTypeKeyword(bminor::TokenType::KW_BOOLEAN));
    assert(bminor::isTypeKeyword(bminor::TokenType::KW_CHAR));
    assert(bminor::isTypeKeyword(bminor::TokenType::KW_STRING));
    assert(!bminor::isTypeKeyword(bminor::TokenType::KW_IF));
}

void testOperatorsDelimitersAndMaximalMunch() {
    const std::string src = "a<=b>=c==d!=e&&f||g++h--i+j-k*l/m%n^!o=p ( ) { } [ ] : ; ,";
    bminor::Lexer lexer(src);
    bminor::LexResult res = lexer.tokenize();

    assert(res.diagnostics.empty());
    const std::string expected =
        "IDENT(a) OP(<=) IDENT(b) OP(>=) IDENT(c) OP(==) IDENT(d) OP(!=) IDENT(e) "
        "OP(&&) IDENT(f) OP(||) IDENT(g) OP(++) IDENT(h) OP(--) IDENT(i) OP(+) "
        "IDENT(j) OP(-) IDENT(k) OP(*) IDENT(l) OP(/) IDENT(m) OP(%) IDENT(n) "
        "OP(^) OP(!) IDENT(o) ASSIGN IDENT(p) "
        "LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET COLON SEMI COMMA EOF";
    assert(joinParserTokens(res.parserTokens) == expected);

    // Maximal munch: x+++y -> x ++ + y
    bminor::Lexer mmLexer("x+++y");
    bminor::LexResult mmRes = mmLexer.tokenize();
    assert(mmRes.diagnostics.empty());
    assert(joinParserTokens(mmRes.parserTokens) == "IDENT(x) OP(++) OP(+) IDENT(y) EOF");
}

void testStringsCharsCommentsAndLineTracking() {
    const std::string src =
        "// line comment\r\n"
        "s: string = \"hello\\n\\0\\\"world\\\\\"; /* block\n"
        "comment */ c: char = '\\n'; q: char = '\\'';\n";

    bminor::Lexer lexer(src, "test.bminor");
    bminor::LexResult res = lexer.tokenize();

    assert(res.diagnostics.empty());
    assert(reconstructSource(res.allTokens) == src);

    // Check tokens on line 2 & line 3
    assert(res.parserTokens[0].type == bminor::TokenType::IDENT);
    assert(res.parserTokens[0].location.line == 2);
    assert(res.parserTokens[0].location.column == 1);
    assert(res.parserTokens[4].type == bminor::TokenType::STRING_LITERAL);

    bool foundCharQ = false;
    for (const auto& tok : res.parserTokens) {
        if (tok.lexeme == "q") {
            foundCharQ = true;
            assert(tok.location.line == 3);
        }
    }
    assert(foundCharQ);
}

void testLengthLimits256() {
    const std::string okIdent(256, 'a');
    const std::string tooLongIdent(257, 'b');
    const std::string okStr = "\"" + std::string(256, 'x') + "\"";
    const std::string tooLongStr = "\"" + std::string(257, 'y') + "\"";

    bminor::Lexer lexer(okIdent + " " + tooLongIdent + " " + okStr + " " + tooLongStr);
    bminor::LexResult res = lexer.tokenize();

    assert(res.parserTokens.size() == 5); // okIdent, INVALID, okStr, INVALID, EOF
    assert(res.parserTokens[0].type == bminor::TokenType::IDENT);
    assert(res.parserTokens[1].type == bminor::TokenType::INVALID);
    assert(res.parserTokens[2].type == bminor::TokenType::STRING_LITERAL);
    assert(res.parserTokens[3].type == bminor::TokenType::INVALID);
    assert(res.diagnostics.size() == 2);
}

void testErrorRecovery() {
    // 1. Unknown character recovery matching p.9 example (1:15)
    {
        const std::string src =
            "x: integer = 5@;\n"
            "y: integer = 10;\n";
        bminor::Lexer lexer(src, "err.bminor");
        bminor::LexResult res = lexer.tokenize();

        assert(res.diagnostics.size() == 1);
        assert(res.diagnostics[0].location.line == 1);
        assert(res.diagnostics[0].location.column == 15);
        assert(reconstructSource(res.allTokens) == src);

        bool sawInvalidAt = false;
        bool sawInt10 = false;
        for (const auto& tok : res.parserTokens) {
            if (tok.type == bminor::TokenType::INVALID && tok.lexeme == "@" &&
                tok.location.line == 1 && tok.location.column == 15) {
                sawInvalidAt = true;
            }
            if (tok.type == bminor::TokenType::INT_LITERAL && tok.lexeme == "10" &&
                tok.location.line == 2) {
                sawInt10 = true;
            }
        }
        assert(sawInvalidAt && sawInt10);
    }

    // 2. Unterminated string literal, invalid char literals, single '&'/'|'
    {
        const std::string src =
            "s: string = \"unterminated\n"
            "a: char = '';\n"
            "b: char = 'ab';\n"
            "c: boolean = x & y | z;\n";
        bminor::Lexer lexer(src);
        bminor::LexResult res = lexer.tokenize();

        assert(res.diagnostics.size() == 5);
        assert(reconstructSource(res.allTokens) == src);
        // Ensure scanning continued through line 4 identifier z.
        bool sawZ = false;
        for (const auto& tok : res.parserTokens) {
            if (tok.type == bminor::TokenType::IDENT && tok.lexeme == "z" && tok.location.line == 4) {
                sawZ = true;
            }
        }
        assert(sawZ);
    }

    // 3. Unterminated block comment
    {
        const std::string src = "x: integer = 1; /* never closed";
        bminor::Lexer lexer(src);
        bminor::LexResult res = lexer.tokenize();

        assert(res.diagnostics.size() == 1);
        assert(res.diagnostics[0].location.line == 1);
        assert(res.diagnostics[0].location.column == 17);
        assert(res.parserTokens.back().type == bminor::TokenType::END_OF_FILE);
        assert(reconstructSource(res.allTokens) == src);
    }
}

} // namespace

int main() {
    testFactorialSnippetFromSpec();
    testAllKeywordsAndPriority();
    testOperatorsDelimitersAndMaximalMunch();
    testStringsCharsCommentsAndLineTracking();
    testLengthLimits256();
    testErrorRecovery();

    std::cout << "All Lexer unit tests passed successfully.\n";
    return 0;
}
