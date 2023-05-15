#pragma once

#include <memory>
#include <optional>
#include <string>

namespace test {

auto parse_term(std::string str) -> std::string;
auto parse_literal(std::string str) -> std::string;
auto parse_head_literal(std::string str) -> std::string;
auto parse_body_literal(std::string str) -> std::string;
auto parse_statement(std::string str) -> std::string;

struct Parser {
    struct Impl;
    Parser(std::string input);
    ~Parser();
    [[nodiscard]] auto scan() const -> std::optional<std::string>;
    std::unique_ptr<Impl> impl;
};
} // namespace test
