#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

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

void printUsage() {
    std::cout << "B-Minor Compiler Front-End\n"
              << "Usage:\n"
              << "  bminor --lex <source-file>     Tokenize input and print token stream\n"
              << "  bminor --parse <source-file>   Parse input and print AST\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    const std::string arg1 = argv[1];
    std::string filePath;
    bool modeLex = true;
    bool modeParse = false;

    if (arg1 == "--help" || arg1 == "-h") {
        printUsage();
        return 0;
    } else if (arg1 == "--lex" && argc >= 3) {
        modeLex = true;
        filePath = argv[2];
    } else if ((arg1 == "--parse" || arg1 == "--ast") && argc >= 3) {
        modeLex = false;
        modeParse = true;
        filePath = argv[2];
    } else {
        filePath = arg1;
    }

    std::string source;
    if (!readFileToString(filePath, source)) {
        std::cerr << "Error: could not open input file '" << filePath << "'\n";
        return 1;
    }

    bminor::Lexer lexer(source, filePath);
    bminor::LexResult lexResult = lexer.tokenize();

    if (modeLex) {
        for (std::size_t i = 0; i < lexResult.parserTokens.size(); ++i) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << bminor::formatToken(lexResult.parserTokens[i]);
        }
        std::cout << '\n';
    }

    for (const auto& diag : lexResult.diagnostics) {
        std::cerr << diag.toString() << '\n';
    }

    if (modeParse) {
        bminor::Parser parser(lexResult.parserTokens, filePath);
        bminor::ParseResult parseResult = parser.parse();
        if (parseResult.program) {
            std::cout << bminor::dumpAst(*parseResult.program);
        }
        for (const auto& diag : parseResult.diagnostics) {
            std::cerr << diag.toString() << '\n';
        }
        return (lexResult.diagnostics.empty() && !parseResult.hasErrors()) ? 0 : 1;
    }

    return lexResult.diagnostics.empty() ? 0 : 1;
}
