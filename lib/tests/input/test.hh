#pragma once

#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>

#include <util/algorithm.hh>
#include <util/print.hh>

namespace Gringo::Input::Test {

inline auto default_store() -> SymbolStore & {
    static auto store = make_symbol_store(true, true);
    return *store;
}

inline auto parse_term(std::string_view str) -> std::optional<Term> {
    Logger log;
    return Gringo::Input::parse_term(log, default_store(), str);
}

inline auto parse_literal(std::string_view str) -> std::optional<Literal> {
    Logger log;
    return Gringo::Input::parse_literal(log, default_store(), str);
}

inline auto parse_head_literal(std::string_view str) -> std::optional<HeadLiteral> {
    Logger log;
    return Gringo::Input::parse_head_literal(log, default_store(), str);
}

inline auto parse_body_literal(std::string_view str) -> std::optional<BodyLiteral> {
    Logger log;
    return Gringo::Input::parse_body_literal(log, default_store(), str);
}

inline auto parse_statement(std::string_view str) -> std::optional<Statement> {
    Logger log;
    return Gringo::Input::parse_statement(log, default_store(), str);
}

template <class T> auto to_str(T const &value) -> std::string { return to_string(value); }

template <class T> auto to_str(std::optional<T> const &value) -> std::string {
    if (value) {
        return to_str(value.value());
    }
    return "<failed>";
}

template <class T> auto to_str(std::vector<T> const &value, char const *sep = ", ") -> std::string {
    std::ostringstream oss;
    oss << "[" << Util::p_range(value, sep) << "]";
    return oss.str();
}

} // namespace Gringo::Input::Test
