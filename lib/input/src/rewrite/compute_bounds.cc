#include <clingo/input/rewrite/iesolver.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/compute_bounds.hh>

#include <clingo/util/type_traits.hh>
#include <utility>

namespace CppClingo::Input {

namespace {

//! Struct to indicate whether there is a relation literal (or interval) that asserts a bound.
struct BoundState {
    uint8_t lower : 1 = 0;
    uint8_t upper : 1 = 0;
    uint8_t both : 1 = 0;

    //! Check if the given bound is covered.
    [[nodiscard]] auto value(IEInterval::Type type) const -> bool {
        if (both == 1) {
            return true;
        }
        switch (type) {
            case IEInterval::Lower: {
                return lower == 1;
            }
            case IEInterval::Upper: {
                break;
            }
        }
        return upper == 1;
    }

    //! Mark a bound as covered.
    void set_value(IEInterval::Type type) {
        switch (type) {
            case IEInterval::Lower: {
                lower = 1;
                break;
            }
            case IEInterval::Upper: {
                upper = 1;
                break;
            }
        }
    }
};
using BoundStateMap = std::vector<BoundState>;

//! Compare two numbers according to the given relation and return a comparator.
[[nodiscard]] auto cmp(Number const &a, Relation rel, Number const &b) -> bool {
    auto cmp = compare(a, b);
    switch (rel) {
        case Relation::equal: {
            return cmp == 0;
        }
        case Relation::not_equal: {
            return cmp != 0;
        }
        case Relation::less: {
            return cmp < 0;
        }
        case Relation::less_equal: {
            return cmp <= 0;
        }
        case Relation::greater: {
            return cmp > 0;
        }
        case Relation::greater_equal: {
            break;
        }
    }
    return cmp >= 0;
}

//! Flip the given bound type.
[[nodiscard]] auto flip(IEInterval::Type type) -> IEInterval::Type {
    switch (type) {
        case IEInterval::Lower: {
            return IEInterval::Upper;
        }
        case IEInterval::Upper: {
            break;
        }
    }
    return IEInterval::Lower;
}

//! Extract (and classify) linear terms from terms.
class ExtractTerms {
  public:
    ExtractTerms(IETermVec &terms, bool add = true) : terms_{&terms}, add_{add} {}

    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()([[maybe_unused]] auto const &term) const -> bool { return false; }

    auto operator()(TermVariable const &term) const -> bool {
        auto x = IETerm{Number{1}, term.name()};
        if (!add_) {
            x.coefficient *= -1;
        }
        add_term(*terms_, std::move(x));
        return true;
    }

    auto operator()(TermSymbol const &term) const -> bool {
        if (term.value().type() == SymbolType::number) {
            auto x = IETerm{term.value().num(), String{}};
            if (!add_) {
                x.coefficient *= -1;
            }
            add_term(*terms_, std::move(x));
            return true;
        }
        return false;
    }

    auto operator()(TermUnary const &term) const -> bool {
        if (term.op() == UnaryOperator::minus) {
            return ExtractTerms{*terms_, !add_}(*term.rhs());
        }
        return false;
    }

    auto operator()(TermBinary const &term) const -> bool {
        switch (term.op()) {
            case BinaryOperator::minus: {
                return operator()(*term.lhs()) && ExtractTerms{*terms_, !add_}(*term.rhs());
            }
            case BinaryOperator::plus: {
                return operator()(*term.lhs()) && operator()(*term.rhs());
            }
            case BinaryOperator::times: {
                IETermVec lhs;
                IETermVec rhs;
                auto fixed_lhs = Number{0};
                auto fixed_rhs = Number{0};
                auto ret_lhs = ExtractTerms{lhs, true}(*term.lhs());
                auto ret_rhs = ExtractTerms{rhs, true}(*term.rhs());
                if (!ret_lhs && !ret_rhs) {
                    return false;
                }
                if (ret_lhs) {
                    fixed_lhs = simplify(lhs);
                }
                if (ret_rhs) {
                    fixed_rhs = simplify(rhs);
                }
                if (!ret_rhs) {
                    return lhs.empty() && fixed_lhs == 0;
                }
                if (!ret_lhs) {
                    return rhs.empty() && fixed_rhs == 0;
                }
                auto fixed = fixed_lhs * fixed_rhs;
                if (!add_) {
                    fixed *= -fixed;
                }
                add_term(*terms_, IETerm{std::move(fixed), String{}});
                if (!lhs.empty()) {
                    lhs.swap(rhs);
                    fixed_lhs.swap(fixed_rhs);
                }
                if (lhs.empty()) {
                    for (auto &x : rhs) {
                        x.coefficient *= fixed_lhs;
                        if (!add_) {
                            x.coefficient *= -1;
                        }
                        add_term(*terms_, std::move(x));
                    }
                    return true;
                }
                return false;
            }
            default: {
                return false;
            }
        }
        return true;
    }

