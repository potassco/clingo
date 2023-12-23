#include <input/algo/analyze.hh>
#include <input/algo/compute_bounds.hh>

#include <input/iesolver.hh>

// TODO: remove
#include <input/algo/print.hh>
#include <iostream>

namespace Gringo::Input {

using Util::TruthValue;

namespace {

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

struct ExtractBounds {
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

    IESolver &slv;
};

struct BoundState {
    uint8_t lower : 1 = 0;
    uint8_t upper : 1 = 0;
    uint8_t both : 1 = 0;
};
using BoundStateMap = std::vector<BoundState>;

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
            return LiteralRelation{lit.loc, lit.sign, lhs,
                                   Util::make_vec<Guard>(Guard{rel, TermSymbol{std::move(loc), store.num(bound)}})};
        };
        auto make_interval = [&lit](auto var, auto loc, auto u, auto v) -> Util::ResultState<Literal> {
            if (u.value == v.value) {
                return {true,
                        LiteralRelation{lit.loc, lit.sign, var, Util::make_vec<Guard>(Guard{Relation::equal, u})}};
            }
            return {true, LiteralRelation{lit.loc, lit.sign, std::move(var),
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
            switch (rel) {
                case Relation::greater:
                case Relation::greater_equal: {
                    // drop if covered
                    if (state.lower == 1 || state.both == 1) {
                        return {false};
                    }
                    // var >= num
                    if (!it->second.has_value(IEInterval::Lower)) {
                        return {true};
                    }
                    auto bound = it->second.value(IEInterval::Lower);
                    if (it->second.has_value(IEInterval::Upper)) {
                        state.both = 1;
                        return make_interval(*var, location(rhs),
                                             make_symbol(*sym, it->second.value(IEInterval::Lower)),
                                             make_symbol(*sym, it->second.value(IEInterval::Upper)));
                    }
                    // mark as covered
                    state.lower = 1;
                    // update if changed
                    if (rel == Relation::greater_equal ? bound > num : bound >= num) {
                        return {true, make_relation(lhs, Relation::greater_equal, location(rhs), bound)};
                    }
                    break;
                }
                case Relation::less:
                case Relation::less_equal: {
                    // drop if covered
                    if (state.upper == 1 || state.both == 1) {
                        return {false};
                    }
                    // var >= num
                    if (!it->second.has_value(IEInterval::Upper)) {
                        return {true};
                    }
                    // mark as covered
                    auto bound = it->second.value(IEInterval::Upper);
                    if (it->second.has_value(IEInterval::Lower)) {
                        state.both = 1;
                        return make_interval(*var, location(rhs),
                                             make_symbol(*sym, it->second.value(IEInterval::Lower)),
                                             make_symbol(*sym, it->second.value(IEInterval::Upper)));
                    }
                    state.upper = 1;
                    // update if changed
                    if (rel == Relation::less_equal ? bound < num : bound <= num) {
                        return {true, make_relation(lhs, Relation::less_equal, location(rhs), bound)};
                    }
                    break;
                }
                case Relation::equal: {
                    if (state.both == 1) {
                        return {false};
                    }
                    state.both = 1;
                    break;
                }
                case Relation::inequal: {
                    break;
                }
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

    auto operator()(Statement const &stm) const -> Util::ResultState<Statement> { return std::visit(*this, stm); }

    auto operator()(auto const &stm) const -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me: computer bounds other statements");
    }
    auto operator()(Rule const &stm) const -> Util::ResultState<Statement> {
        IESolver slv;
        for (auto const &lit : stm.body) {
            if (auto const *slit = std::get_if<SimpleBodyLiteral>(&lit); slit != nullptr) {
                ExtractBounds{slv}(slit->lit);
            }
        }
        if (!slv.compute(ctx.logger())) {
            return {false};
        }
        auto const &dom = slv.domain();
        if (dom.empty()) {
            return {true};
        }
        std::cerr << "Refine bounds:" << std::endl;
        for (auto const &bound : slv.domain()) {
            std::cerr << "  " << bound.first << ": " << bound.second << std::endl;
        }
        BoundStateMap states;
        states.resize(dom.size());
        states.reserve(dom.size());
        auto res_body = Util::ResultVec{stm.body};
        for (auto const &lit : stm.body) {
            if (auto const *slit = std::get_if<SimpleBodyLiteral>(&lit); slit != nullptr) {
                auto res = ApplyBounds{dom, states, ctx.store()}(slit->lit);
                if (!res.state) {
                    res_body.remove();
                } else {
                    res_body.update(std::move(res.value));
                }
            } else {
                res_body.keep();
            }
        }
        if (res_body) {
            std::cerr << Statement{Rule{stm.loc, stm.head, res_body.value()}} << std::endl;
            return {true, Rule{stm.loc, stm.head, std::move(res_body).value()}};
        }
        return {true};
    }

    RewriteContext &ctx;
};

} // namespace

[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Statement const &stm) -> Util::ResultState<Statement> {
    return ComputeBounds{ctx}(stm);
}

} // namespace Gringo::Input
