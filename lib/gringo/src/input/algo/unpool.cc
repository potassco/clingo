#include <algorithm>

#include <gringo/util/optional.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/simplify.hh>
#include <gringo/input/algo/substitute.hh>
#include <gringo/input/algo/unpool.hh>
#include <gringo/input/algo/visit_variables.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct LiteralToTuple {
    auto operator()(Literal const &orig, Literal const &lit) -> std::vector<Term> {
        return std::visit(*this, std::variant<std::reference_wrapper<Literal const>>{orig}, lit);
    }

    auto tuple_from_vars(Literal const &orig) -> std::vector<Term> {
        auto var_set = select_variables(orig);
        auto var_vec = VariableVec(var_set.begin(), var_set.end());
        std::sort(var_vec.begin(), var_vec.end());
        std::vector<Term> res;
        res.reserve(var_vec.size() + 1);
        res.emplace_back(TermSymbol{location(orig), store.num(n)});
        for (auto const &var : var_vec) {
            res.emplace_back(TermVariable{location(orig), var});
        }
        return res;
    }

    auto operator()(Literal const &orig, LiteralBoolean const &lit) -> std::vector<Term> {
        static_cast<void>(lit);
        return tuple_from_vars(orig);
    }

    auto operator()(Literal const &orig, LiteralRelation const &lit) -> std::vector<Term> {
        static_cast<void>(lit);
        return tuple_from_vars(orig);
    }

    auto operator()(Literal const &orig, LiteralSymbolic const &lit) -> std::vector<Term> {
        static_cast<void>(orig);
        std::vector<Term> res;
        res.reserve(2);
        int i = 0;
        switch (lit.sign_) {
            case Sign::none: {
                i = 0;
                break;
            }
            case Sign::once: {
                i = 1;
                break;
            }
            case Sign::twice: {
                i = 2;
                break;
            }
        }
        res.emplace_back(TermSymbol{lit.loc(), store.num(i)});
        res.emplace_back(lit.term_);
        return res;
    }

    void next() { ++n; }

    SymbolStore &store;
    int n = 2;
};

struct Unpool {

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<std::vector<T>> = delete;

    // terms

    auto operator()(Term const &term) const -> std::optional<std::vector<Term>> { return std::visit(*this, term); }

    auto operator()(std::optional<Term> const &term) const -> std::optional<std::vector<std::optional<Term>>> {
        return Util::and_then(term, [this](Term const &term) {
            return Util::transform_vec(operator()(term), [](Term term) { return std::make_optional(std::move(term)); });
        });
    }

    auto operator()(TermVec const &terms) const -> std::optional<std::vector<std::vector<Term>>> {
        return unpool_crossproduct(terms, *this);
    }

    auto operator()(TermSymbol const &term) const -> std::optional<std::vector<Term>> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<std::vector<Term>> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(ArgumentTuple::Element const &elem) const -> std::optional<std::vector<ArgumentTuple::Element>> {
        return std::visit(
            [this](auto const &x) -> std::optional<std::vector<ArgumentTuple::Element>> {
                GRINGO_MATCH(x, Term) {
                    return Util::transform_vec(operator()(x),
                                               [](auto term) { return ArgumentTuple::Element{std::move(term)}; });
                }
                GRINGO_MATCH(x, Projection) { return std::nullopt; }
            },
            elem);
    }

    auto operator()(TermTuple::Element const &tuple_or_term) const -> std::optional<TermTuple::ElementVec> {
        return std::visit(
            [this](auto const &x) -> std::optional<TermTuple::ElementVec> {
                GRINGO_MATCH(x, Term) {
                    return Util::transform_vec(operator()(x),
                                               [](auto term) { return TermTuple::Element{std::move(term)}; });
                }
                GRINGO_MATCH(x, ArgumentTuple) {
                    return Util::transform_vec(unpool_crossproduct(x.elems(), *this), [](auto tuple) {
                        return TermTuple::Element{ArgumentTuple{std::move(tuple)}};
                    });
                }
            },
            tuple_or_term);
    }

