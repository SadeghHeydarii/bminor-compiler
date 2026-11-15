#include "ast/ast.hpp"
#include "highlighter/highlighter.hpp"
#include "lexer/lexer.hpp"
#include "output/formatter.hpp"
#include "parser/parser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class CliMode {
    Lex,
    Parse,
    HighlightAnsi,
    HighlightHtml
};

bool readFileToString(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

bool writeStringToFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << content;
    return out.good();
}

void printUsage() {
    std::cout << "B-Minor Compiler Front-End\n"
              << "Usage:\n"
              << "  bminor [--ansi | --highlight-ansi] <source-file>\n"
              << "  bminor [--html | --highlight-html] <source-file> [-o <output.html>]\n"
              << "  bminor --lex <source-file>\n"
              << "  bminor --parse <source-file>\n";
}

void printDiagnostics(const std::vector<bminor::Diagnostic>& diagnostics) {
    for (const auto& diag : diagnostics) {
        std::cerr << diag.toString() << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    CliMode mode = CliMode::HighlightAnsi;
    std::string filePath;
    std::string outputPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--lex") {
            mode = CliMode::Lex;
        } else if (arg == "--parse" || arg == "--ast") {
            mode = CliMode::Parse;
        } else if (arg == "--ansi" || arg == "--highlight-ansi") {
            mode = CliMode::HighlightAnsi;
        } else if (arg == "--html" || arg == "--highlight-html") {
            mode = CliMode::HighlightHtml;
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Error: unknown option '" << arg << "'\n";
            return 1;
        } else {
            filePath = arg;
        }
    }

    if (filePath.empty()) {
        std::cerr << "Error: missing input source file.\n";
        printUsage();
        return 1;
    }

    std::string source;
    if (!readFileToString(filePath, source)) {
        std::cerr << "Error: could not open input file '" << filePath << "'\n";
        return 1;
    }

    bminor::Lexer lexer(source, filePath);
    bminor::LexResult lexResult = lexer.tokenize();

    if (mode == CliMode::Lex) {
        for (std::size_t i = 0; i < lexResult.parserTokens.size(); ++i) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << bminor::formatToken(lexResult.parserTokens[i]);
        }
        std::cout << '\n';
        printDiagnostics(lexResult.diagnostics);
        return lexResult.diagnostics.empty() ? 0 : 1;
    }

    bminor::Parser parser(lexResult.parserTokens, filePath);
    bminor::ParseResult parseResult = parser.parse();

    if (mode == CliMode::Parse) {
        if (parseResult.program) {
            std::cout << bminor::dumpAst(*parseResult.program);
        }
        printDiagnostics(lexResult.diagnostics);
        printDiagnostics(parseResult.diagnostics);
        return (lexResult.diagnostics.empty() && !parseResult.hasErrors()) ? 0 : 1;
    }

    std::vector<bminor::Diagnostic> allDiagnostics = lexResult.diagnostics;
    allDiagnostics.insert(allDiagnostics.end(),
                          parseResult.diagnostics.begin(),
                          parseResult.diagnostics.end());

    bminor::SyntaxHighlighter highlighter;
    const auto highlighted = highlighter.highlight(
        lexResult.allTokens, parseResult.program.get(), allDiagnostics);

    if (mode == CliMode::HighlightAnsi) {
        std::cout << bminor::formatAnsi(highlighted);
        if (!source.empty() && source.back() != '\n') {
            std::cout << '\n';
        }
    } else if (mode == CliMode::HighlightHtml) {
        const std::string html = bminor::formatHtml(highlighted, filePath);
        if (!outputPath.empty()) {
            if (!writeStringToFile(outputPath, html)) {
                std::cerr << "Error: could not write output file '" << outputPath << "'\n";
                return 1;
            }
        } else {
            std::cout << html;
        }
    }

    printDiagnostics(allDiagnostics);
    return allDiagnostics.empty() ? 0 : 1;
}
