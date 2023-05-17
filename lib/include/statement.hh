#pragma once

#include <iostream>
#include <memory>

#include <body_literal.hh>
#include <head_literal.hh>
#include <optional>

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
            out << " :- " << p_range(body, "; ");
        }
        out << ".";
    }
    UHeadLiteral head;
    UBodyLiteralVec body;
};

enum class TheoryOpType { unary, binary_left, binary_right };

struct TheoryOpDefinition {
    TheoryOpDefinition(std::string op, int priority, TheoryOpType type)
        : op{std::move(op)}, priority{priority}, type{type} {}
    friend auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream & {
        out << def.op << " : " << def.priority << ", ";
        switch (def.type) {
            case TheoryOpType::unary: {
                out << "unary";
                break;
            }
            case TheoryOpType::binary_left: {
                out << "binary, left";
                break;
            }
            case TheoryOpType::binary_right: {
                out << "binary, right";
                break;
            }
        }
        return out;
    }
    std::string op;
    int priority;
    TheoryOpType type;
};
using TheoryOpDefinitionVec = std::vector<TheoryOpDefinition>;

struct TheoryTermDefinition {
    TheoryTermDefinition(std::string name, TheoryOpDefinitionVec op_defs)
        : name{std::move(name)}, op_defs{std::move(op_defs)} {}
    friend auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream & {
        out << "  " << def.name << " {";
        if (def.op_defs.empty()) {
            out << " }";
        } else {
            for (auto const &op_def : def.op_defs) {
                out << "    " << op_def << "\n";
            }
            out << "  }";
        }
        return out;
    }
    std::string name;
    std::vector<TheoryOpDefinition> op_defs;
};

enum class TheoryAtomType { head, body, any, directive };

struct TheoryAtomDefinition {
    std::string name;
    int arity;
    std::string term;
    std::optional<std::pair<std::vector<std::string>, std::string>> guard;
    TheoryAtomType type;
};

struct TheoryDefinition : Statement {
    TheoryDefinition(std::string name) : name{std::move(name)} {}
    void print(std::ostream &out) const override { out << "#theory " << name << " { ... }."; }
    std::string name;
};
