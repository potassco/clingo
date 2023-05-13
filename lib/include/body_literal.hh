#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <aggregate.hh>
#include <literal.hh>

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
};

using UBodyLiteral = std::unique_ptr<BodyLiteral>;
using UBodyLiteralVec = std::vector<UBodyLiteral>;

struct ConditionalLiteral : BodyLiteral {
    ConditionalLiteral(ULiteral literal, ULiteralVec condition)
        : literal{std::move(literal)}, condition{std::move(condition)} {}
    void add_sign(Sign s) override { literal->add_sign(s); }
    void print(std::ostream &out) const override {
        out << *literal;
        if (!condition.empty()) {
            out << ":" << p_range(condition);
        }
    }

    ULiteral literal;
    ULiteralVec condition;
};

struct BodyAggregate : BodyLiteral {
    using Element = std::tuple<UTermVec, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    BodyAggregate(AggregateFunction fun, ElementVec elems) : fun(fun), elements(std::move(elems)) {}
    BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, UTerm rhs)
        : fun(fun), elements(std::move(elems)), right_guard(std::make_pair(rel, std::move(rhs))) {}
    void add_sign(Sign s) override { sign += s; }
    void set_left_guard(UTerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        out << sign;
        if (left_guard) {
            out << *left_guard->first << left_guard->second;
        }
        out << fun << "{" << p_range_with(elements, ";", [](std::ostream &out, auto const &elem) {
            out << p_range{std::get<0>(elem), ","};
            if (!std::get<1>(elem).empty()) {
                out << ":" << p_range{std::get<1>(elem)};
            }
        }) << "}";
        if (right_guard) {
            out << right_guard->first << *right_guard->second;
        }
    }
    Sign sign = Sign::none;
    AggregateFunction fun;
    ElementVec elements;
    std::optional<std::pair<UTerm, Relation>> left_guard;
    std::optional<std::pair<Relation, UTerm>> right_guard;
};

using UBodyAggregate = std::unique_ptr<BodyAggregate>;

struct BodySetAggregate : BodyLiteral {
    using Element = std::pair<ULiteral, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    BodySetAggregate(ElementVec elements) : elements{std::move(elements)} {}
    BodySetAggregate(ElementVec elements, Relation rel, UTerm rhs)
        : elements{std::move(elements)}, right_guard(std::make_pair(rel, std::move(rhs))) {}
    void add_sign(Sign s) override { sign += s; }
    void set_left_guard(UTerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        out << sign;
        if (left_guard) {
            out << *left_guard->first << left_guard->second;
        }
        out << "{" << p_range_with(elements, ";", [](std::ostream &out, auto const &elem) {
            out << *std::get<0>(elem);
            if (!std::get<1>(elem).empty()) {
                out << ":" << p_range{std::get<1>(elem)};
            }
        }) << "}";
        if (right_guard) {
            out << right_guard->first << *right_guard->second;
        }
    }
    Sign sign = Sign::none;
    ElementVec elements;
    std::optional<std::pair<UTerm, Relation>> left_guard;
    std::optional<std::pair<Relation, UTerm>> right_guard;
};

using UBodySetAggregate = std::unique_ptr<BodySetAggregate>;

struct BodyTheoryAtom : BodyLiteral {
    void add_sign(Sign s) override { sign += s; }
    void print(std::ostream &out) const override { out << sign << "&p{...}"; }
    Sign sign = Sign::none;
};
