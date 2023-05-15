#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

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
    explicit TheoryTermUnparsed(UTheoryTerm term, GuardVec guards = {})
        : term{std::move(term)}, guards{std::move(guards)} {}
    void print(std::ostream &out) const override {
        if (!ops.empty()) {
            out << p_range(ops, " ") << " ";
        }
        out << *term;
        p_range_with(guards, [](std::ostream &out, Guard const &guard) {
            out << p_range(guard.first, " ") << " " << guard.second;
        });
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
    explicit TheoryTermTuple(TheoryTermTupleType type, ElementVec elems) : elems{std::move(elems)} {}
    void print(std::ostream &out) const override { out << left_bracket(type) << p_range(elems) << right_bracket(type); }
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
    void print(std::ostream &out) const override { out << value; }
    std::string value;
};

struct TheoryTermVariable : TheoryTerm {
    using Element = UTheoryTerm;
    using ElementVec = std::vector<UTheoryTerm>;
    explicit TheoryTermVariable(std::string value) : value{std::move(value)} {}
    void print(std::ostream &out) const override { out << value; }
    std::string value;
};

struct TheoryTermAnonymousVariable : TheoryTerm {
    using Element = UTheoryTerm;
    using ElementVec = std::vector<UTheoryTerm>;
    TheoryTermAnonymousVariable() = default;
    void print(std::ostream &out) const override { out << "_"; }
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
    friend auto operator<<(std::ostream &out, TheoryAtom const &atom) -> std::ostream & {
        out << "&p{...}";
        return out;
    }
};
