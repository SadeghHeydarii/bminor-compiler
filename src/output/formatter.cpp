#include "output/formatter.hpp"

#include <sstream>

namespace bminor {

std::string ansiPrefixForCategory(HighlightCategory category) {
    switch (category) {
        case HighlightCategory::Keyword:             return "\x1b[1;34m";
        case HighlightCategory::TypeName:            return "\x1b[36m";
        case HighlightCategory::VarIdentifier:       return "";
        case HighlightCategory::FuncIdentifier:      return "\x1b[93m";
        case HighlightCategory::IntLiteral:          return "\x1b[38;5;208m";
        case HighlightCategory::BoolLiteral:         return "\x1b[38;5;208m";
        case HighlightCategory::StringOrCharLiteral: return "\x1b[32m";
        case HighlightCategory::Operator:            return "\x1b[37m";
        case HighlightCategory::Comment:             return "\x1b[2;3;90m";
        case HighlightCategory::Error:               return "\x1b[4;91m";
        case HighlightCategory::None:                return "";
    }
    return "";
}

std::string htmlClassForCategory(HighlightCategory category) {
    switch (category) {
        case HighlightCategory::Keyword:             return "kw";
        case HighlightCategory::TypeName:            return "type";
        case HighlightCategory::VarIdentifier:       return "var";
        case HighlightCategory::FuncIdentifier:      return "fn";
        case HighlightCategory::IntLiteral:          return "lit";
        case HighlightCategory::BoolLiteral:         return "lit";
        case HighlightCategory::StringOrCharLiteral: return "str";
        case HighlightCategory::Operator:            return "op";
        case HighlightCategory::Comment:             return "comment";
        case HighlightCategory::Error:               return "err";
        case HighlightCategory::None:                return "";
    }
    return "";
}

std::string escapeHtml(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
            case '&':  escaped += "&amp;";  break;
            case '<':  escaped += "&lt;";   break;
            case '>':  escaped += "&gt;";   break;
            case '"':  escaped += "&quot;"; break;
            case '\'': escaped += "&#39;";  break;
            default:   escaped += ch;       break;
        }
    }
    return escaped;
}

std::string formatAnsi(const std::vector<HighlightedToken>& tokens) {
    std::ostringstream out;
    for (const auto& tok : tokens) {
        const std::string prefix = ansiPrefixForCategory(tok.category);
        if (prefix.empty()) {
            out << tok.lexeme;
        } else {
            out << prefix << tok.lexeme << "\x1b[0m";
        }
    }
    return out.str();
}

std::string formatHtml(const std::vector<HighlightedToken>& tokens,
                       const std::string& title) {
    const std::string safeTitle = escapeHtml(title);
    std::ostringstream out;
    out << "<!DOCTYPE html>\n"
        << "<html lang=\"en\">\n"
        << "<head>\n"
        << "  <meta charset=\"UTF-8\">\n"
        << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        << "  <title>" << safeTitle << "</title>\n"
        << "  <style>\n"
        << "    body { margin: 0; padding: 24px; background: #0f172a; color: #f8fafc; font-family: system-ui, -apple-system, sans-serif; }\n"
        << "    .container { max-width: 960px; margin: 0 auto; background: #1e293b; border: 1px solid #334155; border-radius: 8px; overflow: hidden; }\n"
        << "    .header { padding: 10px 16px; background: #0b1120; border-bottom: 1px solid #334155; font-size: 13px; color: #94a3b8; }\n"
        << "    pre.bminor-code { margin: 0; padding: 16px; overflow-x: auto; font-family: Consolas, \"Courier New\", monospace; font-size: 14px; line-height: 1.55; }\n"
        << "    .kw { color: #60a5fa; font-weight: bold; }\n"
        << "    .type { color: #2dd4bf; }\n"
        << "    .var { color: #f8fafc; }\n"
        << "    .fn { color: #facc15; }\n"
        << "    .lit { color: #fb923c; }\n"
        << "    .str { color: #86efac; }\n"
        << "    .op { color: #cbd5e1; }\n"
        << "    .comment { color: #94a3b8; font-style: italic; }\n"
        << "    .err { color: #f87171; text-decoration: underline wavy #ef4444; }\n"
        << "  </style>\n"
        << "</head>\n"
        << "<body>\n"
        << "  <div class=\"container\">\n"
        << "    <div class=\"header\">" << safeTitle << "</div>\n"
        << "    <pre class=\"bminor-code\"><code>";

    for (const auto& tok : tokens) {
        const std::string escapedLexeme = escapeHtml(tok.lexeme);
        const std::string cssClass = htmlClassForCategory(tok.category);
        if (cssClass.empty()) {
            out << escapedLexeme;
        } else {
            out << "<span class=\"" << cssClass << "\">" << escapedLexeme << "</span>";
        }
    }

    out << "</code></pre>\n"
        << "  </div>\n"
        << "</body>\n"
        << "</html>\n";
    return out.str();
}

} // namespace bminor