  private:
    IETermVec *terms_;
    bool add_;
};

//! Extract inequalities from relation literals.
class ExtractInequalities {
  public:
    ExtractInequalities(IESolver &slv) : slv_{&slv} {}

    void operator()(Lit const &lit) const { std::visit(*this, lit); }

    void operator()([[maybe_unused]] auto const &lit) const {}

    void operator()(LitComparison const &lit) const {
        assert(lit.sign() == Sign::none);
        auto const &rhs = lit.rhs().front();
        int bound = 0;
        // handle intervals
        //   X = u..t -> X - u >= 0
        //               t - X >= 0
        if (is_variable(lit.lhs()) && is_interval(rhs.second)) {
            auto const &u = *std::get<TermBinary>(rhs.second).lhs();
            auto const &t = *std::get<TermBinary>(rhs.second).rhs();
            if (IETermVec terms; ExtractTerms{terms, true}(lit.lhs()) && ExtractTerms{terms, false}(u)) {
                slv_->add(IE{std::move(terms), bound});
            }
            if (IETermVec terms; ExtractTerms{terms, false}(lit.lhs()) && ExtractTerms{terms, true}(t)) {
                slv_->add(IE{std::move(terms), bound});
            }
            return;
        }
        // handle linear terms
        //   X >= Y -> X - Y >= 0
        //   X >  Y -> X - Y >= 1
        //   X <= Y -> Y - X >= 0
        //   X <  Y -> Y - X >= 1
        //   X =  Y -> X - Y >= 0
        //             Y - X >= 0
        //   X != Y -> cannot handle
        switch (rhs.first) {
            case Relation::greater: {
                bound = 1;
                [[fallthrough]];
            }
            case Relation::greater_equal: {
                if (IETermVec terms; ExtractTerms{terms, true}(lit.lhs()) && ExtractTerms{terms, false}(rhs.second)) {
                    slv_->add(IE{std::move(terms), bound});
                }
                break;
            }
            case Relation::less: {
                bound = 1;
                [[fallthrough]];
            }
            case Relation::less_equal: {
                if (IETermVec terms; ExtractTerms{terms, false}(lit.lhs()) && ExtractTerms{terms, true}(rhs.second)) {
                    slv_->add(IE{std::move(terms), bound});
                }
                break;
            }
            case Relation::equal: {
                if (IETermVec terms; ExtractTerms{terms, true}(lit.lhs()) && ExtractTerms{terms, false}(rhs.second)) {
                    slv_->add(IE{std::move(terms), bound});
                }
                if (IETermVec terms; ExtractTerms{terms, false}(lit.lhs()) && ExtractTerms{terms, true}(rhs.second)) {
                    slv_->add(IE{std::move(terms), bound});
                }
                break;
            }
            case Relation::not_equal: {
                break;
            }
        }
    }

    void operator()(BdLit const &lit) const {
        if (auto const *slit = std::get_if<BdLitSimple>(&lit); slit != nullptr) {
            operator()(slit->lit());
        }
    }

  private:
    IESolver *slv_;
};

//! Apply bounds modifying relation literals.
//!
//! The following comparisons are handled:
//! Number n in `X rel n` is adjusted according to relation and bounds.
//! Numbers n and m in `X = m..n` are adjusted according to bounds.
//! Unnecessary relations are dropped.
class ApplyBounds {
  public:
    ApplyBounds(IEDomain const &dom, BoundStateMap &states, SymbolStore &store)
        : dom_{&dom}, states_{&states}, store_{&store} {}

    auto operator()(Lit const &lit) const -> Util::ResultState<Lit> { return std::visit(*this, lit); }

    auto operator()([[maybe_unused]] auto const &lit) const -> Util::ResultState<Lit> { return {true}; }

