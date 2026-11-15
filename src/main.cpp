#include "lexer/lexer.hpp"

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

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "B-Minor Compiler Front-End\n"
                  << "Usage: bminor --lex <source-file>\n";
        return 0;
    }

    const std::string arg1 = argv[1];
    std::string filePath;
    bool lexOnly = false;

    if (arg1 == "--lex" && argc >= 3) {
        lexOnly = true;
        filePath = argv[2];
    } else {
        lexOnly = true;
        filePath = arg1;
    }

    std::string source;
    if (!readFileToString(filePath, source)) {
        std::cerr << "Error: could not open input file '" << filePath << "'\n";
        return 1;
    }

    bminor::Lexer lexer(source, filePath);
    bminor::LexResult lexResult = lexer.tokenize();

    if (lexOnly) {
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

    return lexResult.diagnostics.empty() ? 0 : 1;
}
