#include <input/algo/analyze.hh>
#include <input/algo/compute_bounds.hh>

#include <input/iesolver.hh>

// TODO: remove
#include <iostream>

namespace Gringo::Input {

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
            }
            case Relation::greater_equal: {
                if (IETermVec terms; ExtractTerms{terms, true}(lit.lhs) && ExtractTerms{terms, false}(rhs.second)) {
                    slv.add(IE{std::move(terms), bound});
                }
                break;
            }
            case Relation::less: {
                bound = 1;
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

struct ApplyBounds {
    void operator()(Literal const &lit) const { std::visit(*this, lit); }

    void operator()(auto const &lit) const { static_cast<void>(lit); }

    void operator()(LiteralRelation const &lit) const {
        assert(lit.sign == Sign::none);
        auto const &rhs = lit.rhs.front();
        if (is_variable(lit.lhs) && is_interval(rhs.second)) {
            auto const &u = *std::get<TermBinary>(rhs.second).lhs;
            auto const &t = *std::get<TermBinary>(rhs.second).rhs;
            static_cast<void>(t);
            static_cast<void>(u);
            throw std::logic_error("implement me!!!");
            // if is const u and u < dom[var].lower:
            //   u = dom[var].lower
            // if is const t and t >= dom[var].upper:
            //   t = dom[var].upper
            // mark interval as covered if both const
            // if t == u:
            //   replace rhs by t
            //   (maybe add 0 if not const)
            return;
        }
        auto update_bound = [this](auto &lhs, Relation rel, auto &rhs) {
            auto const *var = std::get_if<TermVariable>(&lhs);
            auto const *sym = std::get_if<TermSymbol>(&rhs);
            if (var == nullptr || sym == nullptr || sym->value.type() != SymbolType::number) {
                return false;
            }
            auto it = dom.find(var->name);
            if (it == dom.end()) {
                return false;
            }
            auto const &num = *sym->value.num();
            switch (rel) {
                case Relation::greater: {
                    // var >= num
                    if (!it->second.has_value(IEInterval::Lower)) {
                        return false;
                    }
                    // drop if already covered!!!
                    // TODO
                    // mark as covered
                    // TODO
                    // update if changed
                    if (it->second.value(IEInterval::Lower) < num) {
                        // the interval changed
                        return true;
                    }
                    break;
                }
                case Relation::greater_equal: {
                    break;
                }
                case Relation::less: {
                    break;
                }
                case Relation::less_equal: {
                    break;
                }
                case Relation::equal: {
                    break;
                }
                case Relation::inequal: {
                    break;
                }
            }
            return false;
        };
        // handle linear terms
        //   X >= Y -> X - Y >=  0
        //   X >  Y -> X - Y >= -1
        //   X <= Y -> Y - X >=  0
        //   X <  Y -> Y - X >=  1
        //   X =  Y -> X - Y >=  0
        //             Y - X >=  0
        //   X != Y -> cannot handle
        update_bound(lit.lhs, rhs.first, rhs.second);
        update_bound(rhs.second, flip(rhs.first), lit.lhs);
    }

    IEDomain const &dom;
};

struct ComputeBounds {

    auto operator()(Statement const &stm) const -> Util::ResultState<Statement> { return std::visit(*this, stm); }

    auto operator()(auto const &stm) const -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
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
        for (auto const &lit : stm.body) {
            if (auto const *slit = std::get_if<SimpleBodyLiteral>(&lit); slit != nullptr) {
                ApplyBounds{dom}(slit->lit);
            }
        }
        // TODO: add pool/and refine bounds
        throw std::logic_error("implement me!!!");
    }

    RewriteContext &ctx;
};

} // namespace

[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Statement const &stm) -> Util::ResultState<Statement> {
    return ComputeBounds{ctx}(stm);
}

} // namespace Gringo::Input
