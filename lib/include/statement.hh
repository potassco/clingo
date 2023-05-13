#pragma once

#include <memory>
#include <iostream>
#include <sstream>

#include <head_literal.hh>
#include <body_literal.hh>

struct Statement {
    virtual ~Statement() = default;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream & {
        stm.print(out);
        return out;
    }
};

using UStatement = std::unique_ptr<Statement>;

struct Rule : Statement {
    Rule(UHeadLiteral head, UBodyLiteralVec body) : head{std::move(head)}, body{std::move(body)} {}
    void print(std::ostream &out) const override {
        out << *head;
        if (head->print_empty() || !body.empty()) {
            out << ":-" << p_range(body, ";");
        }
        out << ".";
    }
    UHeadLiteral head;
    UBodyLiteralVec body;
};


