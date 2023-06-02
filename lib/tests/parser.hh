#pragma once

#include <memory>

#include <statement.hh>

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

template <class T> auto to_str(std::optional<T> value) -> std::string {
    if (value) {
        return value.value()->to_string();
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
