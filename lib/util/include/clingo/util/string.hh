#pragma once

#include <cassert>
#include <string_view>

namespace CppClingo::Util {

void quote(std::string_view in, auto out) {
    for (auto c : in) {
        switch (c) {
            case '\n': {
                *out++ = '\\';
                *out++ = 'n';
                break;
            }
            case '\t': {
                *out++ = '\\';
                *out++ = 't';
                break;
            }
            case '\\': {
                *out++ = '\\';
                *out++ = '\\';
                break;
            }
            case '"': {
                *out++ = '\\';
                *out++ = '"';
                break;
            }
            default: {
                *out++ = c;
                break;
            }
        }
    }
}
void unquote(std::string_view in, auto out, bool fstring = false) {
    auto escape = '\0';
    for (auto c : in) {
        if (escape == '{' || escape == '}') {
            if (c == escape) {
                *out++ = escape;
            } else {
                assert(false);
            }
            escape = '\0';
        } else if (escape == '\\') {
            switch (c) {
                case 'n': {
                    *out++ = '\n';
                    break;
                }
                case 't': {
                    *out++ = '\t';
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
                    assert(false);
                    break;
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