    auto operator()(LitComparison const &lit) const -> Util::ResultState<Lit> {
        assert(lit.sign() == Sign::none);
        auto const &rhs = lit.rhs().front();
        auto make_symbol = [this](TermSymbol const &sym, auto const &bound) {
            if (sym.value().num() == bound) {
                return sym;
            }
            return TermSymbol{sym.loc(), store_->num_ref(bound)};
        };
        auto make_relation = [this, &lit](auto lhs, Relation rel, Location loc, auto bound) {
            return lit.update(a_lhs = std::move(lhs),
                              a_rhs = Util::make_vec<Guard>(
                                  Guard{rel, TermSymbol{std::move(loc), store_->num_ref(std::move(bound))}}));
        };
        auto make_interval = [&lit](auto var, auto loc, auto u, auto v) -> Util::ResultState<Lit> {
            if (u.value() == v.value()) {
                return {true,
                        lit.update(a_lhs = std::move(var), a_rhs = Util::make_vec<Guard>(Guard{Relation::equal, u}))};
            }
            return {true, lit.update(a_lhs = std::move(var),
                                     a_rhs = Util::make_vec<Guard>(
                                         Guard{Relation::equal, TermBinary{std::move(loc), std::move(u),
                                                                           BinaryOperator::dots, std::move(v)}}))};
        };
        if (is_variable(lit.lhs()) && is_interval(rhs.second)) {
            auto const *var = std::get_if<TermVariable>(&lit.lhs());
            auto it = dom_->find(var->name());
            if (it == dom_->end()) {
                return {true};
            }
            auto const *u = std::get_if<TermSymbol>(&std::get<TermBinary>(rhs.second).lhs().get());
            auto const *t = std::get_if<TermSymbol>(&std::get<TermBinary>(rhs.second).rhs().get());
            // Note: in theory the lower/upper bounds could be refined even if only one of them is a number
            if (u == nullptr || t == nullptr || u->value().type() != SymbolType::number ||
                t->value().type() != SymbolType::number) {
                return {true};
            }
            auto &state = states_->operator[](std::distance(dom_->begin(), it));
            if (state.both == 1) {
                return {false};
            }
            state.both = 1;
            auto res_u = std::optional<TermSymbol>{};
            if (u->value().num() < it->second.value(IEInterval::Lower)) {
                res_u = make_symbol(*u, it->second.value(IEInterval::Lower));
            }
            auto res_t = std::optional<TermSymbol>{};
            if (t->value().num() > it->second.value(IEInterval::Upper)) {
                res_t = make_symbol(*t, it->second.value(IEInterval::Upper));
            }
            if (res_u || res_t) {
                return make_interval(lit.lhs(), location(rhs.second), res_u.value_or(*u), res_t.value_or(*t));
            }
            return {true};
        }
        // Result=true  => keep
        // Result=false => drop
        // has value    => replace
        auto update_bound = [this, &make_symbol, &make_relation,
                             &make_interval](auto const &lhs, Relation rel, auto const &rhs) -> Util::ResultState<Lit> {
            auto const *var = std::get_if<TermVariable>(&lhs);
            auto const *sym = std::get_if<TermSymbol>(&rhs);
            // Note: non-integer bounds could also be handled
            if (var == nullptr || sym == nullptr || sym->value().type() != SymbolType::number) {
                return {true};
            }
            auto it = dom_->find(var->name());
            if (it == dom_->end()) {
                return {true};
            }
            auto &state = states_->operator[](std::distance(dom_->begin(), it));
            auto bound_type = IEInterval::Lower;
            switch (rel) {
                case Relation::greater:
                case Relation::greater_equal: {
                    bound_type = IEInterval::Lower;
                    break;
                }
                case Relation::less:
                case Relation::less_equal: {
                    bound_type = IEInterval::Upper;
                    break;
                }
                case Relation::equal: {
                    if (state.both == 1) {
                        return {false};
                    }
                    state.both = 1;
                    return {true};
                }
                case Relation::not_equal: {
                    return {true};
                }
            }
            // drop if covered
            if (state.value(bound_type)) {
                return {false};
            }
            // ignore if not bounded
            if (!it->second.has_value(bound_type)) {
                return {true};
            }
            auto bound = it->second.value(bound_type);
            if (it->second.has_value(flip(bound_type))) {
                state.both = 1;
                return make_interval(*var, location(rhs), make_symbol(*sym, it->second.value(IEInterval::Lower)),
                                     make_symbol(*sym, it->second.value(IEInterval::Upper)));
            }
            // mark as covered
            state.set_value(bound_type);
            // update if changed
            if (cmp(bound, rel, sym->value().num() + Number{bound_type == IEInterval::Lower ? 1 : -1})) {
                auto rel = bound_type == IEInterval::Lower ? Relation::greater_equal : Relation::less_equal;
                return {true, make_relation(lhs, rel, location(rhs), std::move(bound))};
            }
            return {true};
        };
        if (auto res = update_bound(lit.lhs(), rhs.first, rhs.second); !res.state || res.value) {
            return res;
        }
        return update_bound(rhs.second, flip(rhs.first), lit.lhs());
    }

