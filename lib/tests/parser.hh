#pragma once

#include <memory>
#include <optional>
#include <sstream>

#include <statement.hh>

#include <util/print.hh>

using STerm = shared_ptr<Term>;
using SLiteral = shared_ptr<Literal>;
using SHeadLiteral = shared_ptr<HeadLiteral>;
using SBodyLiteral = shared_ptr<BodyLiteral>;
using SStatement = shared_ptr<Statement>;

namespace test {

auto parse_term(std::string str) -> std::optional<STerm>;
auto parse_literal(std::string str) -> std::optional<SLiteral>;
auto parse_head_literal(std::string str) -> std::optional<SHeadLiteral>;
auto parse_body_literal(std::string str) -> std::optional<SBodyLiteral>;
auto parse_statement(std::string str) -> std::optional<SStatement>;

template <class T> auto to_str(std::optional<T> const &value) -> std::string {
    if (value) {
        return value.value()->to_string();
    }
    return "<failed>";
}

template <class T> auto to_str(shared_ptr<T> const &value) -> std::string { return value->to_string(); }

template <class T> auto to_str(std::vector<T> const &value, char const *sep = ", ") -> std::string {
    std::ostringstream oss;
    oss << "[" << p_range(value, sep) << "]";
    return oss.str();
}

template <class T> auto unpool_str(std::optional<shared_ptr<T>> value, char const *sep = ", ") -> std::string {
    if (value) {
        return to_str(value.value()->unpool(), sep);
    }
    return "<failed>";
}

template <class T> auto project_str(std::optional<shared_ptr<T>> value) -> std::string {
    if (value) {
        return to_str(value.value()->project());
    }
    return "<failed>";
}

template <class T>
auto variables_str(std::optional<shared_ptr<T>> value, auto mode, char const *sep = ", ") -> std::string {
    if (value) {
        auto vars = select_variables(*value.value(), mode);
        auto sorted = std::vector<VariableSet::value_type>{vars.begin(), vars.end()};
        std::sort(sorted.begin(), sorted.end());
        return to_str(sorted);
    }
    return "<failed>";
}

struct Parser {
    struct Impl;
    Parser(std::string input);
    ~Parser();
    [[nodiscard]] auto scan() const -> std::optional<std::string>;
    std::unique_ptr<Impl> impl;
};
} // namespace test
