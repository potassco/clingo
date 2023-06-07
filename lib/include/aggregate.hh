#pragma once

#include <functional>
#include <optional>

#include <literal.hh>

enum class AggregateFunction {
    count,
    sum,
    sump,
    min,
    max,
};

auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream &;

using LGuard = std::optional<std::pair<STerm, Relation>>;
using RGuard = std::optional<std::pair<Relation, STerm>>;

struct UnpoolGuards {
    template <class T> static auto is_empty_value(std::optional<T> value) { return !value.has_value(); }
    static void unpool(PoolTerm &pool, LGuard &lhs) {
        if (lhs.has_value()) {
            lhs->first->unpool(pool);
        }
    }
    static void unpool(PoolTerm &pool, RGuard &rhs) {
        if (rhs.has_value()) {
            rhs->second->unpool(pool);
        }
    }
    static auto equal(STerm &term, LGuard &lhs) { return lhs.has_value() && term == lhs->first; }
    static auto equal(STerm &term, RGuard &rhs) { return rhs.has_value() && term == rhs->second; }
};

class SetAggregate {
  public:
    using Element = std::pair<SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    SetAggregate(ElementVec elems) : elems_{std::move(elems)} {}
    SetAggregate(ElementVec elems, Relation rel, STerm rhs)
        : elems_{std::move(elems)}, rhs_(std::make_pair(rel, std::move(rhs))) {}

    void set_rhs(STerm lhs, Relation rel);
    void unpool(PoolLiteral &pool, std::function<void(std::optional<SetAggregate>)> cb);

    friend auto operator<<(std::ostream &out, SetAggregate const &aggr) -> std::ostream &;

  private:
    ElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};
