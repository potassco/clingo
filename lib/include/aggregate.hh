#pragma once

#include <iostream>
#include <optional>

#include <util/print.hh>

#include <literal.hh>

enum class AggregateFunction {
    count,
    sum,
    sump,
    min,
    max,
};

inline auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream & {
    switch (fun) {
        case AggregateFunction::count: {
            out << "#count";
            break;
        }
        case AggregateFunction::sum: {
            out << "#sum";
            break;
        }
        case AggregateFunction::sump: {
            out << "#sum+";
            break;
        }
        case AggregateFunction::min: {
            out << "#min";
            break;
        }
        case AggregateFunction::max: {
            out << "#max";
            break;
        }
    }
    return out;
}

struct SetAggregate {
    using Element = std::pair<SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;
    SetAggregate(ElementVec elements) : elements{std::move(elements)} {}
    SetAggregate(ElementVec elements, Relation rel, STerm rhs)
        : elements{std::move(elements)}, right_guard(std::make_pair(rel, std::move(rhs))) {}
    void set_left_guard(STerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    friend auto operator<<(std::ostream &out, SetAggregate const &aggr) -> std::ostream & {
        if (aggr.left_guard) {
            out << *aggr.left_guard->first << " " << aggr.left_guard->second << " ";
        }
        out << "{ " << p_range_with(aggr.elements, "; ", [](std::ostream &out, auto const &elem) {
            out << *std::get<0>(elem);
            if (!std::get<1>(elem).empty()) {
                out << ": " << p_range{std::get<1>(elem), ", "};
            }
        }) << (aggr.elements.empty() ? "}" : " }");
        if (aggr.right_guard) {
            out << " " << aggr.right_guard->first << " " << *aggr.right_guard->second;
        }
        return out;
    }
    ElementVec elements;
    std::optional<std::pair<STerm, Relation>> left_guard;
    std::optional<std::pair<Relation, STerm>> right_guard;
};
