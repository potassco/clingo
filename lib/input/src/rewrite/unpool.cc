#include "unpool.hh"

#include <clingo/input/print.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/simplify.hh>
#include <clingo/input/rewrite/substitute.hh>
#include <clingo/input/rewrite/unpool.hh>
#include <clingo/input/rewrite/visit_variables.hh>

#include <clingo/util/optional.hh>
#include <clingo/util/type_traits.hh>

#include <algorithm>

namespace CppClingo::Input {

namespace {

class LiteralToTuple {
  public:
    LiteralToTuple() = default;
    LiteralToTuple(LiteralToTuple const &) = delete;
    auto operator=(LiteralToTuple const &) -> LiteralToTuple & = delete;

    auto operator()(Lit const &orig, Lit const &lit) const -> std::vector<Term> {
        return std::visit(*this, std::variant<std::reference_wrapper<Lit const>>{orig}, lit);
    }

    [[nodiscard]] auto tuple_from_vars(Lit const &orig) const -> std::vector<Term> {
        auto var_set = select_variables(orig);
        auto var_vec = VariableVec(var_set.begin(), var_set.end());
        std::ranges::sort(var_vec);
        std::vector<Term> res;
        res.reserve(var_vec.size() + 1);
        res.emplace_back(TermSymbol{location(orig), CppClingo::SymbolStore::num_ref(n)});
        for (auto const &var : var_vec) {
            res.emplace_back(TermVariable{location(orig), var});
        }
        return res;
    }

    auto operator()(Lit const &orig, [[maybe_unused]] LitBool const &lit) const -> std::vector<Term> {
        return tuple_from_vars(orig);
    }

    auto operator()(Lit const &orig, [[maybe_unused]] LitComparison const &lit) const -> std::vector<Term> {
        return tuple_from_vars(orig);
    }

    auto operator()([[maybe_unused]] Lit const &orig, LitSymbolic const &lit) const -> std::vector<Term> {
        std::vector<Term> res;
        res.reserve(2);
        int i = 0;
        switch (lit.sign()) {
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
        res.emplace_back(TermSymbol{lit.loc(), CppClingo::SymbolStore::num_ref(i)});
        res.emplace_back(lit.term());
        return res;
    }

    void next() { ++n; }

  private:
    int n = 2;
};

class Unpool {
  public:
    explicit Unpool(RewriteContext &ctx) : ctx_{&ctx} {}

    // terms

    auto operator()(Term const &term) const -> std::optional<std::vector<Term>> { return std::visit(*this, term); }

    auto operator()(std::optional<Term> const &term) const -> std::optional<std::vector<std::optional<Term>>> {
        return Util::and_then(term, [this](Term const &term) {
            return Util::transform_vec(operator()(term), [](Term term) { return std::make_optional(std::move(term)); });
        });
    }

    auto operator()(TermArray const &terms) const -> std::optional<std::vector<std::vector<Term>>> {
        return unpool_crossproduct(terms, *this);
    }

    auto operator()([[maybe_unused]] TermSymbol const &term) const -> std::optional<std::vector<Term>> {
        return std::nullopt;
    }

    auto operator()([[maybe_unused]] TermVariable const &term) const -> std::optional<std::vector<Term>> {
        return std::nullopt;
    }

    auto operator()([[maybe_unused]] TermFormatString const &term) const -> std::optional<std::vector<Term>> {
        return std::nullopt;
    }

    auto operator()(Argument const &elem) const -> std::optional<std::vector<Argument>> {
        return std::visit(
            [this]<class T>(T const &x) -> std::optional<std::vector<Argument>> {
                if constexpr (std::is_same_v<T, Term>) {
                    return Util::transform_vec(operator()(x), [](auto term) { return Argument{std::move(term)}; });
                }
                if constexpr (std::is_same_v<T, Projection>) {
                    return std::nullopt;
                }
            },
            elem);
    }