  private:
    IEDomain const *dom_;
    BoundStateMap *states_;
    SymbolStore *store_;
};

class ComputeBounds {
  public:
    ComputeBounds(RewriteContext &ctx) : ctx_{&ctx} {}

    //! Compute bounds given a set of literals/body literals.
    template <class Span>
    auto compute_bounds(IESolver &slv, Location const &loc, Span const &lits)
        -> std::pair<bool, decltype(Util::ResultVec{lits})> {

        auto res_lits = Util::ResultVec{lits};

        // add inequalities to solver
        for (auto const &lit : lits) {
            ExtractInequalities{slv}(lit);
        }

        // compute bounds
        if (!slv.compute(ctx_->logger())) {
            return {false, std::move(res_lits)};
        }
        auto const &dom = slv.domain();
        if (dom.empty()) {
            return {true, std::move(res_lits)};
        }

        // adjust relation literals in condition
        BoundStateMap states;
        states.resize(dom.size());
        states.reserve(dom.size());
        for (auto const &lit : lits) {
            Lit const *slit = nullptr;
            if constexpr (std::is_same_v<typename Span::value_type, BdLit>) {
                if (auto sblit = std::get_if<BdLitSimple>(&lit); sblit != nullptr) {
                    slit = &sblit->lit();
                }
            } else {
                slit = &lit;
            }
            if (slit != nullptr) {
                auto res = ApplyBounds{dom, states, ctx_->store()}(*slit);
                if (!res.state) {
                    res_lits.remove();
                } else {
                    res_lits.update(std::move(res.value));
                }
            } else {
                res_lits.keep();
            }
        }

        // add relation literals to literals if required
        auto make_relation = [this, &loc](auto const &var, Relation rel, auto const &bound) -> Lit {
            auto term_var = TermVariable{loc, var};
            return LitComparison{loc, Sign::none, term_var,
                                 Util::make_vec<Guard>(Guard{rel, TermSymbol{loc, ctx_->store().num_ref(bound)}})};
        };
        auto make_interval = [this, &loc](auto var, Number const &u, Number const &v) -> Lit {
            auto term_var = TermVariable{loc, var};
            auto term_u = TermSymbol{loc, ctx_->store().num_ref(u)};
            if (u == v) {
                return LitComparison{loc, Sign::none, term_var, Util::make_vec<Guard>(Guard{Relation::equal, term_u})};
            }
            auto term_v = TermSymbol{loc, ctx_->store().num_ref(v)};
            return LitComparison{
                loc, Sign::none, term_var,
                Util::make_vec<Guard>(Guard{Relation::equal, TermBinary{loc, term_u, BinaryOperator::dots, term_v}})};
        };
        auto it = dom.begin();
        for (auto &state : states) {
            if (!slv.strengthens(it->first)) {
                continue;
            }
            if (it->second.has_value(IEInterval::Lower) && it->second.has_value(IEInterval::Upper)) {
                if (state.both == 0) {
                    res_lits.append(make_interval(it->first, it->second.value(IEInterval::Lower),
                                                  it->second.value(IEInterval::Upper)));
                }
            } else if (it->second.has_value(IEInterval::Lower)) {
                if (state.lower == 0) {
                    res_lits.append(
                        make_relation(it->first, Relation::greater_equal, it->second.value(IEInterval::Lower)));
                }
            } else if (it->second.has_value(IEInterval::Upper)) {
                if (state.upper == 0) {
                    res_lits.append(
                        make_relation(it->first, Relation::less_equal, it->second.value(IEInterval::Upper)));
                }
            }
            ++it;
        }
        return {true, res_lits};
    }

    //! Helper to compute bounds for a set of elements.
    template <class E, class R> auto compute_bounds_elem(E const &elem, R &elems) {
        auto sub_slv = IESolver{&slv_};
        auto [state_lits, res_lits] = compute_bounds(sub_slv, elem.loc(), elem.cond());
        if (!state_lits) {
            elems.remove();
        } else {
            elems.update(elem.rewrite(a_cond = std::move(res_lits)));
        }
    }

    // set aggregates and theory atoms

    template <bool Body> using HBRes = std::conditional_t<Body, Util::ResultState<BdLit>, std::optional<HdLit>>;

    template <bool Body> auto operator()([[maybe_unused]] SetAggregate<Body> const &lit) -> HBRes<Body> {
        throw std::runtime_error("unpool must be called before computing bounds");
    }

