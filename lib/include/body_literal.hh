#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <aggregate.hh>
#include <literal.hh>
#include <theory.hh>

struct BodyLiteral {
    virtual ~BodyLiteral() = default;
    virtual void add_sign(Sign sign) = 0;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, BodyLiteral const &literal) -> std::ostream & {
        literal.print(out);
        return out;
    }
    size_t refs = 0;
};

using SBodyLiteral = shared_ptr<BodyLiteral>;
using SBodyLiteralVec = std::vector<SBodyLiteral>;

struct ConditionalLiteral : BodyLiteral {
    ConditionalLiteral(SLiteral literal, SLiteralVec condition)
        : literal{std::move(literal)}, condition{std::move(condition)} {}
    void add_sign(Sign s) override { literal->add_sign(s); }
    void print(std::ostream &out) const override {
        out << *literal;
        if (!condition.empty()) {
            out << ": " << p_range(condition, ", ");
        }
    }

    SLiteral literal;
    SLiteralVec condition;
};

struct BodyAggregate : BodyLiteral {
    using Element = std::tuple<STermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;
    BodyAggregate(AggregateFunction fun, ElementVec elems) : fun(fun), elements(std::move(elems)) {}
    BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, STerm rhs)
        : fun(fun), elements(std::move(elems)), right_guard(std::make_pair(rel, std::move(rhs))) {}
    void add_sign(Sign s) override { sign += s; }
    void set_left_guard(STerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        out << sign;
        if (left_guard) {
            out << *left_guard->first << " " << left_guard->second << " ";
        }
        out << fun << " { " << p_range_with(elements, "; ", [](std::ostream &out, auto const &elem) {
            out << p_range{std::get<0>(elem), ","};
            if (!std::get<1>(elem).empty()) {
                out << ": " << p_range{std::get<1>(elem), ", "};
            }
        }) << (elements.empty() ? "}" : " }");
        if (right_guard) {
            out << " " << right_guard->first << " " << *right_guard->second;
        }
    }
    Sign sign = Sign::none;
    AggregateFunction fun;
    ElementVec elements;
    std::optional<std::pair<STerm, Relation>> left_guard;
    std::optional<std::pair<Relation, STerm>> right_guard;
};

using SBodyAggregate = shared_ptr<BodyAggregate>;

struct BodySetAggregate : BodyLiteral {
    BodySetAggregate(SetAggregate aggr) : aggr{std::move(aggr)} {}
    void add_sign(Sign s) override { sign += s; }
    void set_left_guard(STerm lhs, Relation rel) { aggr.set_left_guard(std::move(lhs), rel); }
    void print(std::ostream &out) const override { out << sign << aggr; }
    Sign sign = Sign::none;
    SetAggregate aggr;
};

using SBodySetAggregate = shared_ptr<BodySetAggregate>;

struct BodyTheoryAtom : BodyLiteral {
    BodyTheoryAtom(TheoryAtom atom) : atom(std::move(atom)) {}
    void add_sign(Sign s) override { sign += s; }
    void print(std::ostream &out) const override { out << sign << atom; }
    Sign sign = Sign::none;
    TheoryAtom atom;
};
