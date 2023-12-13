#include <util/algorithm.hh>
#include <util/optional.hh>

#include <input/algo/analyze.hh>
#include <input/algo/unpool_relations.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct NegateLiteral {
    auto operator()(Literal const &lit) const -> Literal { return std::visit(*this, lit); }
    auto operator()(LiteralBoolean const &lit) const -> Literal {
        return LiteralBoolean{lit.loc, lit.sign, !lit.value};
    }
    auto operator()(LiteralRelation const &lit) const -> Literal {
        assert(lit.rhs.size() == 1);
        auto const &[rel, rhs] = lit.rhs.front();
        return LiteralRelation{lit.loc, lit.sign, lit.lhs, Util::make_vec<Guard>(Guard{complement(rel), rhs})};
    }
    auto operator()(LiteralSymbolic const &lit) const -> Literal {
        return LiteralSymbolic{lit.loc, lit.sign + Sign::once, lit.term};
    }
};

auto rewrite_head(HeadLiteral const &head, Util::ResultVec<BodyLiteral> &body) -> std::optional<HeadLiteral> {
    std::optional<HeadLiteral> res_head;
    auto res = std::optional<std::pair<HeadLiteral, BodyLiteralVec>>{};
    if (auto const *lit = std::get_if<SimpleHeadLiteral>(&head); lit != nullptr) {
        if (!is_atom(lit->lit) && !is_boolean(lit->lit)) {
            body.append(NegateLiteral{}(lit->lit));
            res_head = SimpleHeadLiteral{LiteralBoolean{location(lit->lit), Sign::none, false}};
        }
    } else if (auto const *disj = std::get_if<Disjunction>(&head); disj != nullptr) {
        auto res_elems = Util::ResultVec{disj->elems};
        for (auto const &elem : disj->elems) {
            if (elem.cond.empty()) {
                auto res_lits = Util::ResultVec{elem.lits};
                for (auto const &lit : elem.lits) {
                    // Note: we should never get a boolean literal
                    // here. False literals are removed from the
                    // set of literals and true literals with an
                    // empty conditions would make the whole
                    // disjunction true.
                    if (!is_atom(lit)) {
                        res_lits.remove();
                        body.append(NegateLiteral{}(lit));
                    } else {
                        res_lits.keep();
                    }
                }
                if (res_lits->empty()) {
                    res_elems.remove();
                } else if (res_lits) {
                    res_elems.replace(ConditionalLiteral{elem.loc, *std::move(res_lits), {}});
                } else {
                    res_elems.keep();
                }
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            if (res_elems->empty()) {
                res_head = SimpleHeadLiteral{LiteralBoolean{disj->loc, Sign::none, false}};
            } else {
                res_head = Disjunction{disj->loc, *std::move(res_elems)};
            }
        }
    }
    return res_head;
}

auto rewrite_body(BodyLiteralVec const &body) -> Util::ResultVec<BodyLiteral> {
    auto res_body = Util::ResultVec{body};
    for (auto const &blit : body) {
        if (auto const *conj = std::get_if<Conjunction>(&blit)) {
            auto res_elems = Util::ResultVec{conj->elems};
            for (auto const &elem : conj->elems) {
                if (elem.cond.empty()) {
                    res_elems.remove();
                    for (auto const &lit : elem.lits) {
                        res_body.append(SimpleBodyLiteral{lit});
                    }
                } else {
                    res_elems.keep();
                }
            }
            // construct conjunctions from the modified elements
            if (res_elems) {
                res_body.remove();
                for (auto &elem : *std::move(res_elems)) {
                    res_body.append(Conjunction{conj->loc, Util::make_vec<ConditionalLiteral>(std::move(elem))});
                }
            }
            // construct conjunctions from the existing elements
            else if (conj->elems.size() != 1) {
                res_body.remove();
                for (auto const &elem : conj->elems) {
                    res_body.append(Conjunction{conj->loc, Util::make_vec<ConditionalLiteral>(elem)});
                }
            }
            // keep existing literal
            else {
                res_body.keep();
            }
        }
        // keep existing literal
        else {
            res_body.keep();
        }
    }
    return res_body;
}

template <class F, class... Args>
auto rewrite_statement(std::optional<StatementVec> res, F &&fun, Args &&...args) -> std::optional<StatementVec> {
    if (!res) {
        auto res_stm = std::invoke(std::forward<F>(fun), std::forward<Args>(args)...);
        if (res_stm) {
            return Util::make_vec<Statement>(*std::move(res_stm));
        }
    }
    return res;
}

struct UnpoolRelations {

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<std::vector<T>> = delete;
    template <class T> auto operator()(T const &x, bool conjunctive) const -> std::optional<std::vector<T>> = delete;

    // literal

    auto operator()(Literal const &lit, bool conjunctive) const -> std::optional<LiteralVec> {
        return std::visit(*this, lit, std::variant<bool>{conjunctive});
    }

    auto operator()(LiteralBoolean const &lit, bool conjunctive) const -> std::optional<LiteralVec> {
        static_cast<void>(conjunctive);
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit, bool conjunctive) const -> std::optional<LiteralVec> {
        // TODO: the literal still has to be simplified
        if (lit.rhs.size() > 1 && conjunctive == (lit.sign != Sign::once)) {
            auto const *lhs = &lit.lhs;
            LiteralVec res;
            for (auto const &rhs : lit.rhs) {
                auto rel = lit.sign == Sign::none ? rhs.first : complement(rhs.first);
                res.emplace_back(
                    LiteralRelation{lit.loc, Sign::none, *lhs, Util::make_vec<Guard>(Guard{rel, rhs.second})});
                lhs = &rhs.second;
            }
            return res;
        }
        return std::nullopt;
    }

    auto operator()(LiteralSymbolic const &lit, bool conjunctive) const -> std::optional<LiteralVec> {
        static_cast<void>(conjunctive);
        static_cast<void>(lit);
        return std::nullopt;
    }

    // conditional literal
    template <bool head> using HBLitVecVec = std::conditional_t<head, HeadLiteralVec, BodyLiteralVec>;

    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const -> std::optional<HBLitVecVec<!Conjunctive>> {
        auto unpool_elem = [this](ConditionalLiteral const &lit) {
            auto unpool = [&](auto const &lits) {
                // the hack would be avoidable by tagging
                bool conjunctive = &lits == &lit.lits ? !Conjunctive : false;
                return unpool_crossproduct(
                    lits, [this, conjunctive](auto const &lit) { return this->operator()(lit, conjunctive); });
            };
            auto build = [&lit](auto lits, auto cond) {
                return ConditionalLiteral{lit.loc, std::move(lits), std::move(cond)};
            };
            return unpool_crossproducts(build, unpool, lit.lits, lit.cond);
        };
        auto unpool_lits = [&](auto const &lits, bool conjunctive) {
            auto res_lits = Util::ResultVec{lits};
            for (auto &lit : lits) {
                if (auto res_lit = operator()(lit, conjunctive); res_lit) {
                    res_lits.remove();
                    for (auto &unpooled : *res_lit) {
                        res_lits.append(std::move(unpooled));
                    }
                } else {
                    res_lits.keep();
                }
            }
            return res_lits;
        };
        // expand horizontally
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto &elem : lit.elems) {
            auto res_lits = unpool_lits(elem.lits, Conjunctive);
            auto res_cond = unpool_lits(elem.cond, true);
            if (res_lits || res_cond) {
                res_elems.update(ConditionalLiteral{elem.loc, *std::move(res_lits), *std::move(res_cond)});
            } else {
                res_elems.keep();
            }
        }
        // expand horizontally
        auto unpooled = unpool_crossproduct(res_elems.value(), unpool_elem);
        if (res_elems || unpooled) {
            HBLitVecVec<!Conjunctive> res;
            if (!unpooled) {
                res.emplace_back(Junction<Conjunctive>{lit.loc, *std::move(res_elems)});
            } else {
                res.reserve(unpooled->size());
                for (auto &elems : *unpooled) {
                    res.emplace_back(Junction<Conjunctive>{lit.loc, std::move(elems)});
                }
            }
            return res;
        }
        return std::nullopt;
    }

    // aggregate

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &lit) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    // theory

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const -> std::optional<HBLitVecVec<!HasSign>> {
        auto res_elems = Util::ResultVec{atom.elems};
        auto unpool_lit = [this](auto const &lit) { return this->operator()(lit, true); };
        for (auto const &elem : atom.elems) {
            auto extend = [&elem, &res_elems, this](auto const &lits) {
                auto unpool = [this](auto const &lits) {
                    return unpool_crossproduct(lits, [this](auto const &lit) { return this->operator()(lit, false); });
                };
                auto build = [&elem](auto lits) { return TheoryElement{elem.first, std::move(lits)}; };
                if (auto res_elem = unpool_crossproducts(build, unpool, lits)) {
                    res_elems.extend(std::make_move_iterator(res_elem->begin()),
                                     std::make_move_iterator(res_elem->end()));
                    return true;
                }
                return false;
            };
            if (auto res_lits = unpool_union(elem.second, unpool_lit); res_lits) {
                if (!extend(*res_lits)) {
                    res_elems.replace(elem.first, *std::move(res_lits));
                }
            } else if (!extend(elem.second)) {
                res_elems.keep();
            }
        }
        if (res_elems) {
            if constexpr (HasSign) {
                return Util::make_vec<BodyLiteral>(
                    TheoryAtom<HasSign>{atom.loc, atom.sign, atom.name, *std::move(res_elems), atom.rhs});
            } else {
                return Util::make_vec<HeadLiteral>(
                    TheoryAtom<HasSign>{atom.loc, atom.name, *std::move(res_elems), atom.rhs});
            }
        }
        return std::nullopt;
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteralVec> {
        if (auto res_lits = operator()(lit.lit, false); res_lits) {
            ConditionalLiteralVec elems;
            elems.reserve(res_lits->size());
            for (auto &lit : *res_lits) {
                elems.emplace_back(location(lit), Util::make_vec<Literal>(std::move(lit)), LiteralVec{});
            }
            return Util::make_vec<HeadLiteral>(Disjunction{location(lit.lit), std::move(elems)});
        }
        if (auto res_lits = operator()(lit.lit, true); res_lits) {
            HeadLiteralVec head_lits;
            head_lits.reserve(res_lits->size());
            for (auto &lit : *res_lits) {
                head_lits.emplace_back(SimpleHeadLiteral{std::move(lit)});
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
        if (auto res_lits = operator()(lit.lit, true); res_lits) {
            ConditionalLiteralVec elems;
            elems.reserve(res_lits->size());
            for (auto &lit : *res_lits) {
                elems.emplace_back(location(lit), Util::make_vec<Literal>(std::move(lit)), LiteralVec{});
            }
            return Util::make_vec<BodyLiteral>(Conjunction{location(lit.lit), std::move(elems)});
        }
        if (auto res_lits = operator()(lit.lit, false); res_lits) {
            BodyLiteralVec body_lits;
            body_lits.reserve(res_lits->size());
            for (auto &lit : *res_lits) {
                body_lits.emplace_back(SimpleBodyLiteral{std::move(lit)});
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
        auto rewrite_rule = [&](auto const &rule, bool not_null) -> std::optional<Statement> {
            auto body = rewrite_body(rule.body);
            auto head = rewrite_head(rule.head, body);
            if (head || body || not_null) {
                return Rule{stm.loc, std::move(head).value_or(rule.head), *std::move(body)};
            }
            return std::nullopt;
        };
        return rewrite_statement(unpool_crossproducts(
                                     [&](auto head, auto body) -> Statement {
                                         return *rewrite_rule(Rule{stm.loc, std::move(head), std::move(body)}, true);
                                     },
                                     *this, stm.head, stm.body),
                                 rewrite_rule, stm, false);
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

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Literal const &lit, bool conjunctive)
    -> std::optional<LiteralVec> {
    return UnpoolRelations{ctx}(lit, conjunctive);
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
