#include <lexy/action/parse.hpp>
#include <lexy/action/scan.hpp>

#include <memory>
#include <optional>
#include <parser/statement.hh>

#include "parser.hh"
#include "parser/base.hh"

namespace test {

namespace grammar {

using input = StreamInput<::grammar::encoding>;

namespace dsl = lexy::dsl;

template <class P, char t = '\0'> struct parse_root : ::grammar::control {
    static constexpr auto terminator() { return t; }
    static constexpr auto eof() {
        if constexpr (t == '*') {
            return dsl::return_;
        } else if constexpr (t == '\0') {
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

template <typename Control> auto parse(std::string str) -> std::string {
    if (Control::terminator() != '\0') {
        str.push_back('.');
    }
    std::istringstream in;
    in.str(std::move(str));
    auto input = ::test::grammar::input{in};
    auto stm = lexy::parse<Control>(input, report_error);
    return stm.has_value() ? stm.value()->to_string() : "<failed>";
}

template <typename Control> auto match(std::string str) {
    std::istringstream in;
    in.str(std::move(str));
    auto input = ::test::grammar::input{in};
    auto res = lexy::validate<Control>(input, report_error);
    return res.is_success();
}

} // namespace grammar

auto parse_term(std::string str) -> std::string {
    return grammar::parse<grammar::parse_root<::grammar::term>>(std::move(str));
}

auto parse_literal(std::string str) -> std::string {
    return grammar::parse<grammar::parse_root<::grammar::literal>>(std::move(str));
}

auto parse_head_literal(std::string str) -> std::string {
    return grammar::parse<grammar::parse_root<::grammar::head_literal, '.'>>(std::move(str));
}

auto parse_body_literal(std::string str) -> std::string {
    return grammar::parse<grammar::parse_root<::grammar::body_literal, '.'>>(std::move(str));
}

auto parse_statement(std::string str) -> std::string {
    return grammar::parse<grammar::parse_root<::grammar::statement>>(std::move(str));
}

struct Parser::Impl {
    Impl(std::string str) : in{str}, input{in}, scanner{lexy::scan<::grammar::control>(input, report_error)} {}

    std::istringstream in;
    ::test::grammar::input input = ::test::grammar::input{in};
    decltype(lexy::scan<::grammar::control>(input, report_error)) scanner;
};

Parser::Parser(std::string input) : impl{std::make_unique<Impl>(std::move(input))} {}

auto Parser::scan() const -> std::optional<std::string> {
    impl->input.discard_before(impl->scanner.position());
    auto res = impl->scanner.parse<grammar::parse_root<::grammar::statement, '*'>>();
    if (res) {
        return res.value()->to_string();
    }
    return std::nullopt;
}
Parser::~Parser() = default;

} // namespace test
