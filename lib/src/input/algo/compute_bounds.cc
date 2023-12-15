#include <input/algo/analyze.hh>
#include <input/algo/compute_bounds.hh>

#include <input/iesolver.hh>

namespace Gringo::Input {

namespace {

struct ExtractBounds {
    void operator()(Literal const &lit) const { std::visit(*this, lit); }

    void operator()(auto const &lit) const { static_cast<void>(lit); }

    void operator()(LiteralRelation const &lit) const {
        IETermVec terms;
        auto const &rhs = lit.rhs.front();
        // handle intervals
        if (is_variable(lit.lhs) && is_interval(rhs.second)) {
            throw std::logic_error("implement me!!!");
        }
        // X >= Y -> X - Y >=  0
        // X >  Y -> X - Y >= -1
        // X <= Y -> Y - X >=  0
        // X <  Y -> Y - X >=  1
        // X =  Y -> X - Y >=  0
        //           Y - X >=  0
        // X != Y -> cannot handle
        int bound = 0;
        switch (rhs.first) {
            case Relation::greater: {
                bound = -1;
            }
            case Relation::greater_equal: {
                break;
            }
            case Relation::less: {
                bound = 1;
            }
            case Relation::less_equal: {
                break;
            }
            case Relation::equal: {
                break;
            }
            case Relation::inequal: {
                return;
            }
        }
        slv.add(IE{std::move(terms), bound});
        throw std::logic_error("implement me!!!");
    }

    IESolver &slv;
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
        // TODO: add pool/and refine bounds
        throw std::logic_error("implement me!!!");
        return {true};
    }

    RewriteContext &ctx;
};

} // namespace

[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Statement const &stm) -> Util::ResultState<Statement> {
    return ComputeBounds{ctx}(stm);
}

} // namespace Gringo::Input
