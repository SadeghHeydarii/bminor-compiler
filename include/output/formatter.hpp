#ifndef BMINOR_OUTPUT_FORMATTER_HPP
#define BMINOR_OUTPUT_FORMATTER_HPP

#include "highlighter/highlighter.hpp"

#include <string>
#include <vector>

namespace bminor {

/// Returns the ANSI SGR prefix sequence for a highlighting category (empty for default/plain).
std::string ansiPrefixForCategory(HighlightCategory category);

/// Returns the HTML CSS class name for a highlighting category (empty for unstyled text).
std::string htmlClassForCategory(HighlightCategory category);

/// Escapes special HTML characters (&, <, >, ", ') for safe rendering inside HTML/CSS output.
std::string escapeHtml(const std::string& text);

/// Renders highlighted tokens into an ANSI-colored string preserving exact source layout.
std::string formatAnsi(const std::vector<HighlightedToken>& tokens);

/// Renders highlighted tokens into a standalone HTML5/CSS document containing a <pre> block (no JS).
std::string formatHtml(const std::vector<HighlightedToken>& tokens,
                       const std::string& title = "B-Minor Source");

} // namespace bminor

#endif // BMINOR_OUTPUT_FORMATTER_HPP
