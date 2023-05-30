#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <literal.hh>
#include <term.hh>

#include <util/print.hh>

struct TheoryTerm {
    virtual ~TheoryTerm() = default;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream & {
        term.print(out);
        return out;
    }
};

using UTheoryTerm = std::unique_ptr<TheoryTerm>;
using UTheoryTermVec = std::vector<UTheoryTerm>;

struct TheoryTermUnparsed : TheoryTerm {
    using OpVec = std::vector<std::string>;
    using Guard = std::pair<OpVec, UTheoryTerm>;
    using GuardVec = std::vector<Guard>;
    explicit TheoryTermUnparsed(OpVec ops, UTheoryTerm term, GuardVec guards = {})
        : ops{std::move(ops)}, term{std::move(term)}, guards{std::move(guards)} {}
    explicit TheoryTermUnparsed(UTheoryTerm term, GuardVec guards) : term{std::move(term)}, guards{std::move(guards)} {}
    void print(std::ostream &out) const override {
        bool needs_parens = !ops.empty() || !guards.empty();
        if (needs_parens) {
            out << "(";
        }
        if (!ops.empty()) {
            out << p_range(ops, " ") << " ";
        }
        out << *term << p_range_with(guards, [](std::ostream &out, Guard const &guard) {
            out << " " << p_range(guard.first, " ") << " " << *guard.second;
        });
        if (needs_parens) {
            out << ")";
        }
    }
    OpVec ops;
    UTheoryTerm term;
    GuardVec guards;
};

enum class TheoryTermTupleType { Tuple, Set, List };

inline auto left_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::Tuple: {
            return '(';
        }
        case TheoryTermTupleType::Set: {
            return '{';
        }
        case TheoryTermTupleType::List: {
            break;
        }
    }
    return '[';
}

inline auto right_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::Tuple: {
            return ')';
        }
        case TheoryTermTupleType::Set: {
            return '}';
        }
        case TheoryTermTupleType::List: {
            break;
        }
    }
    return ']';
}

struct TheoryTermTuple : TheoryTerm {
    using Element = UTheoryTerm;
    using ElementVec = std::vector<UTheoryTerm>;
    explicit TheoryTermTuple(TheoryTermTupleType type, ElementVec elems) : type{type}, elems{std::move(elems)} {}
    void print(std::ostream &out) const override {
        out << left_bracket(type) << p_range(elems);
        if (type == TheoryTermTupleType::Tuple && elems.size() == 1) {
            out << ",";
        }
        out << right_bracket(type);
    }
    TheoryTermTupleType type;
    ElementVec elems;
};

struct TheoryTermConstant : TheoryTerm {
    using Element = UTheoryTerm;
    using ElementVec = std::vector<UTheoryTerm>;
    TheoryTermConstant(Constant value) : value{value} {}
    void print(std::ostream &out) const override { out << value; }
    Constant value;
};

struct TheoryTermInteger : TheoryTerm {
    using Element = UTheoryTerm;
    using ElementVec = std::vector<UTheoryTerm>;
    explicit TheoryTermInteger(int value) : value{value} {}
    void print(std::ostream &out) const override { out << value; }
    int value;
};

struct TheoryTermString : TheoryTerm {
    using Element = UTheoryTerm;
    using ElementVec = std::vector<UTheoryTerm>;
    explicit TheoryTermString(std::string value) : value{std::move(value)} {}
    void print(std::ostream &out) const override { print_quoted(out, value); }
    std::string value;
};

struct TheoryTermVariable : TheoryTerm {
    using Element = UTheoryTerm;
    using ElementVec = std::vector<UTheoryTerm>;
    explicit TheoryTermVariable(std::string value) : value{std::move(value)} {}
    void print(std::ostream &out) const override { out << value; }
    std::string value;
};

struct TheoryTermFunction : TheoryTerm {
    explicit TheoryTermFunction(std::string name, UTheoryTermVec args = {})
        : name(std::move(name)), args{std::move(args)} {}

    void print(std::ostream &out) const override {
        out << name;
        if (!args.empty()) {
            out << "(" << p_range(args) << ")";
        }
    }

    std::string name;
    UTheoryTermVec args;
};

struct TheoryAtom {
    using Element = std::pair<UTheoryTermVec, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    TheoryAtom(STerm name, ElementVec elems) : name{std::move(name)}, elems{std::move(elems)} {}
    TheoryAtom(STerm name, ElementVec elems, std::string guard_op, UTheoryTerm guard_term)
        : name{std::move(name)}, elems{std::move(elems)},
          guard{std::make_pair(std::move(guard_op), std::move(guard_term))} {}
    friend auto operator<<(std::ostream &out, TheoryAtom const &atom) -> std::ostream & {
        out << "&" << *atom.name;
        if (!atom.elems.empty() || atom.guard.has_value()) {
            out << " { " << p_range_with(atom.elems, "; ", [](std::ostream &out, Element const &elem) {
                out << p_range(elem.first);
                if (!elem.second.empty() || elem.first.empty()) {
                    out << ": " << p_range(elem.second, ", ");
                }
            }) << (atom.elems.empty() ? "}" : " }");
        }
        if (atom.guard.has_value()) {
            out << " " << atom.guard.value().first << " " << *atom.guard.value().second;
        }
        return out;
    }
    STerm name;
    ElementVec elems;
    std::optional<std::pair<std::string, UTheoryTerm>> guard;
};
