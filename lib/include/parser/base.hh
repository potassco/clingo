#pragma once

#include <string>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <util/lexy_report_error.hh>
#include <util/lexy_stream_input.hh>

#define STRING_TAG(n, v)                                                                                               \
    struct expected_##n {                                                                                              \
        static constexpr char const *name = v;                                                                         \
    }

namespace grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;

struct block_comment : lexy::scan_production<void> {
    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        size_t n = 0;
        do {
            if (scanner.branch(LEXY_LIT("%*"))) {
                ++n;
                continue;
            }
            if (scanner.branch(LEXY_LIT("*%"))) {
                --n;
                continue;
            }
            scanner.parse(dsl::code_point);
            if (!scanner) {
                return lexy::scan_failed;
            }
        } while (n > 0);
        return scan_result{true};
    }
};

struct control {
    struct expected_bc_close {
        static constexpr char const *name = "unclosed block comment";
    };
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline |
                                       dsl::peek(LEXY_LIT("%*")) >>
                                           (dsl::token(dsl::p<block_comment>) | dsl::error<expected_bc_close>) |
                                       LEXY_LIT("%") >> dsl::until(dsl::newline);
};

} // namespace grammar
