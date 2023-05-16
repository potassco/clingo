#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <aggregate.hh>
#include <literal.hh>
#include <theory.hh>

struct HeadLiteral {
    virtual ~HeadLiteral() = default;
    [[nodiscard]] virtual auto print_empty() const -> bool { return false; }
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, HeadLiteral const &literal) -> std::ostream & {
        literal.print(out);
        return out;
    }
};

using UHeadLiteral = std::unique_ptr<HeadLiteral>;

struct Disjunction : HeadLiteral {
    using Element = std::pair<ULiteral, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    Disjunction(ElementVec elems) : elems{std::move(elems)} {}
    [[nodiscard]] auto print_empty() const -> bool override { return elems.empty(); }
    void print(std::ostream &out) const override {
        out << p_range_with(elems, "; ", [](std::ostream &out, auto const &elem) {
            out << *elem.first;
            if (!elem.second.empty()) {
                out << ": " << p_range(elem.second);
            }
        });
    }

    ElementVec elems;
};

struct HeadTheoryAtom : HeadLiteral {
    HeadTheoryAtom(TheoryAtom atom) : atom{std::move(atom)} {}
    void print(std::ostream &out) const override { out << atom; }
    TheoryAtom atom;
};

struct HeadAggregate : HeadLiteral {
    using Element = std::tuple<UTermVec, ULiteral, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    HeadAggregate(AggregateFunction fun, ElementVec elems) : fun(fun), elements(std::move(elems)) {}
    HeadAggregate(AggregateFunction fun, ElementVec elems, Relation rel, UTerm rhs)
        : fun(fun), elements(std::move(elems)), right_guard(std::make_pair(rel, std::move(rhs))) {}
    void set_left_guard(UTerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        if (left_guard) {
            out << *left_guard->first << " " << left_guard->second << " ";
        }
        out << fun << " { " << p_range_with(elements, "; ", [](std::ostream &out, auto const &elem) {
            out << p_range{std::get<0>(elem), ","} << ": " << *std::get<1>(elem);
            if (!std::get<2>(elem).empty()) {
                out << ": " << p_range{std::get<2>(elem), ", "};
            }
        }) << (elements.empty() ? "}" : " }");
        if (right_guard) {
            out << " " << right_guard->first << " " << *right_guard->second;
        }
    }
    AggregateFunction fun;
    ElementVec elements;
    std::optional<std::pair<UTerm, Relation>> left_guard;
    std::optional<std::pair<Relation, UTerm>> right_guard;
};

using UHeadAggregate = std::unique_ptr<HeadAggregate>;

struct HeadSetAggregate : HeadLiteral {
    HeadSetAggregate(SetAggregate aggr) : aggr{std::move(aggr)} {}
    void set_left_guard(UTerm lhs, Relation rel) { aggr.set_left_guard(std::move(lhs), rel); }
    void print(std::ostream &out) const override { out << aggr; }
    SetAggregate aggr;
};

using UHeadSetAggregate = std::unique_ptr<HeadSetAggregate>;
