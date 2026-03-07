/// @file outbuf.hpp
/// @brief OutputBuffer – a fast std::string-backed output sink for converters.
///
/// Replaces std::ostringstream in the HTML5/DocBook/man-page converters.
/// Benefits over std::ostringstream:
///   - No virtual dispatch on operator<< (each call is a direct string append).
///   - Pre-reserves a large buffer up front so the typical document never
///     triggers a reallocation.
///   - str() returns a const-reference rather than a copy; the caller can
///     either copy or std::move the internal buffer.

#pragma once

#include <string>
#include <string_view>
#include <cstddef>

namespace asciiquack {

class OutputBuffer {
public:
    explicit OutputBuffer(std::size_t initial_capacity = 16384) {
        buf_.reserve(initial_capacity);
    }

    // Append operators – all return *this for chaining just like ostringstream.

    OutputBuffer& operator<<(const std::string& s) {
        buf_ += s; return *this;
    }
    OutputBuffer& operator<<(std::string_view s) {
        buf_.append(s.data(), s.size()); return *this;
    }
    OutputBuffer& operator<<(const char* s) {
        if (s) { buf_ += s; } return *this;
    }
    OutputBuffer& operator<<(char c) {
        buf_ += c; return *this;
    }

    // Compatibility with code that calls out.str() to retrieve the result.
    const std::string& str() const noexcept { return buf_; }

    // Allow the caller to move the buffer out (zero-copy result extraction).
    std::string release() noexcept { return std::move(buf_); }

    bool   empty()  const noexcept { return buf_.empty(); }
    std::size_t size() const noexcept { return buf_.size(); }

private:
    std::string buf_;
};

} // namespace asciiquack