    auto operator()(TermTuple const &term) const -> std::optional<std::vector<Term>> {
        // unpool the elements
        auto elems = unpool_union(term.pool(), *this);

        // turn the elements into individual tuple terms or terms
        if (!elems.has_value() && (term.pool().size() != 1 || std::holds_alternative<Term>(term.pool().front()))) {
            elems = std::vector<TermTuple::Element>(term.pool().begin(), term.pool().end());
        }
        return Util::transform_vec(std::move(elems), [&term](auto elem) -> Term {
            return std::visit(
                [&term](auto x) -> Term {
                    GRINGO_MATCH(x, Term) { return x; }
                    GRINGO_MATCH(x, ArgumentTuple) { return TermTuple{term.loc(), {ArgumentTuple{std::move(x)}}}; }
                },
                std::move(elem));
        });
    }

    auto operator()(TermFunction const &term) const -> std::optional<std::vector<Term>> {
        auto elems = unpool_union(term.pool(), [this](ArgumentTuple const &tuple) {
            // unpool the elements
            return unpool_crossproduct(tuple.elems(), *this);
        });

        if (!elems.has_value() && term.pool().size() != 1) {
            elems = PoolVec(term.pool().begin(), term.pool().end());
        }

        return Util::transform_vec(std::move(elems), [&term](auto elem) -> Term {
            // turn individual elements into function terms
            return TermFunction{term.loc(), term.name(), PoolVec{std::move(elem)}, term.external()};
        });
    }

    auto operator()(TermAbs const &term) const -> std::optional<std::vector<Term>> {
        auto unpooled = unpool_union(term.pool(), *this);
        if (!unpooled.has_value() && term.pool().size() != 1) {
            unpooled = std::vector<Term>(term.pool().begin(), term.pool().end());
        }
        return Util::transform_vec(std::move(unpooled), [&term](auto arg) -> Term {
            return TermAbs{term.loc(), TermVec{std::move(arg)}};
        });
    }

    auto operator()(TermUnary const &term) const -> std::optional<std::vector<Term>> {
        return Util::transform_vec(operator()(term.rhs()), [&term](auto rhs) -> Term {
            return TermUnary{term.loc(), term.op(), std::move(rhs)};
        });
    }

    auto operator()(TermBinary const &term) const -> std::optional<std::vector<Term>> {
        return unpool_crossproducts(
            [&term](auto lhs, auto rhs) -> Term {
                return TermBinary{term.loc(), std::move(lhs), term.op(), std::move(rhs)};
            },
            *this, term.lhs(), term.rhs());
    }

    auto operator()(GuardVec const &guards) const -> std::optional<std::vector<GuardVec>> {
        return Util::transform_vec(
            unpool_crossproduct(guards,
                                [this](Guard const &guard) {
                                    return Util::transform_vec(operator()(guard.second), [&guard](auto term) {
                                        return Guard{guard.first, std::move(term)};
                                    });
                                }),
            [](auto vec) { return GuardVec{std::move(vec)}; });
    }

    // literal

    auto operator()(Literal const &lit) const -> std::optional<std::vector<Literal>> { return std::visit(*this, lit); }