    template <bool Body> auto operator()(TheoryAtom<Body> const &lit) -> HBRes<Body> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            compute_bounds_elem(elem, res_elems);
        }
        if constexpr (Body) {
            return {true, lit.rewrite(a_elems = std::move(res_elems))};
        } else {
            return lit.rewrite(a_elems = std::move(res_elems));
        }
    }

    // head literals

    auto operator()(HdLit const &lit) -> std::optional<HdLit> { return std::visit(*this, lit); }

    auto operator()([[maybe_unused]] HdLitSimple const &lit) -> std::optional<HdLit> { return std::nullopt; }

    auto operator()(HdLitDisjunction const &lit) -> std::optional<HdLit> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            if (auto const *clit = std::get_if<CondLit>(&elem)) {
                compute_bounds_elem(*clit, res_elems);
            }
        }
        return lit.rewrite(a_elems = std::move(res_elems));
    }

    auto operator()(HdLitAggregate const &lit) -> std::optional<HdLit> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            compute_bounds_elem(elem, res_elems);
        }
        return lit.rewrite(a_elems = std::move(res_elems));
    }

    // body literals

    auto operator()(BdLit const &lit) -> Util::ResultState<BdLit> { return std::visit(*this, lit); }

    auto operator()([[maybe_unused]] BdLitSimple const &lit) -> Util::ResultState<BdLit> { return {true}; }

    auto operator()(BdLitConjunction const &conj) -> Util::ResultState<BdLit> {
        auto sub_slv = IESolver{&slv_};
        auto [state_cond, res_cond] = compute_bounds(sub_slv, conj.lit().loc(), conj.lit().cond());
        if (!state_cond) {
            return {false, BdLitSimple{LitBool{conj.lit().loc(), Sign::none, false}}};
        }
        return {true, conj.lit().rewrite(a_cond = std::move(res_cond))};
    }

    auto operator()(BdLitAggregate const &lit) -> Util::ResultState<BdLit> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            compute_bounds_elem(elem, res_elems);
        }
        return {true, lit.rewrite(a_elems = std::move(res_elems))};
    }

    // statements

    template <class T, class F> auto compute_bounds_body(T const &stm, F &&fun) -> Util::ResultState<Stm> {
        // compute bounds
        auto [state_body, res_body] = compute_bounds(slv_, stm.loc(), stm.body());
        if (!state_body) {
            return {false, StmRule{stm.loc(), HdLitSimple{LitBool{stm.loc(), Sign::none, true}}, {}}};
        }

        // refine bounds in nested contexts
        auto res_body_nested = Util::ResultVec{res_body.value()};
        for (auto const &lit : res_body.value()) {
            // Note: only the case that a literal became false is handled here.
            if (auto res_lit = operator()(lit); res_lit.state) {
                res_body_nested.update(std::move(res_lit).value);
            } else {
                return {false, StmRule{stm.loc(), HdLitSimple{LitBool{stm.loc(), Sign::none, true}}, {}}};
            }
        }
        if (res_body_nested) {
            res_body.as_optional() = std::move(res_body_nested).as_optional();
        }

        return {true, std::invoke(std::forward<F>(fun), std::move(res_body))};
    }

    auto operator()(Stm const &stm) -> Util::ResultState<Stm> { return std::visit(*this, stm); }

    auto operator()(StmRule const &stm) -> Util::ResultState<Stm> {
        return compute_bounds_body(stm, [&](auto res_body) -> std::optional<Stm> {
            return stm.rewrite(a_head = operator()(stm.head()), a_body = std::move(res_body));
        });
    }

    auto operator()([[maybe_unused]] StmOptimize const &stm) -> Util::ResultState<Stm> {
        throw std::runtime_error("unpool must be called before computing bounds");
    }

    template <class T> auto operator()([[maybe_unused]] T const &stm) -> Util::ResultState<Stm> {
        if constexpr (Util::is_among_v<T, StmWeakConstraint, StmShow, StmProject, StmExternal, StmEdge, StmHeuristic>) {
            return compute_bounds_body(
                stm, [&](auto res_body) -> std::optional<Stm> { return stm.rewrite(a_body = std::move(res_body)); });
        } else {
            static_assert(Util::is_among_v<T, StmTheory, StmProjectSig, StmDefined, StmShowNothing, StmShowSig,
                                           StmScript, StmInclude, StmProgram, StmConst, StmParts, StmComment>);
            return {true};
        }
    }

  private:
    RewriteContext *ctx_;
    IESolver slv_;
};

} // namespace

[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Stm const &stm) -> Util::ResultState<Stm> {
    return ComputeBounds{ctx}(stm);
}

} // namespace CppClingo::Input
