#pragma once

#include <string>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <util/lexy_report_error.hh>
#include <util/lexy_stream_input.hh>

namespace grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;
using input = StreamInput<encoding>;
using iterator = input::iterator;
using lexeme = lexy::lexeme_for<input>;

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
                                       dsl::peek(LEXY_LIT("%*")) >> (dsl::token(dsl::p<block_comment>) | dsl::error<expected_bc_close>) |
                                       LEXY_LIT("%") >> dsl::until(dsl::newline);
};

struct identifier : lexy::token_production {
    static constexpr auto rule = []() {
        auto prefix = dsl::while_one(LEXY_LIT("_") / LEXY_LIT("'"));
        auto head = dsl::ascii::lower;
        auto tail = dsl::ascii::alpha_underscore / LEXY_LIT("'");
        auto id = dsl::identifier(head, tail);
        auto kw_not = LEXY_KEYWORD("not", id);

        return id.reserve(kw_not) | dsl::capture(dsl::token(prefix + id));
    }();
    static constexpr auto value = lexy::as_string<std::string>;
};

} // namespace grammar
