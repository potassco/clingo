#pragma once

#include <sstream>

#include <lexy/action/parse.hpp>

#include <parser/base.hh>

namespace test {

namespace grammar {

namespace dsl = lexy::dsl;

template <class P, char t = '\0'> struct parse_root : ::grammar::control {
    static constexpr auto terminator() { return t; }
    static constexpr auto eof() {
        if constexpr (t == '\0') {
            return dsl::eof;
        } else {
            return dsl::lit_c<t> + dsl::eof;
        }
    }
    static constexpr auto rule = dsl::p<P> + eof();
    static constexpr auto value = lexy::forward<typename decltype(P::value)::return_type>;
};

template <class P> struct match_root : ::grammar::control {
    static constexpr auto rule = dsl::p<P> + dsl::eof;
};

} // namespace grammar

template <typename Control> auto parse(std::string str) -> std::string {
    if (Control::terminator() != '\0') {
        str.push_back('.');
    }
    std::istringstream in;
    in.str(std::move(str));
    auto input = ::grammar::input{in};
    auto stm = lexy::parse<Control>(input, report_error);
    return stm.has_value() ? stm.value()->to_string() : "<failed>";
}

template <typename Control> auto match(std::string str) {
    std::istringstream in;
    in.str(std::move(str));
    auto input = ::grammar::input{in};
    auto res = lexy::validate<Control>(input, report_error);
    return res.is_success();
}

} // namespace test
