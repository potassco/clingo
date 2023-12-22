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
        static_cast<void>(disj);
        throw std::logic_error("implement me!!!");
        /*
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
        */
    }
    return res_head;
}

auto rewrite_body(BodyLiteralVec const &body) -> Util::ResultVec<BodyLiteral> {
    auto res_body = Util::ResultVec{body};
    for (auto const &blit : body) {
        if (auto const *conj = std::get_if<Conjunction>(&blit); conj != nullptr) {
            static_cast<void>(conj);
            if (!body.empty()) {
                throw std::logic_error("implement me!!!");
            }
            /*
            for (auto const &elem : conj->lit) {
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
            */
        }
        // keep existing literal
        else {
            res_body.keep();
        }
    }
    return res_body;
}

auto unpool_conjunctive(LiteralVec const &lits) -> Util::ResultVec<Literal> {
    auto res_lits = Util::ResultVec{lits};
    for (auto const &lit : lits) {
        if (auto res = unpool_relations(lit, true); res) {
            res_lits.extend(std::make_move_iterator(res->begin()), std::make_move_iterator(res->end()));
        } else {
            res_lits.keep();
        }
    }
    return res_lits;
}

void append_conjunctive(auto &lits, Literal lit) {
    if (auto res = unpool_relations(lit, true); res) {
        lits.extend(std::make_move_iterator(res->begin()), std::make_move_iterator(res->end()));
    } else {
        lits.append(std::move(lit));
    }
}

auto shift(auto const &lit, auto &lits, bool negate) -> std::optional<Literal> {
    if (auto const *rel = std::get_if<LiteralRelation>(&lit); rel != nullptr) {
        append_conjunctive(lits, negate ? NegateLiteral{}(*rel) : Literal{*rel});
        return LiteralBoolean{location(lit), Sign::none, false};
    }
    return std::nullopt;
}

auto shift(TheoryElementVec const &elems) -> Util::ResultVec<TheoryElement> {
    auto res_elems = Util::ResultVec{elems};
    for (auto const &elem : elems) {
        if (auto res_cond = unpool_conjunctive(elem.second); res_cond) {
            res_elems.replace(TheoryElement{elem.first, std::move(res_cond).value()});
        } else {
            res_elems.keep();
        }
    }
    return res_elems;
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

template <class F, class U, class... Args>
auto rewrite_with_body(F &&build, U &&unpool, BodyLiteralVec const &body) -> std::optional<StatementVec> {
    auto rewrite = [&](auto const &body, bool not_null) -> std::optional<Statement> {
        if (auto res_body = rewrite_body(body); res_body || not_null) {
            return build(*std::move(res_body));
        }
        return std::nullopt;
    };
    return rewrite_statement(
        unpool_crossproducts([&](auto body) -> Statement { return *rewrite(std::move(body), true); }, unpool, body),
        rewrite, body, false);
}

struct ShiftHead {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteral> {
        return shift(lit.lit, body, true);
    }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteral> {
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto const &elem : lit.elems) {
            std::visit(
                [this, &res_elems](auto const &x) {
                    GRINGO_MATCH(x, Literal) {
                        if (auto res_lit = shift(x, body, true); res_lit) {
                            res_elems.remove();
                        } else {
                            res_elems.keep();
                        }
                    }
                    GRINGO_MATCH(x, ConditionalLiteral) {
                        auto res_cond = unpool_conjunctive(x.cond);
                        auto res_lit = shift(x.lit, res_cond, false);
                        if (res_lit || res_cond) {
                            res_elems.replace(ConditionalLiteral{x.loc, std::move(res_lit).value_or(x.lit),
                                                                 std::move(res_cond).value()});
                        } else {
                            res_elems.keep();
                        }
                    }
                },
                elem);
        }
        if (res_elems) {
            return Disjunction{lit.loc, std::move(res_elems).value()};
        }
        return std::nullopt;
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto const &elem : lit.elems) {
            auto res_cond = unpool_conjunctive(elem.cond);
            auto res_lit = shift(elem.lit, res_cond, false);
            if (res_lit || res_cond) {
                res_elems.replace(HeadAggregate::Element{elem.loc, elem.tuple, std::move(res_lit).value_or(elem.lit),
                                                         std::move(res_cond).value()});
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return HeadAggregate{lit.loc, lit.lhs, lit.fun, *std::move(res_elems), lit.rhs};
        }
        return std::nullopt;
    }

    auto operator()(HeadTheoryAtom const &atom) const -> std::optional<HeadLiteral> {
        if (auto res_elems = shift(atom.elems); res_elems) {
            return HeadTheoryAtom{atom.loc, atom.name, *std::move(res_elems), atom.rhs};
        }
        return std::nullopt;
    }

    Util::ResultVec<BodyLiteral> &body;
};

