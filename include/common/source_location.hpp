#ifndef BMINOR_COMMON_SOURCE_LOCATION_HPP
#define BMINOR_COMMON_SOURCE_LOCATION_HPP

#include <cstddef>
#include <string>

namespace bminor {

/// Represents a 1-based position inside a B-Minor source file.
struct SourceLocation {
    std::string filename;
    std::size_t line = 1;
    std::size_t column = 1;

    /// Formats the position as "line:column".
    std::string toLineColString() const {
        return std::to_string(line) + ":" + std::to_string(column);
    }

    /// Formats the position as "filename:line:column" (or "line:column" if filename is empty).
    std::string toString() const {
        if (filename.empty()) {
            return toLineColString();
        }
        return filename + ":" + toLineColString();
    }
};

} // namespace bminor

#endif // BMINOR_COMMON_SOURCE_LOCATION_HPP