    auto operator()(TupleElement const &tuple_or_term) const -> std::optional<TupleElementArray> {
        return std::visit(
            [this]<class T>(T const &x) -> std::optional<TupleElementArray> {
                if constexpr (std::is_same_v<T, Term>) {
                    return Util::transform_vec(operator()(x), [](auto term) { return TupleElement{std::move(term)}; });
                }
                if constexpr (std::is_same_v<T, ArgumentTuple>) {
                    return Util::transform_vec(unpool_crossproduct(x.elems(), *this), [](auto tuple) {
                        return TupleElement{ArgumentTuple{std::move(tuple)}};
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
            elems.emplace(term.pool().begin(), term.pool().end());
        }
        return Util::transform_vec(std::move(elems), [&term](auto elem) -> Term {
            return std::visit(
                [&term]<class T>(T x) -> Term {
                    if constexpr (std::is_same_v<T, Term>) {
                        return x;
                    }
                    if constexpr (std::is_same_v<T, ArgumentTuple>) {
                        return term.update(a_pool = Util::make_immutable_array<TupleElement>(std::move(x)));
                    }
                },
                std::move(elem));
        });
    }

    auto operator()(TermFunction const &term) const -> std::optional<std::vector<Term>> {
        auto elems = unpool_union(term.pool(), [this](ArgumentTuple const &tuple) {
            // unpool the elements
            return unpool_crossproduct(tuple.elems(), *this);
        });

        if (!elems && term.pool().size() != 1) {
            elems.emplace(term.pool().begin(), term.pool().end());
        }

        return Util::transform_vec(std::move(elems), [&term](auto elem) -> Term {
            // turn individual elements into function terms
            return term.update(a_pool = Util::make_immutable_array<ArgumentTuple>(std::move(elem)));
        });
    }

    auto operator()(TermAbs const &term) const -> std::optional<std::vector<Term>> {
        auto unpooled = unpool_union(term.pool(), *this);
        if (!unpooled.has_value() && term.pool().size() != 1) {
            unpooled.emplace(term.pool().begin(), term.pool().end());
        }
        return Util::transform_vec(std::move(unpooled), [&term](auto arg) -> Term {
            return term.update(a_pool = Util::make_immutable_array<Term>(std::move(arg)));
        });
    }

    auto operator()(TermUnary const &term) const -> std::optional<std::vector<Term>> {
        return Util::transform_vec(operator()(*term.rhs()),
                                   [&term](auto rhs) -> Term { return term.update(a_rhs = std::move(rhs)); });
    }

    auto operator()(Util::immutable_value<Term> const &term) const -> std::optional<std::vector<Term>> {
        return operator()(*term);
    }

    auto operator()(TermBinary const &term) const -> std::optional<std::vector<Term>> {
        return unpool_rewrite<Term>(term, *this, a_lhs, a_rhs);
    }

    auto operator()(GuardArray const &guards) const -> std::optional<std::vector<GuardArray>> {
        return Util::transform_vec(
            unpool_crossproduct(guards,
                                [this](Guard const &guard) {
                                    return Util::transform_vec(operator()(guard.second), [&guard](auto term) {
                                        return Guard{guard.first, std::move(term)};
                                    });
                                }),
            [](auto vec) { return GuardArray{std::move(vec)}; });
    }

    // literal

    auto operator()(Lit const &lit) const -> std::optional<std::vector<Lit>> { return std::visit(*this, lit); }

    auto operator()(LitArray const &lits) const -> std::optional<std::vector<LitArray>> {
        return Util::transform_vec(unpool_crossproduct(lits, *this), [](auto vec) { return LitArray(std::move(vec)); });
    }

    auto operator()([[maybe_unused]] LitBool const &lit) const -> std::optional<std::vector<Lit>> {
        return std::nullopt;
    }

    auto operator()(LitComparison const &lit) const -> std::optional<std::vector<Lit>> {
        return unpool_rewrite<Lit>(lit, *this, a_lhs, a_rhs);
    }

    auto operator()(LitSymbolic const &lit) const -> std::optional<std::vector<Lit>> {
        return Util::transform_vec(operator()(lit.term()),
                                   [&lit](auto term) -> Lit { return lit.update(a_term = std::move(term)); });
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
                std::vector<std::conditional_t<HasSign, BdLitAggregateElement, HdLitAggregateElement>> &elems) const {
        auto set_elems = unpool_rewrite<SetAggregateElement>(elem, *this, a_lit, a_cond);
        auto simplify_lit = [this, &to_tuple, &elem, &elems](SetAggregateElement const &unpooled) {
            auto guard = ctx_->push();
            auto res_subst = map_params(*ctx_, unpooled.lit());
            auto lit = std::move(res_subst).value_or(unpooled.lit());
            auto res_simp = simplify(HasSign ? SimplifyLiteralFlags::matchable
                                             : (SimplifyLiteralFlags::matchable | SimplifyLiteralFlags::unfailable),
                                     *ctx_, lit);
            lit = res_simp.value.value_or(std::move(lit));
            auto res_cond = Util::ResultVec{unpooled.cond()};
            res_cond.keep_all();
            for (auto &[lhs, rhs] : ctx_->aux()) {
                auto loc = location(lhs);
                auto rel = LitComparison{loc, Sign::none, std::move(lhs),
                                         Util::make_vec<Guard>(Guard{Relation::equal, std::move(rhs)})};

                res_cond.append(std::move(rel));
            }
            auto tuple = to_tuple(elem.lit(), lit);
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
        -> std::optional<std::vector<std::conditional_t<HasSign, BdLit, HdLit>>> {
        auto build = [this, &aggr](auto lhs, auto rhs) {
            std::vector<std::conditional_t<HasSign, BdLitAggregateElement, HdLitAggregateElement>> elems;
            auto to_tuple = LiteralToTuple{};
            for (auto &elem : aggr.elems()) {
                to_tuple.next();
                unpool_elem<HasSign>(to_tuple, elem, elems);
            }
            if constexpr (HasSign) {
                return BdLit{BdLitAggregate{aggr.loc(), aggr.sign(), std::move(lhs), AggregateFunction::count,
                                            std::move(elems), std::move(rhs)}};
            } else {
                return HdLit{HdLitAggregate{aggr.loc(), std::move(lhs), AggregateFunction::count, std::move(elems),
                                            std::move(rhs)}};
            }
        };
        auto ret = unpool_build(aggr, build, *this, a_lhs, a_rhs);
        if (!ret.has_value()) {
            ret = Util::make_vec<std::conditional_t<HasSign, BdLit, HdLit>>(build(aggr.lhs(), aggr.rhs()));
        }
        return ret;
    }

    // theory

    auto operator()(TheoryElement const &elem) const -> std::optional<std::vector<TheoryElement>> {
        return unpool_rewrite<TheoryElement>(elem, *this, a_cond);
    }

    auto operator()(TheoryElementArray const &elems) const -> std::optional<std::vector<TheoryElementArray>> {
        return Util::transform(unpool_union(elems, *this),
                               [](auto elems) { return Util::make_vec<TheoryElementArray>(std::move(elems)); });
    }

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const
        -> std::optional<std::vector<std::conditional_t<HasSign, BdLit, HdLit>>> {
        return unpool_rewrite<std::conditional_t<HasSign, BdLit, HdLit>>(atom, *this, a_name, a_elems);
    }

    // head literal

    auto operator()(HdLit const &lit) const -> std::optional<std::vector<HdLit>> { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> std::optional<std::vector<HdLit>> {
        return Util::transform_vec(operator()(lit.lit()),
                                   [](auto lit) -> HdLit { return HdLitSimple{std::move(lit)}; });
    }

    auto operator()(HdLitDisjunctionElement const &elem) const -> std::optional<HdLitDisjunctionElementArray> {
        return std::visit(
            [this]<class T>(T const &elem) -> std::optional<std::vector<HdLitDisjunctionElement>> {
                if constexpr (std::is_same_v<T, Lit>) {
                    return Util::transform_vec(operator()(elem),
                                               [](auto lit) { return HdLitDisjunctionElement{std::move(lit)}; });
                }
                return std::nullopt;
            },
            elem);
    }

    auto operator()(HdLitDisjunctionElementArray const &elems) const
        -> std::optional<std::vector<HdLitDisjunctionElementArray>> {
        return Util::transform_vec(unpool_crossproduct(elems, *this),
                                   [](auto vec) { return HdLitDisjunctionElementArray(std::move(vec)); });
    }

    auto operator()(HdLitDisjunction const &lit) const -> std::optional<std::vector<HdLit>> {
        auto unpool = [this](auto const &elems) {
            auto res_elems = Util::ResultVec{elems};
            for (auto const &elem : elems) {
                if (auto const *clit = std::get_if<CondLit>(&elem); clit != nullptr) {
                    if (auto res_clit = unpool_rewrite<HdLitDisjunctionElement>(*clit, *this, a_lit, a_cond);
                        res_clit) {
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

        if (auto res_pool = unpool_rewrite<HdLit>(lit, *this, a_elems); res_pool) {
            for (auto &lit : res_pool.value()) {
                auto const &disj = std::get<HdLitDisjunction>(lit);
                if (auto res_elems = unpool(disj.elems()); res_elems) {
                    lit = HdLitDisjunction{disj.loc(), std::move(res_elems).value()};
                }
            }
            return res_pool;
        }
        if (auto res_elems = unpool(lit.elems()); res_elems) {
            return Util::make_vec<HdLit>(HdLitDisjunction{lit.loc(), std::move(res_elems).value()});
        }
        return std::nullopt;
    }

    auto operator()(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElementArray> {
        return unpool_rewrite<HdLitAggregateElement>(elem, *this, a_tuple, a_lit, a_cond);
    }

    auto operator()(HdLitAggregateElementArray const &elems) const
        -> std::optional<std::vector<HdLitAggregateElementArray>> {
        return Util::transform(unpool_union(elems, *this),
                               [](auto elems) { return Util::make_vec<HdLitAggregateElementArray>(std::move(elems)); });
    }

    auto operator()(HdLitAggregate const &lit) const -> std::optional<std::vector<HdLit>> {
        return unpool_rewrite<HdLit>(lit, *this, a_lhs, a_elems, a_rhs);
    }

    // body literal

    auto operator()(BdLit const &lit) const -> std::optional<std::vector<BdLit>> { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> std::optional<std::vector<BdLit>> {
        return Util::transform_vec(operator()(lit.lit()),
                                   [](auto lit) -> BdLit { return BdLitSimple{std::move(lit)}; });
    }

    auto operator()([[maybe_unused]] BdLitConjunction const &lit) const -> std::optional<std::vector<BdLit>> {
        return std::nullopt;
    }

    auto operator()(BdLitArray const &lits) const -> std::optional<std::vector<BdLitArray>> {
        auto unpool = [this](auto const &lits) {
            auto res_lits = Util::ResultVec{lits};
            for (auto const &lit : lits) {
                if (auto const *conj = std::get_if<BdLitConjunction>(&lit); conj != nullptr) {
                    if (auto res_conj = unpool_rewrite<BdLitConjunction>(conj->lit(), *this, a_lit, a_cond); res_conj) {
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
            return std::vector<BdLitArray>{std::make_move_iterator(res_pool->begin()),
                                           std::make_move_iterator(res_pool->end())};
        }
        if (auto res_lits = unpool(lits); res_lits) {
            return Util::make_vec<BdLitArray>(std::move(res_lits).value());
        }
        return std::nullopt;
    }

    auto operator()(BdLitAggregateElement const &elem) const -> std::optional<BdLitAggregateElementArray> {
        return unpool_rewrite<BdLitAggregateElement>(elem, *this, a_tuple, a_cond);
    }

    auto operator()(BdLitAggregateElementArray const &elems) const
        -> std::optional<std::vector<BdLitAggregateElementArray>> {
        return Util::transform(unpool_union(elems, *this),
                               [](auto elems) { return Util::make_vec<BdLitAggregateElementArray>(std::move(elems)); });
    }

    auto operator()(BdLitAggregate const &aggr) const -> std::optional<std::vector<BdLit>> {
        return unpool_rewrite<BdLit>(aggr, *this, a_lhs, a_elems, a_rhs);
    }

    // statement

    auto operator()(Stm const &stm) const -> std::optional<StmVec> { return std::visit(*this, stm); }

    auto operator()(Edge const &edge) const -> std::optional<EdgeArray> {
        return unpool_rewrite<Edge>(edge, *this, a_src, a_dst);
    }

    auto operator()(EdgeArray const &edges) const { return unpool_union(edges, *this); }

    auto operator()(StmRule const &stm) const -> std::optional<StmVec> {
        return unpool_rewrite<Stm>(stm, *this, a_head, a_body);
    }

    auto operator()(OptimizeTuple const &tuple) const -> std::optional<std::vector<OptimizeTuple>> {
        return unpool_rewrite<OptimizeTuple>(tuple, *this, a_weight, a_prio, a_terms);
    }

    auto operator()(StmOptimize const &stm) const -> std::optional<StmVec> {
        StmVec stms;
        stms.reserve(stm.elems().size());
        for (auto const &elem : stm.elems()) {
            auto body = std::vector<BdLit>{};
            body.reserve(elem.cond().size());
            for (auto const &lit : elem.cond()) {
                body.emplace_back(BdLitSimple{lit});
            }
            auto tuple = stm.type() == OptimizeType::minimize
                             ? elem.tuple()
                             : OptimizeTuple{TermUnary{location(elem.tuple().weight()), UnaryOperator::minus,
                                                       elem.tuple().weight()},
                                             elem.tuple().prio(), elem.tuple().terms()};
            auto cons = StmWeakConstraint{stm.loc(), std::move(body), std::move(tuple)};
            if (auto opt_stms = operator()(cons); opt_stms.has_value()) {
                stms.insert(stms.end(), std::make_move_iterator(opt_stms->begin()),
                            std::make_move_iterator(opt_stms->end()));
            } else {
                stms.emplace_back(std::move(cons));
            }
        }
        return stms;
    }

    auto operator()(StmWeakConstraint const &stm) const -> std::optional<StmVec> {
        return unpool_rewrite<Stm>(stm, *this, a_body, a_tuple);
    }

    auto operator()(StmShow const &stm) const -> std::optional<StmVec> {
        return unpool_rewrite<Stm>(stm, *this, a_term, a_body);
    }

    auto operator()(StmProject const &stm) const -> std::optional<StmVec> {
        return unpool_rewrite<Stm>(stm, *this, a_atom, a_body);
    }

    auto operator()(StmExternal const &stm) const -> std::optional<StmVec> {
        return unpool_rewrite<Stm>(stm, *this, a_atom, a_body, a_type);
    }

    auto operator()(StmEdge const &stm) const -> std::optional<StmVec> {
        auto edges = Util::transform(operator()(stm.edges()), [](auto vec) { return EdgeArray{std::move(vec)}; });
        auto bodies = operator()(stm.body());
        if (stm.edges().size() != 1 || edges.has_value() || bodies.has_value()) {
            StmVec ret;
            for (auto &body : bodies.value_or(Util::make_vec<BdLitArray>(stm.body()))) {
                for (auto const &edge : edges.value_or(stm.edges())) {
                    ret.emplace_back(stm.update(a_edges = Util::make_vec<Edge>(edge), a_body = body));
                }
            }
            return ret;
        }
        return std::nullopt;
    }

    auto operator()(StmHeuristic const &stm) const -> std::optional<StmVec> {
        return unpool_rewrite<Stm>(stm, *this, a_atom, a_body, a_weight, a_prio, a_type);
    }

    auto operator()(StmConst const &stm) const -> std::optional<StmVec> {
        auto ret = unpool_rewrite<Stm>(stm, *this, a_value);
        if (ret.has_value() && ret->size() != 1) {
            throw std::runtime_error("const statements must not contain pools");
        }
        return ret;
    }

    template <class T> auto operator()([[maybe_unused]] T const &stm) const -> std::optional<StmVec> {
        static_assert(Util::is_among_v<T, StmTheory, StmShowNothing, StmShowSig, StmProjectSig, StmDefined, StmScript,
                                       StmInclude, StmProgram, StmParts, StmComment>);
        return std::nullopt;
    }

  private:
    RewriteContext *ctx_;
};

} // namespace

auto unpool(RewriteContext &ctx, Term const &term) -> std::optional<std::vector<Term>> {
    return Unpool{ctx}(term);
}

auto unpool(RewriteContext &ctx, Lit const &lit) -> std::optional<std::vector<Lit>> {
    return Unpool{ctx}(lit);
}

auto unpool(RewriteContext &ctx, HdLit const &lit) -> std::optional<std::vector<HdLit>> {
    return Unpool{ctx}(lit);
}

auto unpool(RewriteContext &ctx, BdLit const &lit) -> std::optional<std::vector<BdLit>> {
    return Unpool{ctx}(lit);
}

template <class F> struct print {
  public:
    print(F f) : f_{f} {}
    friend auto operator<<(std::ostream &out, print const &p) -> std::ostream & {
        p.f_(out);
        return out;
    }

  private:
    F f_;
};

auto unpool(RewriteContext &ctx, Stm const &stm) -> std::optional<StmVec> {
    auto stms = Unpool{ctx}(stm);
    // Note: minimize statements are rewritten into weak constraints here. This
    // makes all their (local) variables global. Hence, the test below will
    // fail and we must skip it. Another alternative implemenation could
    // perform the rewriting in a follow up step.
    if (stms.has_value() && !std::holds_alternative<StmOptimize>(stm)) {
        VariableSet global = select_variables(stm, VariableContext::global);
        for (auto const &unpooled : stms.value()) {
            if (!check_global(ctx.logger(), global, unpooled)) {
                ctx.set_error();
                return StmVec{};
            }
        }
    }
    return stms;
}

} // namespace CppClingo::Input
