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
void unquote(std::string_view in, auto out) {
    bool slash = false;
    for (auto c : in) {
        if (slash) {
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
            slash = false;
        } else if (c == '\\') {
            slash = true;
        } else {
            *out++ = c;
        }
    }
}

} // namespace CppClingo::Util
