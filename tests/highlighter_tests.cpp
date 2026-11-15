#include "highlighter/highlighter.hpp"
#include "lexer/lexer.hpp"
#include "output/formatter.hpp"
#include "parser/parser.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace bminor;

namespace {

std::vector<HighlightedToken> highlightSource(const std::string& src) {
    Lexer lexer(src, "test.bm");
    LexResult lexRes = lexer.tokenize();
    Parser parser(lexRes.parserTokens, "test.bm");
    ParseResult parseRes = parser.parse();

    std::vector<Diagnostic> diags = lexRes.diagnostics;
    diags.insert(diags.end(), parseRes.diagnostics.begin(), parseRes.diagnostics.end());

    SyntaxHighlighter highlighter;
    return highlighter.highlight(lexRes.allTokens, parseRes.program.get(), diags);
}

std::string stripAnsiCodes(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\x1b' && i + 1 < input.size() && input[i + 1] == '[') {
            i += 2;
            while (i < input.size() && input[i] != 'm') {
                ++i;
            }
            continue;
        }
        out.push_back(input[i]);
    }
    return out;
}

const HighlightedToken* findToken(const std::vector<HighlightedToken>& tokens,
                                  const std::string& lexeme,
                                  std::size_t occurrence = 0) {
    std::size_t seen = 0;
    for (const auto& tok : tokens) {
        if (tok.lexeme == lexeme) {
            if (seen == occurrence) {
                return &tok;
            }
            ++seen;
        }
    }
    return nullptr;
}

void testAstDistinguishesFunctionsFromVariables() {
    // Same identifier name "calc" appears as function decl, function call, local variable, and var ref
    const std::string src =
        "calc: function integer ( v: integer ) = {\n"
        "    return v + 1;\n"
        "}\n"
        "main: function integer () = {\n"
        "    res: integer = calc(5);\n"
        "    {\n"
        "        calc: integer = 10;\n"
        "        print calc, res;\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    const auto tokens = highlightSource(src);

    const auto* fnDeclTok = findToken(tokens, "calc", 0);
    const auto* fnCallTok = findToken(tokens, "calc", 1);
    const auto* localVarDeclTok = findToken(tokens, "calc", 2);
    const auto* localVarUseTok = findToken(tokens, "calc", 3);

    assert(fnDeclTok != nullptr && fnDeclTok->category == HighlightCategory::FuncIdentifier);
    assert(fnCallTok != nullptr && fnCallTok->category == HighlightCategory::FuncIdentifier);
    assert(localVarDeclTok != nullptr && localVarDeclTok->category == HighlightCategory::VarIdentifier);
    assert(localVarUseTok != nullptr && localVarUseTok->category == HighlightCategory::VarIdentifier);

    const auto* paramTok = findToken(tokens, "v", 0);
    assert(paramTok != nullptr && paramTok->category == HighlightCategory::VarIdentifier);
}

