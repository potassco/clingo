#include <sstream>

#include <lexy/action/parse.hpp>
#include <lexy/action/scan.hpp>

#include <input/parser/statement.hh>

#include "input/parser.hh"

namespace Gringo::Input::Test {

namespace Grammar {

using input = Util::StreamInput<Input::Grammar::encoding>;

namespace dsl = lexy::dsl;

template <class P, char t = '\0'> struct parse_root : Input::Grammar::control {
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

template <class P> struct match_root : Input::Grammar::control {
    static constexpr auto rule = dsl::p<P> + dsl::eof;
};

template <typename Control>
auto parse(std::string str) -> std::optional<typename decltype(Control::value)::return_type> {
    if (Control::terminator() != '\0') {
        str.push_back('.');
    }
    std::istringstream in;
    in.str(std::move(str));
    auto inp = input{in};
    auto res = lexy::parse<Control>(inp, Util::report_error);
    if (res.has_value()) {
        return res.value();
    }
    return std::nullopt;
}

template <typename Control> auto match(std::string str) {
    std::istringstream in;
    in.str(std::move(str));
    auto inp = input{in};
    auto res = lexy::validate<Control>(inp, Util::report_error);
    return res.is_success();
}

} // namespace Grammar

auto parse_term(std::string str) -> std::optional<Term> {
    return Grammar::parse<Grammar::parse_root<Input::Grammar::term>>(std::move(str));
}

auto parse_literal(std::string str) -> std::optional<SLiteral> {
    return Grammar::parse<Grammar::parse_root<Input::Grammar::literal>>(std::move(str));
}

auto parse_head_literal(std::string str) -> std::optional<SHeadLiteral> {
    return Grammar::parse<Grammar::parse_root<Input::Grammar::head_literal, '.'>>(std::move(str));
}

auto parse_body_literal(std::string str) -> std::optional<SBodyLiteral> {
    return Grammar::parse<Grammar::parse_root<Input::Grammar::body_literal, '.'>>(std::move(str));
}

auto parse_statement(std::string str) -> std::optional<SStatement> {
    return Grammar::parse<Grammar::parse_root<Input::Grammar::statement>>(std::move(str));
}

struct Parser::Impl {
    Impl(std::string str)
        : in{str}, input{in}, scanner{lexy::scan<Input::Grammar::control>(input, Util::report_error)} {}

    std::istringstream in;
    Grammar::input input;
    decltype(lexy::scan<Input::Grammar::control>(input, Util::report_error)) scanner;
};

Parser::Parser(std::string input) : impl{std::make_unique<Impl>(std::move(input))} {}

auto Parser::scan() const -> std::optional<std::string> {
    impl->input.discard_before(impl->scanner.position());
    auto res = impl->scanner.parse<Grammar::parse_root<Input::Grammar::statement, '*'>>();
    if (res) {
        return to_string(*res.value());
    }
    return std::nullopt;
}
Parser::~Parser() = default;

} // namespace Gringo::Input::Test