    auto operator()(LiteralVec const &lits) const -> std::optional<std::vector<LiteralVec>> {
        return Util::transform_vec(unpool_crossproduct(lits, *this),
                                   [](auto vec) { return LiteralVec{std::move(vec)}; });
    }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<std::vector<Literal>> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit) const -> std::optional<std::vector<Literal>> {
        return unpool_crossproducts(
            [&lit](auto lhs, auto rhs) -> Literal {
                return LiteralRelation{lit.loc(), lit.sign_, std::move(lhs), std::move(rhs)};
            },
            *this, lit.lhs_, lit.rhs_);
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<std::vector<Literal>> {
        return Util::transform_vec(operator()(lit.term_), [&lit](auto term) -> Literal {
            return LiteralSymbolic{lit.loc(), lit.sign_, std::move(term)};
        });
    }

    // set aggregate

    auto operator()(LGuard const &lhs) const -> std::optional<std::vector<LGuard>> {
        return Util::and_then(lhs, [this](auto const &lhs) {
            return Util::transform_vec(operator()(lhs.first), [&lhs](auto term) {
                return std::make_optional<LGuard::value_type>(std::move(term), lhs.second);
            });
        });
    }

    auto operator()(RGuard const &rhs) const -> std::optional<std::vector<RGuard>> {
        return Util::and_then(rhs, [this](auto const &rhs) {
            return Util::transform_vec(operator()(rhs.second), [&rhs](auto term) {
                return std::make_optional<RGuard::value_type>(rhs.first, std::move(term));
            });
        });
    }

    //! Unpool a set aggregate element.
    //!
    //! Note that this function rewrites into a tuple aggregate
    //! because set aggregates cannot represent unpooled relation literals (nicely).
    //! To be able to rewrite, the literal of the element is simplified first.
    template <bool HasSign>
    void
    unpool_elem(LiteralToTuple &to_tuple, SetAggregateElement const &elem,
                std::vector<typename std::conditional_t<HasSign, BodyAggregate, HeadAggregate>::Element> &elems) const {
        auto set_elems = unpool_crossproducts(
            [&elem](auto lit, auto cond) {
                return SetAggregateElement{elem.loc(), std::move(lit), std::move(cond)};
            },
            *this, elem.lit_, elem.cond_);
        auto simplify_lit = [this, &to_tuple, &elem, &elems](SetAggregateElement unpooled) {
            auto guard = ctx.push();
            auto res_subst = map_params(ctx, unpooled.lit_);
            auto lit = std::move(res_subst).value_or(std::move(unpooled.lit_));
            auto res_simp = simplify(HasSign ? SimplifyLiteralFlags::matchable
                                             : (SimplifyLiteralFlags::matchable | SimplifyLiteralFlags::unfailable),
                                     ctx, lit);
            lit = res_simp.value.value_or(std::move(lit));
            auto res_cond = Util::ResultVec{unpooled.cond_};
            res_cond.keep_all();
            for (auto &[lhs, rhs] : ctx.aux()) {
                auto loc = location(lhs);
                auto rel = LiteralRelation{loc, Sign::none, std::move(lhs),
                                           Util::make_vec<Guard>(Guard{Relation::equal, std::move(rhs)})};

                res_cond.append(std::move(rel));
            }
            auto tuple = to_tuple(elem.lit_, lit);
            if constexpr (HasSign) {
                res_cond.append(std::move(lit));
                elems.emplace_back(elem.loc(), std::move(tuple), std::move(res_cond).value());
            } else {
                elems.emplace_back(elem.loc(), std::move(tuple), std::move(lit), std::move(res_cond).value());
            }
        };
        if (set_elems.has_value()) {
            for (auto &unpooled : set_elems.value()) {
                simplify_lit(std::move(unpooled));
            }
        } else {
            simplify_lit(elem);
        }
    }

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &aggr) const
        -> std::optional<std::vector<std::conditional_t<HasSign, BodyLiteral, HeadLiteral>>> {
        auto convert = [this, &aggr](auto lhs, auto rhs) {
            std::vector<typename std::conditional_t<HasSign, BodyAggregate, HeadAggregate>::Element> elems;
            LiteralToTuple to_tuple{ctx.store()};
            for (auto &elem : aggr.elems_) {
                to_tuple.next();
                unpool_elem<HasSign>(to_tuple, elem, elems);
            }
            if constexpr (HasSign) {
                return BodyLiteral{BodyAggregate{aggr.loc(), aggr.sign_, std::move(lhs), AggregateFunction::count,
                                                 std::move(elems), std::move(rhs)}};
            } else {
                return HeadLiteral{HeadAggregate{aggr.loc(), std::move(lhs), AggregateFunction::count, std::move(elems),
                                                 std::move(rhs)}};
            }
        };
        auto ret = unpool_crossproducts(convert, *this, aggr.lhs_, aggr.rhs_);
        if (!ret.has_value()) {
            ret = std::vector<std::conditional_t<HasSign, BodyLiteral, HeadLiteral>>{};
            ret->emplace_back(convert(aggr.lhs_, aggr.rhs_));
        }
        return ret;
    }

    // theory

    auto operator()(TheoryElement const &elem) const -> std::optional<std::vector<TheoryElement>> {
        return unpool_crossproducts(
            [&elem](auto cond) {
                return TheoryElement{elem.loc(), elem.tuple_, std::move(cond)};
            },
            *this, elem.cond_);
    }

    auto operator()(TheoryElementVec const &elems) const -> std::optional<std::vector<TheoryElementVec>> {
        return Util::transform(unpool_union(elems, *this),
                               [](auto elems) { return Util::make_vec<TheoryElementVec>(std::move(elems)); });
    }

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const
        -> std::optional<std::vector<std::conditional_t<HasSign, BodyLiteral, HeadLiteral>>> {
        return unpool_crossproducts(
            [&atom](auto name, auto elems) {
                if constexpr (HasSign) {
                    return BodyLiteral{
                        TheoryAtom<HasSign>{atom.loc(), atom.sign_, std::move(name), std::move(elems), atom.rhs_}};
                } else {
                    return HeadLiteral{TheoryAtom<HasSign>{atom.loc(), std::move(name), std::move(elems), atom.rhs_}};
                }
            },
            *this, atom.name_, atom.elems_);
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<std::vector<HeadLiteral>> {
        return std::visit(*this, lit);
    }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<std::vector<HeadLiteral>> {
        return Util::transform_vec(operator()(lit.lit_),
                                   [](auto lit) -> HeadLiteral { return SimpleHeadLiteral{std::move(lit)}; });
    }

    auto operator()(Disjunction::Element const &elem) const -> std::optional<Disjunction::ElementVec> {
        return std::visit(
            [this](auto const &elem) -> std::optional<std::vector<Disjunction::Element>> {
                GRINGO_MATCH(elem, Literal) {
                    return unpool_crossproducts([](auto lit) { return Disjunction::Element{std::move(lit)}; }, *this,
                                                elem);
                }
                return std::nullopt;
            },
            elem);
    }

    auto operator()(Disjunction::ElementVec const &elems) const -> std::optional<std::vector<Disjunction::ElementVec>> {
        return Util::transform_vec(unpool_crossproduct(elems, *this),
                                   [](auto vec) { return Disjunction::ElementVec{std::move(vec)}; });
    }

    auto operator()(Disjunction const &lit) const -> std::optional<std::vector<HeadLiteral>> {
        auto unpool = [this](auto const &elems) {
            auto res_elems = Util::ResultVec{elems};
            for (auto const &elem : elems) {
                if (auto const *clit = std::get_if<ConditionalLiteral>(&elem); clit != nullptr) {
                    auto build = [clit](auto lit, auto elem) -> Disjunction::Element {
                        return ConditionalLiteral{clit->loc(), std::move(lit), std::move(elem)};
                    };
                    auto res_clit = unpool_crossproducts(build, *this, clit->lit_, clit->cond_);
                    if (res_clit) {
                        res_elems.remove();
                        res_elems.extend(std::make_move_iterator(res_clit->begin()),
                                         std::make_move_iterator(res_clit->end()));
                    } else {
                        res_elems.keep();
                    }
                } else {
                    res_elems.keep();
                }
            }
            return res_elems;
        };
        auto res_pool = unpool_crossproducts(
            [&lit](auto elems) -> HeadLiteral {
                return Disjunction{lit.loc(), std::move(elems)};
            },
            *this, lit.elems_);
        if (res_pool) {
            for (auto &lit : res_pool.value()) {
                auto &elems = std::get<Disjunction>(lit).elems_;
                if (auto res_elems = unpool(elems); res_elems) {
                    elems = std::move(res_elems).value();
                }
            }
            return res_pool;
        }
        if (auto res_elems = unpool(lit.elems_); res_elems) {
            return Util::make_vec<HeadLiteral>(Disjunction{lit.loc(), std::move(res_elems).value()});
        }
        return std::nullopt;
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::ElementVec> {
        return unpool_crossproducts(
            [&elem](auto tuple, auto lit, auto cond) {
                return HeadAggregate::Element{elem.loc(), std::move(tuple), std::move(lit), std::move(cond)};
            },
            *this, elem.tuple_, elem.lit_, elem.cond_);
    }

    auto operator()(HeadAggregate::ElementVec const &elems) const
        -> std::optional<std::vector<HeadAggregate::ElementVec>> {
        return Util::transform(unpool_union(elems, *this),
                               [](auto elems) { return Util::make_vec<HeadAggregate::ElementVec>(std::move(elems)); });
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<std::vector<HeadLiteral>> {
        return unpool_crossproducts(
            [&lit](auto lhs, auto elems, auto rhs) -> HeadLiteral {
                return HeadAggregate{lit.loc(), std::move(lhs), lit.fun_, std::move(elems), std::move(rhs)};
            },
            *this, lit.lhs_, lit.elems_, lit.rhs_);
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<std::vector<BodyLiteral>> {
        return std::visit(*this, lit);
    }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<std::vector<BodyLiteral>> {
        return Util::transform_vec(operator()(lit.lit_),
                                   [](auto lit) -> BodyLiteral { return SimpleBodyLiteral{std::move(lit)}; });
    }

    auto operator()(Conjunction const &lit) const -> std::optional<std::vector<BodyLiteral>> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(BodyLiteralVec const &lits) const -> std::optional<std::vector<BodyLiteralVec>> {
        auto unpool = [this](auto const &lits) {
            auto res_lits = Util::ResultVec{lits};
            for (auto const &lit : lits) {
                if (auto const *conj = std::get_if<Conjunction>(&lit); conj != nullptr) {
                    auto build = [conj](auto lit, auto elem) -> BodyLiteral {
                        return Conjunction{ConditionalLiteral{conj->lit_.loc(), std::move(lit), std::move(elem)}};
                    };
                    auto res_conj = unpool_crossproducts(build, *this, conj->lit_.lit_, conj->lit_.cond_);
                    if (res_conj) {
                        res_lits.remove();
                        res_lits.extend(std::make_move_iterator(res_conj->begin()),
                                        std::make_move_iterator(res_conj->end()));
                    } else {
                        res_lits.keep();
                    }
                } else {
                    res_lits.keep();
                }
            }
            return res_lits;
        };
        if (auto res_pool = unpool_crossproduct(lits, *this); res_pool) {
            for (auto &lits : res_pool.value()) {
                if (auto res_lits = unpool(lits); res_lits) {
                    lits = std::move(res_lits).value();
                }
            }
            return std::vector<BodyLiteralVec>{std::make_move_iterator(res_pool->begin()),
                                               std::make_move_iterator(res_pool->end())};
        }
        if (auto res_lits = unpool(lits); res_lits) {
            return Util::make_vec<BodyLiteralVec>(std::move(res_lits).value());
        }
        return std::nullopt;
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::ElementVec> {
        return unpool_crossproducts(
            [&elem](auto tuple, auto cond) {
                return BodyAggregate::Element{elem.loc(), std::move(tuple), std::move(cond)};
            },
            *this, elem.tuple_, elem.cond_);
    }

    auto operator()(BodyAggregate::ElementVec const &elems) const
        -> std::optional<std::vector<BodyAggregate::ElementVec>> {
        return Util::transform(unpool_union(elems, *this),
                               [](auto elems) { return Util::make_vec<BodyAggregate::ElementVec>(std::move(elems)); });
    }

    auto operator()(BodyAggregate const &aggr) const -> std::optional<std::vector<BodyLiteral>> {
        return unpool_crossproducts(
            [&aggr](auto lhs, auto elems, auto rhs) -> BodyLiteral {
                return BodyAggregate{aggr.loc(), aggr.sign_,       std::move(lhs),
                                     aggr.fun_,  std::move(elems), std::move(rhs)};
            },
            *this, aggr.lhs_, aggr.elems_, aggr.rhs_);
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<StatementVec> { return std::visit(*this, stm); }

    auto operator()(StatementEdge::Edge const &edge) const -> std::optional<StatementEdge::EdgeVec> {
        return unpool_crossproducts(
            [](auto u, auto v) {
                return StatementEdge::Edge{std::move(u), std::move(v)};
            },
            *this, edge.u_, edge.v_);
    }

    auto operator()(StatementEdge::EdgeVec const &edges) const { return unpool_union(edges, *this); }

    auto operator()(Rule const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto head, auto body) -> Statement {
                return Rule{stm.loc(), std::move(head), std::move(body)};
            },
            *this, stm.head_, stm.body_);
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementWeakConstraint::Tuple const &tuple) const
        -> std::optional<std::vector<StatementOptimize::Tuple>> {
        return unpool_crossproducts(
            [](auto weight, auto prio, auto terms) {
                return StatementOptimize::Tuple{std::move(weight), std::move(prio), std::move(terms)};
            },
            *this, tuple.weight_, tuple.priority_, tuple.terms_);
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<StatementVec> {
        StatementVec stms;
        stms.reserve(stm.elems_.size());
        for (auto const &elem : stm.elems_) {
            auto body = std::vector<BodyLiteral>{};
            body.reserve(elem.second.size());
            for (auto const &lit : elem.second) {
                body.emplace_back(SimpleBodyLiteral{lit});
            }
            auto tuple = elem.first;
            if (stm.type_ == OptimizeType::maximize) {
                tuple.weight_ = TermUnary{location(tuple.weight_), UnaryOperator::negate, std::move(tuple.weight_)};
            }
            auto cons = StatementWeakConstraint{stm.loc(), std::move(body), std::move(tuple)};
            if (auto opt_stms = operator()(cons); opt_stms.has_value()) {
                stms.insert(stms.end(), std::make_move_iterator(opt_stms->begin()),
                            std::make_move_iterator(opt_stms->end()));
            } else {
                stms.emplace_back(std::move(cons));
            }
        }
        return stms;
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto body, auto tuple) -> Statement {
                return StatementWeakConstraint{stm.loc(), std::move(body), std::move(tuple)};
            },
            *this, stm.body_, stm.tuple_);
    }

    auto operator()(StatementShow const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto term, auto body) -> Statement {
                return StatementShow{stm.loc(), std::move(term), std::move(body)};
            },
            *this, stm.term_, stm.body_);
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto term, auto body) -> Statement {
                return StatementProject{stm.loc(), std::move(term), std::move(body)};
            },
            *this, stm.term_, stm.body_);
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
        return unpool_crossproducts(
            [&stm](auto term, auto body, auto type) -> Statement {
                return StatementExternal{stm.loc(), std::move(term), std::move(body), std::move(type)};
            },
            *this, stm.term_, stm.body_, stm.type_);
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<StatementVec> {
        auto edges = operator()(stm.edges_);
        auto bodies = operator()(stm.body_);
        if (stm.edges_.size() != 1 || edges.has_value() || bodies.has_value()) {
            StatementVec ret;
            for (auto &body : bodies.value_or(Util::make_vec<BodyLiteralVec>(stm.body_))) {
                for (auto &edge : edges.value_or(stm.edges_)) {
                    ret.emplace_back(StatementEdge{stm.loc(), Util::make_vec<StatementEdge::Edge>(edge), body});
                }
            }
            return ret;
        }
        return std::nullopt;
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto atom, auto body, auto type, auto prio, auto mod) -> Statement {
                return StatementHeuristic{stm.loc(),       std::move(atom), std::move(body),
                                          std::move(type), std::move(prio), std::move(mod)};
            },
            *this, stm.atom_, stm.body_, stm.type_, stm.prio_, stm.mod_);
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
        auto ret = unpool_crossproducts(
            [&stm](auto value) -> Statement {
                return StatementConst{stm.loc(), stm.type_, stm.name_, std::move(value)};
            },
            *this, stm.value_);
        if (ret.has_value() && ret->size() != 1) {
            throw std::runtime_error("const statements must not contain pools");
        }
        return ret;
    }

    auto operator()(Comment const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    RewriteContext &ctx;
};

} // namespace