void testAllTableOneCategoriesAndAnsiRoundTrip() {
    const std::string src =
        "// line comment\n"
        "/* block comment */\n"
        "grid: array [2] integer = { 1, 2 };\n"
        "say: function void ( s: string ) = {\n"
        "    ok: boolean = true;\n"
        "    ch: char = 'a';\n"
        "    if (ok && grid[0] <= 2) {\n"
        "        print \"hi\", ch;\n"
        "    }\n"
        "}\n";

    const auto tokens = highlightSource(src);

    assert(findToken(tokens, "// line comment")->category == HighlightCategory::Comment);
    assert(findToken(tokens, "/* block comment */")->category == HighlightCategory::Comment);
    assert(findToken(tokens, "array")->category == HighlightCategory::Keyword);
    assert(findToken(tokens, "function")->category == HighlightCategory::Keyword);
    assert(findToken(tokens, "if")->category == HighlightCategory::Keyword);
    assert(findToken(tokens, "print")->category == HighlightCategory::Keyword);
    assert(findToken(tokens, "integer")->category == HighlightCategory::TypeName);
    assert(findToken(tokens, "void")->category == HighlightCategory::TypeName);
    assert(findToken(tokens, "string")->category == HighlightCategory::TypeName);
    assert(findToken(tokens, "boolean")->category == HighlightCategory::TypeName);
    assert(findToken(tokens, "char")->category == HighlightCategory::TypeName);
    assert(findToken(tokens, "say")->category == HighlightCategory::FuncIdentifier);
    assert(findToken(tokens, "grid")->category == HighlightCategory::VarIdentifier);
    assert(findToken(tokens, "2", 0)->category == HighlightCategory::IntLiteral);
    assert(findToken(tokens, "true")->category == HighlightCategory::BoolLiteral);
    assert(findToken(tokens, "'a'")->category == HighlightCategory::StringOrCharLiteral);
    assert(findToken(tokens, "\"hi\"")->category == HighlightCategory::StringOrCharLiteral);
    assert(findToken(tokens, "<=")->category == HighlightCategory::Operator);
    assert(findToken(tokens, "&&")->category == HighlightCategory::Operator);

    const std::string ansi = formatAnsi(tokens);
    assert(ansi.find("\x1b[1;34mfunction\x1b[0m") != std::string::npos);
    assert(ansi.find("\x1b[36minteger\x1b[0m") != std::string::npos);
    assert(ansi.find("\x1b[93msay\x1b[0m") != std::string::npos);
    assert(ansi.find("\x1b[38;5;208mtrue\x1b[0m") != std::string::npos);
    assert(ansi.find("\x1b[2;3;90m// line comment\x1b[0m") != std::string::npos);

    // Stripping ANSI codes must reconstruct the original source character-for-character
    assert(stripAnsiCodes(ansi) == src);
}

void testHtmlEscapingCssAndNoJavaScript() {
    const std::string src =
        "check: function boolean ( a: integer, b: integer ) = {\n"
        "    if (a < b && b > 0) {\n"
        "        print \"<ok & 'yes'>\";\n"
        "        return true;\n"
        "    }\n"
        "    return false;\n"
        "}\n";

    const auto tokens = highlightSource(src);
    const std::string html = formatHtml(tokens, "check.bminor");

    assert(html.find("<!DOCTYPE html>") != std::string::npos);
    assert(html.find("<style>") != std::string::npos);
    assert(html.find("<pre class=\"bminor-code\">") != std::string::npos);
    assert(html.find("<span class=\"kw\">if</span>") != std::string::npos);
    assert(html.find("<span class=\"lit\">true</span>") != std::string::npos);
    assert(html.find("<span class=\"fn\">check</span>") != std::string::npos);
    assert(html.find("<span class=\"type\">boolean</span>") != std::string::npos);
    assert(html.find("<span class=\"var\">a</span>") != std::string::npos);
    assert(html.find("<span class=\"op\">&lt;</span>") != std::string::npos);
    assert(html.find("<span class=\"op\">&amp;&amp;</span>") != std::string::npos);
    assert(html.find("<span class=\"op\">&gt;</span>") != std::string::npos);
    assert(html.find("&quot;&lt;ok &amp; &#39;yes&#39;&gt;&quot;") != std::string::npos);
    assert(html.find("<script") == std::string::npos);
}

void testErrorTokenHighlightingInAnsiAndHtml() {
    const std::string src =
        "x: integer = 5@;\n"
        "main: function integer () = {\n"
        "    y: integer = ;\n"
        "    return 0;\n"
        "}\n";

    const auto tokens = highlightSource(src);
    const auto* badLex = findToken(tokens, "@");
    const auto* badSyn = findToken(tokens, ";", 1);
    assert(badLex != nullptr && badLex->category == HighlightCategory::Error);
    assert(badSyn != nullptr && badSyn->category == HighlightCategory::Error);

    const std::string ansi = formatAnsi(tokens);
    assert(ansi.find("\x1b[4;91m@\x1b[0m") != std::string::npos);
    assert(stripAnsiCodes(ansi) == src);

    const std::string html = formatHtml(tokens, "errors.bminor");
    assert(html.find("<span class=\"err\">@</span>") != std::string::npos);
    assert(html.find("<span class=\"err\">;</span>") != std::string::npos);
}

} // namespace

int main() {
    testAstDistinguishesFunctionsFromVariables();
    testAllTableOneCategoriesAndAnsiRoundTrip();
    testHtmlEscapingCssAndNoJavaScript();
    testErrorTokenHighlightingInAnsiAndHtml();

    std::cout << "All Syntax Highlighter & Output Formatter unit tests passed successfully.\n";
    return 0;
}