// TODO: shift body
// TODO: shift statement

struct UnpoolRelations {

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<std::vector<T>> = delete;

    // aggregate

    template <bool head> using HBLitVecVec = std::conditional_t<head, HeadLiteralVec, BodyLiteralVec>;

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &lit) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    // theory

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const -> std::optional<HBLitVecVec<!HasSign>> {
        auto res_elems = Util::ResultVec{atom.elems};
        auto unpool_lit = [](auto const &lit) { return unpool_relations(lit, true); };
        for (auto const &elem : atom.elems) {
            auto extend = [&elem, &res_elems](auto const &lits) {
                auto unpool = [](auto const &lits) {
                    return unpool_crossproduct(lits, [](auto const &lit) { return unpool_relations(lit, false); });
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

    // TODO: does not need to return a vector!!!

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteralVec> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteralVec> {
        auto unpool_elem = [](Disjunction::Element const &elem) {
            return std::visit(
                [](auto const &elem) -> std::optional<Disjunction::ElementVec> {
                    GRINGO_MATCH(elem, Literal) { return std::nullopt; }
                    GRINGO_MATCH(elem, ConditionalLiteral) {
                        auto build = [&elem](auto lits) -> Disjunction::Element {
                            return ConditionalLiteral{elem.loc, elem.lit, std::move(lits)};
                        };
                        auto unpool = [](auto const &lits) {
                            auto unpool = [](auto const &lit) { return unpool_relations(lit, false); };
                            return unpool_crossproduct(lits, unpool);
                        };
                        return unpool_crossproducts(build, unpool, elem.cond);
                    }
                },
                elem);
        };
        auto res_elems = unpool_union(lit.elems, unpool_elem);
        if (res_elems) {
            return Util::make_vec<HeadLiteral>(Disjunction{lit.loc, std::move(res_elems).value()});
        }
        return std::nullopt;
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        auto res_elems = Util::ResultVec{lit.elems};
        auto unpool_lit = [](auto const &lit) { return unpool_relations(lit, true); };
        for (auto const &elem : lit.elems) {
            auto res_elem = std::optional<HeadAggregate::Element>{};
            if (!is_atom(elem.lit) && !is_boolean(elem.lit)) {
                auto bool_lit = LiteralBoolean{elem.loc, Sign::none, true};
                auto cond = elem.cond;
                cond.emplace_back(elem.lit);
                res_elem.emplace(elem.loc, elem.tuple, std::move(bool_lit), std::move(cond));
            }
            auto const &elem_lit = res_elem.has_value() ? res_elem->lit : elem.lit;
            auto const &elem_cond = res_elem.has_value() ? res_elem->cond : elem.cond;
            auto extend = [&](auto const &lits) {
                auto unpool = [](auto const &lits) {
                    return unpool_crossproduct(lits, [](auto const &lit) { return unpool_relations(lit, false); });
                };
                auto build = [&](auto lits) {
                    return HeadAggregate::Element{elem.loc, elem.tuple, elem_lit, std::move(lits)};
                };
                if (auto res_elem = unpool_crossproducts(build, unpool, lits)) {
                    res_elems.extend(std::make_move_iterator(res_elem->begin()),
                                     std::make_move_iterator(res_elem->end()));
                    return true;
                }
                return false;
            };
            if (auto res_lits = unpool_union(elem_cond, unpool_lit); res_lits) {
                if (!extend(*res_lits)) {
                    res_elems.replace(elem.loc, elem.tuple, elem_lit, *std::move(res_lits));
                }
            } else if (!extend(elem_cond)) {
                if (res_elem) {
                    res_elems.replace(*std::move(res_elem));
                } else {
                    res_elems.keep();
                }
            }
        }
        if (res_elems) {
            return Util::make_vec<HeadLiteral>(
                HeadAggregate{lit.loc, lit.lhs, lit.fun, *std::move(res_elems), lit.rhs});
        }
        return std::nullopt;
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<BodyLiteralVec> {
        auto build = [](auto lit) -> BodyLiteral { return SimpleBodyLiteral{std::move(lit)}; };
        auto unpool = [](auto const &lit) { return unpool_relations(lit, false); };
        return unpool_crossproducts(build, unpool, lit.lit);
    }

    auto operator()(Conjunction const &lit) const -> std::optional<BodyLiteralVec> {
        static_cast<void>(lit);
        throw std::logic_error("implement me!!!");
        /*
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
        */
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteralVec> {
        auto res_elems = Util::ResultVec{lit.elems};
        auto unpool_lit = [](auto const &lit) { return unpool_relations(lit, true); };
        for (auto const &elem : lit.elems) {
            auto extend = [&elem, &res_elems](auto const &lits) {
                auto unpool = [](auto const &lits) {
                    return unpool_crossproduct(lits, [](auto const &lit) { return unpool_relations(lit, false); });
                };
                auto build = [&elem](auto lits) {
                    return BodyAggregate::Element{elem.loc, elem.tuple, std::move(lits)};
                };
                if (auto res_elem = unpool_crossproducts(build, unpool, lits)) {
                    res_elems.extend(std::make_move_iterator(res_elem->begin()),
                                     std::make_move_iterator(res_elem->end()));
                    return true;
                }
                return false;
            };
            if (auto res_lits = unpool_union(elem.cond, unpool_lit); res_lits) {
                if (!extend(*res_lits)) {
                    res_elems.replace(elem.loc, elem.tuple, *std::move(res_lits));
                }
            } else if (!extend(elem.cond)) {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return Util::make_vec<BodyLiteral>(
                BodyAggregate{lit.loc, lit.sign, lit.lhs, lit.fun, *std::move(res_elems), lit.rhs});
        }
        return std::nullopt;
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
        return std::nullopt;
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::runtime_error("unpool must be called before unpooling relations");
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement {
            return StatementWeakConstraint{stm.loc, std::move(body), stm.tuple};
        };
        return rewrite_with_body(build, *this, stm.body);
    }

    auto operator()(StatementShow const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementShow{stm.loc, stm.term, std::move(body)}; };
        return rewrite_with_body(build, *this, stm.body);
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementProject{stm.loc, stm.term, std::move(body)}; };
        return rewrite_with_body(build, *this, stm.body);
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement {
            return StatementExternal{stm.loc, stm.term, std::move(body), stm.type};
        };
        return rewrite_with_body(build, *this, stm.body);
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementEdge{stm.loc, stm.edges, std::move(body)}; };
        return rewrite_with_body(build, *this, stm.body);
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement {
            return StatementHeuristic{stm.loc, stm.atom, std::move(body), stm.type, stm.prio, stm.mod};
        };
        return rewrite_with_body(build, *this, stm.body);
    }

    auto operator()(StatementScript const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementConst const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(Comment const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    RewriteContext const &ctx;
};

} // namespace

[[nodiscard]] auto unpool_relations(Literal const &lit, bool conjunctive) -> std::optional<LiteralVec> {
    auto const *rel = std::get_if<LiteralRelation>(&lit);
    if (rel != nullptr && rel->rhs.size() > 1 && conjunctive == (rel->sign != Sign::once)) {
        auto const *lhs = &rel->lhs;
        LiteralVec res;
        for (auto const &rhs : rel->rhs) {
            auto cmp = rel->sign == Sign::none ? rhs.first : complement(rhs.first);
            res.emplace_back(
                LiteralRelation{rel->loc, Sign::none, *lhs, Util::make_vec<Guard>(Guard{cmp, rhs.second})});
            lhs = &rhs.second;
        }
        return res;
    }
    return std::nullopt;
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteralVec> {
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteralVec> {
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec> {
    return UnpoolRelations{ctx}(stm);
}

} // namespace Gringo::Input