auto unpool(RewriteContext &ctx, Term const &term) -> std::optional<std::vector<Term>> { return Unpool{ctx}(term); }

auto unpool(RewriteContext &ctx, Literal const &lit) -> std::optional<std::vector<Literal>> { return Unpool{ctx}(lit); }

auto unpool(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<std::vector<HeadLiteral>> {
    return Unpool{ctx}(lit);
}

auto unpool(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<std::vector<BodyLiteral>> {
    return Unpool{ctx}(lit);
}

template <class F> struct print {
    print(F const &f) : f{f} {}
    friend auto operator<<(std::ostream &out, print const &p) -> std::ostream & {
        p.f(out);
        return out;
    }
    F const &f;
};

auto unpool(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec> {
    auto stms = Unpool{ctx}(stm);
    // Note: minimize statements are rewritten into weak constraints here. This
    // makes all their (local) variables global. Hence, the test below will
    // fail and we must skip it. Another alternative implemenation could
    // perform the rewriting in a follow up step.
    if (stms.has_value() && !std::holds_alternative<StatementOptimize>(stm)) {
        VariableSet global = select_variables(stm, VariableContext::global);
        for (auto const &unpooled : stms.value()) {
            if (!check_global(ctx.logger(), global, unpooled)) {
                return StatementVec{};
            }
        }
    }
    return stms;
}

} // namespace Gringo::Input
