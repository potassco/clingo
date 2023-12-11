#include <util/algorithm.hh>

#include <input/algo/analyze.hh>
#include <input/algo/unpool_relations.hh>

#include "unpool.hh"

// TODO: remove
#include <iostream>

namespace Gringo::Input {

namespace {

struct NegateLiteral {
    template <class... T> auto operator()(Literal const &lit) const -> Literal { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) -> Literal { return LiteralBoolean{lit.loc, lit.sign, !lit.value}; }
    auto operator()(LiteralRelation const &lit) -> Literal {
        assert(lit.rhs.size() == 1);
        auto const &[rel, rhs] = lit.rhs.front();
        return LiteralRelation{lit.loc, lit.sign, lit.lhs, Util::make_vec<Guard>(Guard{complement(rel), rhs})};
    }
    auto operator()(LiteralSymbolic const &lit) -> Literal {
        return LiteralSymbolic{lit.loc, lit.sign + Sign::once, lit.term};
    }

    Sign sign;
};

struct UnpoolRelations {

    // protect ourselves -> no unintended overloads

    template <class T>
    auto operator()(T const &x, bool head) const -> std::optional<std::vector<std::vector<T>>> = delete;

    // literal

    auto operator()(Literal const &lit, bool head) const -> std::optional<LiteralVecVec> {
        return std::visit(*this, lit, std::variant<bool>{head});
    }

    auto operator()(LiteralBoolean const &lit, bool head) const -> std::optional<LiteralVecVec> {
        static_cast<void>(head);
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit, bool head) const -> std::optional<LiteralVecVec> {
        // TODO: the literal still has to be simplified
        if (lit.rhs.size() > 1) {
            auto conjunctive = head == (lit.sign == Sign::once);
            auto const *lhs = &lit.lhs;
            LiteralVecVec res;
            if (conjunctive) {
                res.emplace_back();
                res.back().reserve(lit.rhs.size());
            }
            for (auto const &rhs : lit.rhs) {
                if (!conjunctive) {
                    res.emplace_back();
                }
                auto rel = lit.sign == Sign::none ? rhs.first : complement(rhs.first);
                res.back().emplace_back(
                    LiteralRelation{lit.loc, Sign::none, *lhs, Util::make_vec<Guard>(Guard{rel, rhs.second})});
                lhs = &rhs.second;
            }
            return res;
        }
        return std::nullopt;
    }

    auto operator()(LiteralSymbolic const &lit, bool head) const -> std::optional<LiteralVecVec> {
        static_cast<void>(head);
        static_cast<void>(lit);
        return std::nullopt;
    }

    // conditional literal
    template <bool head> using HBLitVecVec = std::conditional_t<head, HeadLiteralVec, BodyLiteralVec>;

    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const -> std::optional<HBLitVecVec<!Conjunctive>> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // aggregate

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &lit) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // theory

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(atom);
        throw std::runtime_error("implement me!!!");
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteralVec> {
        if (auto res_lits = operator()(lit.lit, true); res_lits) {
            HeadLiteralVec head_lits;
            head_lits.reserve(res_lits->size());
            for (auto &lits : *res_lits) {
                assert(!lits.empty());
                if (lits.size() == 1) {
                    head_lits.emplace_back(SimpleHeadLiteral{std::move(lits.back())});
                } else {
                    ConditionalLiteralVec elems;
                    elems.reserve(lits.size());
                    for (auto &lit : lits) {
                        elems.emplace_back(location(lit), Util::make_vec<Literal>(std::move(lit)), LiteralVec{});
                    }
                    head_lits.emplace_back(Disjunction{location(lit.lit), std::move(elems)});
                }
            }
            return head_lits;
        }
        return std::nullopt;
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<BodyLiteralVec> {
        if (auto res_lits = operator()(lit.lit, false); res_lits) {
            BodyLiteralVec body_lits;
            body_lits.reserve(res_lits->size());
            for (auto &lits : *res_lits) {
                assert(!lits.empty());
                if (lits.size() == 1) {
                    body_lits.emplace_back(SimpleBodyLiteral{std::move(lits.back())});
                } else {
                    ConditionalLiteralVec elems;
                    elems.reserve(lits.size());
                    for (auto &lit : lits) {
                        elems.emplace_back(location(lit), Util::make_vec<Literal>(std::move(lit)), LiteralVec{});
                    }
                    body_lits.emplace_back(Conjunction{location(lit.lit), std::move(elems)});
                }
            }
            return body_lits;
        }
        return std::nullopt;
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteralVec> {
        // we now need the crossproduct here!!!
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<StatementVec> { return std::visit(*this, stm); }

    auto operator()(BodyLiteralVec const &lits) const -> std::optional<std::vector<BodyLiteralVec>> {
        return unpool_crossproduct(lits, *this);
    }

    auto operator()(Rule const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto head, auto body) -> Statement {
                if (auto *lit = std::get_if<Disjunction>(&head); lit != nullptr) {
                    for (auto &elem : lit->elems) {
                        if (elem.cond.empty()) {
                            for (auto &lit : elem.lits) {
                                if (is_test(lit)) {
                                    body.emplace_back(NegateLiteral{}(lit));
                                }
                            }
                        }
                    }
                }
                return Rule{stm.loc, std::move(head), std::move(body)};
            },
            *this, stm.head, stm.body);
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementShow const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementProject const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementScript const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementConst const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(Comment const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    RewriteContext const &ctx;
};

} // namespace

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Literal const &lit, bool head)
    -> std::optional<LiteralVecVec> {
    return UnpoolRelations{ctx}(lit, head);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteralVec> {
    // 0 < x < 9 :- B.
    // corresponds to:
    // 0 < x :- B.
    // x < 9 :- B.
    // corresponds to:
    // :- B, not 0 < x.
    // :- B, not x < 9.
    // corresponds to:
    // :- B, 0 >= x.
    // :- B, x >= 9.
    //
    // not 0 < x < 9 :- B.
    // corresponds to:
    // not 0 < x | not x < 9 :- B.
    // corresponds to:
    // :- B, 0 < x, x < 9.
    //
    // 0 < x < 9: C :- B.
    // corresponds to:
    // 0 < x: C :- B.
    // x < 9: C :- B.
    // looks like it is not possible to do more
    //
    // not 0 < x < 9: C :- B.
    // corresponds to:
    // (not 0 < x | not x < 9): C :- B.
    // corresponds to:
    // (0 >= x | x >= 9): C :- B.
    // looks like it is not possible to do more
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteralVec> {
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec> {
    return UnpoolRelations{ctx}(stm);
}

} // namespace Gringo::Input
