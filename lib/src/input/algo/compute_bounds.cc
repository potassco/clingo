#include <input/algo/analyze.hh>
#include <input/algo/compute_bounds.hh>

#include <input/iesolver.hh>

// TODO: remove
#include <input/algo/print.hh>
#include <iostream>

namespace Gringo::Input {

using Util::TruthValue;

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
        case Relation::inequal: {
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
struct ExtractTerms {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(auto const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermVariable const &term) const -> bool {
        auto x = IETerm{Number{1}, term.name};
        if (!add) {
            x.coefficient *= -1;
        }
        add_term(terms, std::move(x));
        return true;
    }

    auto operator()(TermSymbol const &term) const -> bool {
        if (term.value.type() == SymbolType::number) {
            auto x = IETerm{term.value.num(), String{}};
            if (!add) {
                x.coefficient *= -1;
            }
            add_term(terms, std::move(x));
            return true;
        }
        return false;
    }

    auto operator()(TermUnary const &term) const -> bool {
        if (term.op == UnaryOperator::negate) {
            return ExtractTerms{terms, !add}(*term.rhs);
        }
        return false;
    }

    auto operator()(TermBinary const &term) const -> bool {
        switch (term.op) {
            case BinaryOperator::minus: {
                return operator()(*term.lhs) && ExtractTerms{terms, !add}(*term.rhs);
            }
            case BinaryOperator::plus: {
                return operator()(*term.lhs) && operator()(*term.rhs);
            }
            case BinaryOperator::times: {
                IETermVec lhs;
                IETermVec rhs;
                auto fixed_lhs = Number{0};
                auto fixed_rhs = Number{0};
                auto ret_lhs = ExtractTerms{lhs, true}(*term.lhs);
                auto ret_rhs = ExtractTerms{rhs, true}(*term.rhs);
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
                if (!add) {
                    fixed *= -fixed;
                }
                add_term(terms, IETerm{std::move(fixed), String{}});
                if (!lhs.empty()) {
                    lhs.swap(rhs);
                    fixed_lhs.swap(fixed_rhs);
                }
                if (lhs.empty()) {
                    for (auto &x : rhs) {
                        x.coefficient *= fixed_lhs;
                        if (!add) {
                            x.coefficient *= -1;
                        }
                        add_term(terms, std::move(x));
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

    IETermVec &terms;
    bool add = true;
};

//! Extract inequalities from relation literals.
struct ExtractInequalities {
    void operator()(Literal const &lit) const { std::visit(*this, lit); }

    void operator()(auto const &lit) const { static_cast<void>(lit); }

    void operator()(LiteralRelation const &lit) const {
        assert(lit.sign == Sign::none);
        auto const &rhs = lit.rhs.front();
        int bound = 0;
        // handle intervals
        //   X = u..t -> X - u >= 0
        //               t - X >= 0
        if (is_variable(lit.lhs) && is_interval(rhs.second)) {
            auto const &u = *std::get<TermBinary>(rhs.second).lhs;
            auto const &t = *std::get<TermBinary>(rhs.second).rhs;
            if (IETermVec terms; ExtractTerms{terms, true}(lit.lhs) && ExtractTerms{terms, false}(u)) {
                slv.add(IE{std::move(terms), bound});
            }
            if (IETermVec terms; ExtractTerms{terms, false}(lit.lhs) && ExtractTerms{terms, true}(t)) {
                slv.add(IE{std::move(terms), bound});
            }
            return;
        }
        // handle linear terms
        //   X >= Y -> X - Y >=  0
        //   X >  Y -> X - Y >= -1
        //   X <= Y -> Y - X >=  0
        //   X <  Y -> Y - X >=  1
        //   X =  Y -> X - Y >=  0
        //             Y - X >=  0
        //   X != Y -> cannot handle
        switch (rhs.first) {
            case Relation::greater: {
                bound = -1;
                [[fallthrough]];
            }
            case Relation::greater_equal: {
                if (IETermVec terms; ExtractTerms{terms, true}(lit.lhs) && ExtractTerms{terms, false}(rhs.second)) {
                    slv.add(IE{std::move(terms), bound});
                }
                break;
            }
            case Relation::less: {
                bound = 1;
                [[fallthrough]];
            }
            case Relation::less_equal: {
                if (IETermVec terms; ExtractTerms{terms, false}(lit.lhs) && ExtractTerms{terms, true}(rhs.second)) {
                    slv.add(IE{std::move(terms), bound});
                }
                break;
            }
            case Relation::equal: {
                if (IETermVec terms; ExtractTerms{terms, true}(lit.lhs) && ExtractTerms{terms, false}(rhs.second)) {
                    slv.add(IE{std::move(terms), bound});
                }
                if (IETermVec terms; ExtractTerms{terms, false}(lit.lhs) && ExtractTerms{terms, true}(rhs.second)) {
                    slv.add(IE{std::move(terms), bound});
                }
                break;
            }
            case Relation::inequal: {
                break;
            }
        }
    }

    void operator()(BodyLiteral const &lit) const {
        if (auto const *slit = std::get_if<SimpleBodyLiteral>(&lit); slit != nullptr) {
            operator()(slit->lit);
        }
    }

    IESolver &slv;
};

//! Apply bounds modifying relation literals.
//!
//! The following comparisons are handled:
//! Number n in `X rel n` is adjusted according to relation and bounds.
//! Numbers n and m in `X = m..n` are adjusted according to bounds.
//! Unnecessary relations are dropped.
struct ApplyBounds {
    auto operator()(Literal const &lit) const -> Util::ResultState<Literal> { return std::visit(*this, lit); }

    auto operator()(auto const &lit) const -> Util::ResultState<Literal> {
        static_cast<void>(lit);
        return {true};
    }

    auto operator()(LiteralRelation const &lit) const -> Util::ResultState<Literal> {
        assert(lit.sign == Sign::none);
        auto const &rhs = lit.rhs.front();
        auto make_symbol = [this](TermSymbol const &sym, auto &&bound) {
            if (sym.value.num() == bound) {
                return sym;
            }
            return TermSymbol{sym.loc, store.num(GRINGO_FWD(bound))};
        };
        auto make_relation = [this, &lit](auto const &lhs, Relation rel, Location loc, auto const &bound) {
            return LiteralRelation{lit.loc, Sign::none, lhs,
                                   Util::make_vec<Guard>(Guard{rel, TermSymbol{std::move(loc), store.num(bound)}})};
        };
        auto make_interval = [&lit](auto var, auto loc, auto u, auto v) -> Util::ResultState<Literal> {
            if (u.value == v.value) {
                return {true,
                        LiteralRelation{lit.loc, Sign::none, var, Util::make_vec<Guard>(Guard{Relation::equal, u})}};
            }
            return {true, LiteralRelation{lit.loc, Sign::none, std::move(var),
                                          Util::make_vec<Guard>(
                                              Guard{Relation::equal, TermBinary{std::move(loc), std::move(u),
                                                                                BinaryOperator::dots, std::move(v)}})}};
        };
        if (is_variable(lit.lhs) && is_interval(rhs.second)) {
            auto const *var = std::get_if<TermVariable>(&lit.lhs);
            auto it = dom.find(var->name);
            if (it == dom.end()) {
                return {true};
            }
            auto const *u = std::get_if<TermSymbol>(std::get<TermBinary>(rhs.second).lhs.get());
            auto const *t = std::get_if<TermSymbol>(std::get<TermBinary>(rhs.second).rhs.get());
            // Note: in theory the lower/upper bounds could be refined even if only one of them is a number
            if (u == nullptr || t == nullptr || u->value.type() != SymbolType::number ||
                t->value.type() != SymbolType::number) {
                return {true};
            }
            auto &state = states[std::distance(dom.begin(), it)];
            if (state.both == 1) {
                return {false};
            }
            state.both = 1;
            auto res_u = std::optional<TermSymbol>{};
            if (*u->value.num() < it->second.value(IEInterval::Lower)) {
                res_u = make_symbol(*u, it->second.value(IEInterval::Lower));
            }
            auto res_t = std::optional<TermSymbol>{};
            if (*t->value.num() > it->second.value(IEInterval::Upper)) {
                res_t = make_symbol(*t, it->second.value(IEInterval::Upper));
            }
            if (res_u || res_t) {
                return make_interval(lit.lhs, location(rhs.second), std::move(res_u).value_or(*u),
                                     std::move(res_t).value_or(*t));
            }
            return {true};
        }
        // Result=true  => keep
        // Result=false => drop
        // has value    => replace
        auto update_bound = [this, &make_symbol, &make_relation,
                             &make_interval](auto &lhs, Relation rel, auto &rhs) -> Util::ResultState<Literal> {
            auto const *var = std::get_if<TermVariable>(&lhs);
            auto const *sym = std::get_if<TermSymbol>(&rhs);
            // Note: non-integer bounds could also be handled
            if (var == nullptr || sym == nullptr || sym->value.type() != SymbolType::number) {
                return {true};
            }
            auto it = dom.find(var->name);
            if (it == dom.end()) {
                return {true};
            }
            auto &state = states[std::distance(dom.begin(), it)];
            auto const &num = *sym->value.num();
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
                case Relation::inequal: {
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
            auto adjust = bound_type == IEInterval::Lower ? 1 : -1;
            if (cmp(bound, rel, num + adjust)) {
                auto rel = bound_type == IEInterval::Lower > 0 ? Relation::greater_equal : Relation::less_equal;
                return {true, make_relation(lhs, rel, location(rhs), bound)};
            }
            return {true};
        };
        if (auto res = update_bound(lit.lhs, rhs.first, rhs.second); !res.state || res.value) {
            return res;
        }
        return update_bound(rhs.second, flip(rhs.first), lit.lhs);
    }

    IEDomain const &dom;
    BoundStateMap &states;
    SymbolStore &store;
};

struct ComputeBounds {
    template <class T>
    auto compute_bounds(IESolver &slv, Location const &loc, std::vector<T> const &lits)
        -> std::pair<bool, Util::ResultVec<T>> {
        auto res_lits = Util::ResultVec{lits};

        // add inequalities to solver
        for (auto const &lit : lits) {
            ExtractInequalities{slv}(lit);
        }

        // compute bounds
        if (!slv.compute(ctx.logger())) {
            return {false, std::move(res_lits)};
        }
        auto const &dom = slv.domain();
        if (dom.empty()) {
            return {true, std::move(res_lits)};
        }
        std::cerr << "Refine bounds of conjunction:" << std::endl;
        for (auto const &bound : slv.domain()) {
            std::cerr << "  " << bound.first << ": " << bound.second << std::endl;
        }

        // adjust relation literals in condition
        BoundStateMap states;
        states.resize(dom.size());
        states.reserve(dom.size());
        for (auto const &lit : lits) {
            Literal const *slit = nullptr;
            if constexpr (std::is_same_v<T, BodyLiteral>) {
                if (auto sblit = std::get_if<SimpleBodyLiteral>(&lit); sblit != nullptr) {
                    slit = &sblit->lit;
                }
            } else {
                slit = &lit;
            }
            if (slit != nullptr) {
                auto res = ApplyBounds{dom, states, ctx.store()}(*slit);
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
        auto make_relation = [this, &loc](auto const &var, Relation rel, auto const &bound) -> Literal {
            auto term_var = TermVariable{loc, var};
            return LiteralRelation{loc, Sign::none, std::move(term_var),
                                   Util::make_vec<Guard>(Guard{rel, TermSymbol{loc, ctx.store().num(bound)}})};
        };
        auto make_interval = [this, &loc](auto var, Number const &u, Number const &v) -> Literal {
            auto term_var = TermVariable{loc, var};
            auto term_u = TermSymbol{loc, ctx.store().num(u)};
            if (u == v) {
                return LiteralRelation{loc, Sign::none, std::move(term_var),
                                       Util::make_vec<Guard>(Guard{Relation::equal, std::move(term_u)})};
            }
            auto term_v = TermSymbol{loc, ctx.store().num(v)};
            return LiteralRelation{
                loc, Sign::none, std::move(term_var),
                Util::make_vec<Guard>(Guard{
                    Relation::equal, TermBinary{loc, std::move(term_u), BinaryOperator::dots, std::move(term_v)}})};
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

    // head literals

    auto operator()(HeadLiteral const &lit) -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(Disjunction const &lit) -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(HeadAggregate const &lit) -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(HeadSetAggregate const &lit) -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("unpool must be called before computing bounds");
    }

    auto operator()(HeadTheoryAtom const &lit) -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me!!!");
    }

    // body literals

    auto operator()(BodyLiteral const &lit) -> Util::ResultState<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) -> Util::ResultState<BodyLiteral> {
        static_cast<void>(lit);
        return {true};
    }

    auto operator()(Conjunction const &conj) -> Util::ResultState<BodyLiteral> {
        auto sub_slv = IESolver{&slv};
        auto [state_cond, res_cond] = compute_bounds(sub_slv, conj.lit.loc, conj.lit.cond);
        if (!state_cond) {
            return {false, SimpleBodyLiteral{LiteralBoolean{conj.lit.loc, Sign::none, false}}};
        }
        if (res_cond) {
            return {true, Conjunction{ConditionalLiteral{conj.lit.loc, conj.lit.lit, std::move(res_cond).value()}}};
        }
        return {true};
    }

    auto operator()(BodyAggregate const &lit) -> Util::ResultState<BodyLiteral> {
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto const &elem : lit.elems) {
            auto sub_slv = IESolver{&slv};
            auto [state_lits, res_lits] = compute_bounds(sub_slv, elem.loc, elem.cond);
            if (state_lits) {
                res_elems.remove();
            } else if (res_elems) {
                res_elems.replace(elem.loc, elem.tuple, std::move(res_lits).value());
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true, BodyAggregate{lit.loc, lit.sign, lit.lhs, lit.fun, std::move(res_elems).value(), lit.rhs}};
        }
        return {true};
    }

    auto operator()(BodySetAggregate const &lit) -> Util::ResultState<BodyLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("unpool must be called before computing bounds");
    }

    auto operator()(BodyTheoryAtom const &lit) -> Util::ResultState<BodyLiteral> {
        // TODO: can be made generic for head/body
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto const &elem : lit.elems) {
            auto sub_slv = IESolver{&slv};
            auto [state_lits, res_lits] = compute_bounds(sub_slv, lit.loc, elem.second);
            if (state_lits) {
                res_elems.remove();
            } else if (res_elems) {
                res_elems.replace(elem.first, std::move(res_lits).value());
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true, BodyTheoryAtom{lit.loc, lit.sign, lit.name, std::move(res_elems).value(), lit.rhs}};
        }
        return {true};
    }

    // statements

    auto operator()(Statement const &stm) -> Util::ResultState<Statement> { return std::visit(*this, stm); }

    auto operator()(auto const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me: computer bounds other statements");
    }
    auto operator()(Rule const &stm) -> Util::ResultState<Statement> {
        // compute bounds
        auto [state_body, res_body] = compute_bounds(slv, stm.loc, stm.body);
        if (!state_body) {
            // TODO: maybe add rep
            return {false};
        }

        // refine bounds in nested contexts
        auto res_head = operator()(stm.head);
        auto res_body_nested = Util::ResultVec{res_body.value()};
        for (auto const &lit : res_body.value()) {
            // Note: only the case that a literal became false is handled here.
            if (auto res_lit = operator()(lit); res_lit.state) {
                res_body_nested.update(std::move(res_lit).value);
            } else {
                // TODO: maybe add rep
                return {false};
            }
        }
        if (res_body_nested) {
            res_body.as_optional() = std::move(res_body_nested).as_optional();
        }

        // return updated result
        if (res_head || res_body) {
            std::cerr << Statement{Rule{stm.loc, stm.head, res_body.value()}} << std::endl;
            return {true, Rule{stm.loc, std::move(res_head).value_or(stm.head), std::move(res_body).value()}};
        }
        return {true};
    }

    RewriteContext &ctx;
    IESolver slv = {};
};

} // namespace

[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Statement const &stm) -> Util::ResultState<Statement> {
    return ComputeBounds{ctx}(stm);
}

} // namespace Gringo::Input
