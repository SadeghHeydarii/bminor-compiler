#ifndef BMINOR_COMMON_DIAGNOSTIC_HPP
#define BMINOR_COMMON_DIAGNOSTIC_HPP

#include "common/source_location.hpp"

#include <string>

namespace bminor {

enum class DiagnosticStage {
    Lexical,
    Syntactic
};

/// Records a compiler diagnostic message with its stage and exact source location.
struct Diagnostic {
    DiagnosticStage stage = DiagnosticStage::Lexical;
    SourceLocation location;
    std::string message;

    std::string toString() const {
        const std::string prefix = (stage == DiagnosticStage::Lexical) ? "Lexical error" : "Syntax error";
        return prefix + " at " + location.toString() + ": " + message;
    }
};

} // namespace bminor

#endif // BMINOR_COMMON_DIAGNOSTIC_HPP
