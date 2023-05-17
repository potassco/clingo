#pragma once

#include <iostream>
#include <memory>
#include <optional>

#include <body_literal.hh>
#include <head_literal.hh>

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
    explicit Rule(UHeadLiteral head, UBodyLiteralVec body) : head{std::move(head)}, body{std::move(body)} {}
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
    explicit TheoryOpDefinition(std::string op, int priority, TheoryOpType type)
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
    explicit TheoryTermDefinition(std::string name, TheoryOpDefinitionVec op_defs)
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

inline auto operator<<(std::ostream &out, TheoryAtomType type) -> std::ostream & {
    switch(type) {
        case TheoryAtomType::head: {
            out << "head";
            break;
        }
        case TheoryAtomType::body: {
            out << "body";
            break;
        }
        case TheoryAtomType::any: {
            out << "any";
            break;
        }
        case TheoryAtomType::directive: {
            out << "directive";
            break;
        }
    }
    return out;
}

struct TheoryAtomDefinition {
    using Guard = std::optional<std::pair<std::vector<std::string>, std::string>>;
    explicit TheoryAtomDefinition(std::string name, int arity, std::string term, Guard guard, TheoryAtomType type)
    : name(std::move(name)), arity(arity), term(std::move(term)), guard(std::move(guard)), type(type) { }
    explicit TheoryAtomDefinition(std::string name, int arity, std::string term, TheoryAtomType type)
    : name(std::move(name)), arity(arity), term(std::move(term)), guard(std::nullopt), type(type) { }
    friend auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream & {
        out << "  &" << def.name << "/" << def.arity << ": " << def.term << ", ";
        if (def.guard) {
            out << "{" << p_range(def.guard->first, ",") << "}, " << def.guard->second << ", ";
        }
        out << def.type;
        return out;
    }
    std::string name;
    int arity;
    std::string term;
    std::optional<std::pair<std::vector<std::string>, std::string>> guard;
    TheoryAtomType type;
};

struct TheoryDefinition : Statement {
    explicit TheoryDefinition(std::string name) : name{std::move(name)} {}
    void print(std::ostream &out) const override { out << "#theory " << name << " { ... }."; }
    std::string name;
};
