#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace CppClingo::Util {

namespace Detail {

// NOLINTBEGIN(readability-magic-numbers)

inline auto hex_val(char c) -> uint32_t {
    if (c >= '0' && c <= '9') {
        return static_cast<uint32_t>(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<uint32_t>(10 + (c - 'A'));
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<uint32_t>(10 + (c - 'a'));
    }
    throw std::invalid_argument("invalid hex character");
}

inline auto encode_utf8(uint32_t cp, auto out) -> void {
    if (cp > 0x10FFFF) {
        throw std::invalid_argument("invalid unicode code point");
    }
    if (cp <= 0x7F) {
        *out++ = static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        *out++ = static_cast<char>(0xC0 | (cp >> 6));
        *out++ = static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        *out++ = static_cast<char>(0xE0 | (cp >> 12));
        *out++ = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        *out++ = static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        *out++ = static_cast<char>(0xF0 | (cp >> 18));
        *out++ = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        *out++ = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        *out++ = static_cast<char>(0x80 | (cp & 0x3F));
    }
}

inline auto parse_unicode_escape(auto it, auto ie, auto out) -> auto {
    uint32_t cp = 0;
    if (it == ie || *it != '{') {
        throw std::runtime_error("expected '{' at the beginning of unicode escape");
    }
    size_t count = 0;
    for (++it; it != ie; ++it, ++count) {
        if (*it == '}') {
            if (count == 0) {
                throw std::runtime_error("expected at least one hex digit in unicode escape");
            }
            encode_utf8(cp, out);
            return it;
        }
        if (count >= 6) {
            throw std::runtime_error("too many hex digits in unicode escape");
        }
        cp = (cp << 4) | hex_val(*it);
    }
    throw std::runtime_error("expected '}' at the end of unicode escape");
}

// NOLINTEND(readability-magic-numbers)

} // namespace Detail

void unquote(std::string_view in, auto out, bool fstring = false) {
    auto escape = '\0';
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto it = in.begin(), ie = in.end(); it != ie; ++it) {
        char c = *it;
        if (escape == '{' || escape == '}') {
            if (c == escape) {
                *out++ = escape;
            } else {
                throw std::runtime_error("expected brace");
            }
            escape = '\0';
        } else if (escape == '\\') {
            switch (c) {
                case 'u': {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    it = Detail::parse_unicode_escape(it + 1, ie, out);
                    break;
                }
                case 'n': {
                    *out++ = '\n';
                    break;
                }
                case 't': {
                    *out++ = '\t';
                    break;
                }
                case 'r': {
                    *out++ = '\r';
                    break;
                }
                case '\\': {
                    *out++ = '\\';
                    break;
                }
                case '"': {
                    *out++ = '"';
                    break;
                }
                default: {
                    throw std::runtime_error("invalid escape sequence");
                }
            }
            escape = '\0';
        } else if (c == '\\' || (fstring && (c == '{' || c == '}'))) {
            escape = c;
        } else {
            *out++ = c;
        }
    }
}

} // namespace CppClingo::Util
