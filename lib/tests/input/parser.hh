#pragma once

#include <memory>
#include <optional>
#include <sstream>

#include <input/statement.hh>

#include <input/algo/print.hh>

#include <util/print.hh>

namespace Gringo::Input::Test {

auto parse_term(std::string str) -> std::optional<STerm>;
auto parse_literal(std::string str) -> std::optional<SLiteral>;
auto parse_head_literal(std::string str) -> std::optional<SHeadLiteral>;
auto parse_body_literal(std::string str) -> std::optional<SBodyLiteral>;
auto parse_statement(std::string str) -> std::optional<SStatement>;

template <class T> auto to_str(std::optional<T> const &value) -> std::string {
    if (value) {
        return to_string(*value.value());
    }
    return "<failed>";
}

template <class T> auto to_str(Util::shared_ptr<T> const &value) -> std::string { return to_string(*value); }

template <class T> auto to_str(std::vector<T> const &value, char const *sep = ", ") -> std::string {
    std::ostringstream oss;
    oss << "[" << Util::p_range(value, sep) << "]";
    return oss.str();
}

template <class T> auto unpool_str(std::optional<Util::shared_ptr<T>> value, char const *sep = ", ") -> std::string {
    if (value) {
        auto unpooled = value.value()->unpool();
        if (!unpooled.has_value()) {
            unpooled = make_vec<Util::shared_ptr<T>>(value.value());
        }
        return to_str(unpooled.value(), sep);
    }
    return "<failed>";
}

template <class T> auto project_str(std::optional<Util::shared_ptr<T>> value) -> std::string {
    if (value) {
        return to_str(value.value()->project(ProjectionMode::pure, true).value_or(value.value()));
    }
    return "<failed>";
}

template <class T> auto variables_str(std::optional<Util::shared_ptr<T>> value, auto mode) -> std::string {
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

} // namespace Gringo::Input::Test
