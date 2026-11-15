#include "ast/ast.hpp"
#include "highlighter/highlighter.hpp"
#include "lexer/lexer.hpp"
#include "output/formatter.hpp"
#include "parser/parser.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace bminor;

namespace {

std::string loadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in.good());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string stripAnsi(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\x1b' && i + 1 < text.size() && text[i + 1] == '[') {
            i += 2;
            while (i < text.size() && text[i] != 'm') {
                ++i;
            }
            continue;
        }
        out.push_back(text[i]);
    }
    return out;
}

struct PipelineOutput {
    LexResult lex;
    ParseResult parse;
    std::vector<Diagnostic> allDiags;
    std::vector<HighlightedToken> highlighted;
    std::string ansi;
    std::string html;
};

PipelineOutput runFullPipeline(const std::string& source, const std::string& filename) {
    PipelineOutput out;
    Lexer lexer(source, filename);
    out.lex = lexer.tokenize();

    Parser parser(out.lex.parserTokens, filename);
    out.parse = parser.parse();

    out.allDiags = out.lex.diagnostics;
    out.allDiags.insert(out.allDiags.end(),
                        out.parse.diagnostics.begin(),
                        out.parse.diagnostics.end());

    SyntaxHighlighter highlighter;
    out.highlighted = highlighter.highlight(out.lex.allTokens, out.parse.program.get(), out.allDiags);
    out.ansi = formatAnsi(out.highlighted);
    out.html = formatHtml(out.highlighted, filename);
    return out;
}

void testValidFixtureEndToEnd(const std::string& path) {
    const std::string source = loadFile(path);
    const PipelineOutput out = runFullPipeline(source, path);

    assert(out.lex.diagnostics.empty());
    assert(!out.parse.hasErrors());
    assert(out.parse.program != nullptr);
    assert(!out.parse.program->declarations.empty());
    assert(stripAnsi(out.ansi) == source);
    assert(out.html.find("<!DOCTYPE html>") != std::string::npos);
    assert(out.html.find("<pre class=\"bminor-code\">") != std::string::npos);
    assert(out.html.find("<script") == std::string::npos);
}

void testInvalidFixtureResilience(const std::string& path,
                                   bool expectLexErrors,
                                   bool expectSyntaxErrors) {
    const std::string source = loadFile(path);
    const PipelineOutput out = runFullPipeline(source, path);

    assert(out.parse.program != nullptr);
    if (expectLexErrors) {
        assert(!out.lex.diagnostics.empty());
    }
    if (expectSyntaxErrors) {
        assert(!out.parse.diagnostics.empty());
    }
    assert(stripAnsi(out.ansi) == source);
    assert(out.html.find("<span class=\"err\">") != std::string::npos);
    assert(out.html.find("<script") == std::string::npos);
}

void testGeneralNonHardcodedArbitraryProgram() {
    // Dynamically constructed identifiers & nesting prove Lexer/Parser/Highlighter are general-purpose.
    std::ostringstream src;
    for (int i = 0; i < 6; ++i) {
        src << "fn_" << i << ": function integer ( arg_" << i << ": integer ) = {\n"
            << "    tmp_" << i << ": integer = arg_" << i << " * " << (i + 2) << ";\n"
            << "    return tmp_" << i << ";\n"
            << "}\n";
    }
    src << "entry: function integer () = {\n"
        << "    acc: integer = 0;\n";
    for (int i = 0; i < 6; ++i) {
        src << "    acc = acc + fn_" << i << "(" << i << ");\n";
    }
    src << "    return acc;\n}\n";

    const PipelineOutput out = runFullPipeline(src.str(), "generated.bminor");
    assert(out.allDiags.empty());
    assert(out.parse.program != nullptr);
    assert(out.parse.program->declarations.size() == 7);
    assert(stripAnsi(out.ansi) == src.str());
    assert(out.html.find("<span class=\"fn\">fn_5</span>") != std::string::npos);
    assert(out.html.find("<span class=\"var\">acc</span>") != std::string::npos);
}

} // namespace

int main() {
    testValidFixtureEndToEnd("tests/valid/factorial.bminor");
    testValidFixtureEndToEnd("tests/valid/language_features.bminor");

    testInvalidFixtureResilience("tests/invalid/lexer_errors.bminor", true, true);
    testInvalidFixtureResilience("tests/invalid/syntax_errors.bminor", false, true);
    testInvalidFixtureResilience("tests/invalid/combined_errors.bminor", true, true);

    // Ensure combined_errors.bminor still recovered the trailing valid function "safe_after".
    {
        const PipelineOutput combined = runFullPipeline(
            loadFile("tests/invalid/combined_errors.bminor"),
            "tests/invalid/combined_errors.bminor");
        bool foundSafeAfter = false;
        for (const auto& decl : combined.parse.program->declarations) {
            if (const auto* fn = dynamic_cast<const FuncDecl*>(decl.get())) {
                if (fn->name == "safe_after") {
                    foundSafeAfter = true;
                }
            }
        }
        assert(foundSafeAfter);
    }

    testGeneralNonHardcodedArbitraryProgram();

    std::cout << "All end-to-end integration tests passed successfully.\n";
    return 0;
}
