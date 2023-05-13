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

struct control {
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline;
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
