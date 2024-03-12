#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/dependency.hh>

#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

namespace Gringo::Input {

namespace {

using Signature = std::tuple<String, size_t, bool>;
using Dependency = std::tuple<Stm const *, Term const *, bool>;

using DependencyMap = Util::unordered_map<Signature, std::vector<Dependency>>;
using ProvideVec = std::vector<Term const *>;

enum class DependencyType : uint32_t {
    positive = 1,
    negative = 2,
};
[[maybe_unused]] consteval void is_bit_set_enum(DependencyType flags);

//! Builder for the dependencies between statements.
struct AddDepend {

    void operator()(LitSymbolic const &lit, DependencyType type) const {
        auto const &term = lit.term();
        auto sig = signature(term).value();
        if (test(type, DependencyType::positive) && lit.sign() == Sign::none) {
            map[sig].emplace_back(&stm, &term, false);
        }
        if (test(type, DependencyType::negative) || lit.sign() != Sign::none) {
            map[sig].emplace_back(&stm, &term, true);
        }
    }

    template <class T>
        requires Util::is_among_v<T, LitComparison, LitSymbolic>
    void operator()([[maybe_unused]] T const &lit, [[maybe_unused]] DependencyType type) const {}

    void operator()(Lit const &lit, DependencyType type) const {
        std::visit([type, this](auto const &lit) { operator()(lit, type); }, lit);
    }

    void operator()(LitArray const &lits, DependencyType type) const {
        for (auto const &lit : lits) {
            operator()(lit, type);
        }
    }

    void operator()([[maybe_unused]] HdLitSimple const &lit) const {
        // Note: we can ignore the literal here because all negative literals
        // have been shifted to the body and positive literals do not induce a
        // dependency.
    }

    void operator()(HdLitDisjunction const &lit) const {
        normal = false;
        for (auto const &elem : lit.elems()) {
            std::visit(
                [this]<class T>(T const &lit) {
                    if constexpr (Util::matches<T, CondLit>) {
                        operator()(lit.cond(), DependencyType::positive);
                    }
                },
                elem);
        }
    }

    template <class T>
        requires Util::is_among_v<T, HdLitSetAggregate, HdLitAggregate, HdLitTheoryAtom>
    void operator()(T const &lit) const {
        normal = false;
        for (auto const &elem : lit.elems()) {
            operator()(elem.cond(), DependencyType::positive);
        }
    }

    void operator()(HdLit const &lit) const { std::visit(*this, lit); }

    void operator()(BdLitSimple const &lit) const { operator()(lit.lit(), DependencyType::positive); }

    void operator()(BdLitConjunction const &lit) const {
        operator()(lit.lit().lit(), DependencyType::positive);
        operator()(lit.lit().cond(), DependencyType::positive | DependencyType::negative);
    }

    void operator()(BdLitSetAggregate const &lit) const {
        auto type = DependencyType::positive;
        if (!reduct_is_monotone(lit.lhs(), AggregateFunction::count, lit.rhs())) {
            type |= DependencyType::negative;
        }
        for (auto const &elem : lit.elems()) {
            operator()(elem.lit(), type);
            operator()(elem.cond(), type);
        }
    }

    void operator()(BdLitAggregate const &lit) const {
        auto type = DependencyType::positive;
        if (!reduct_is_monotone(lit.lhs(), lit.fun(), lit.rhs())) {
            type |= DependencyType::negative;
        }
        for (auto const &elem : lit.elems()) {
            operator()(elem.cond(), type);
        }
    }

    void operator()(BdLitTheoryAtom const &lit) const {
        normal = false;
        for (auto const &elem : lit.elems()) {
            operator()(elem.cond(), DependencyType::positive);
        }
    }

    void operator()(BdLit const &lit) const { return std::visit(*this, lit); }

    template <class T>
        requires Util::is_among_v<T, StmTheory, StmOptimize, StmShowSig, StmProjectSig, StmDefined, StmScript,
                                  StmInclude, StmProgram, StmConst, StmComment>
    void operator()([[maybe_unused]] T const &stm) const {}

    template <class T>
        requires Util::is_among_v<T, StmRule, StmWeakConstraint, StmShow, StmProject, StmExternal, StmEdge,
                                  StmHeuristic>
    void operator()(T const &stm) const {
        if constexpr (Util::matches<T, StmRule>) {
            std::visit(*this, stm.head());
        }
        if constexpr (Util::matches<T, StmProject, StmHeuristic>) {
            auto sig = signature(stm.atom()).value();
            map[sig].emplace_back(&this->stm, &stm.atom(), false);
        }
        std::for_each(stm.body().begin(), stm.body().end(), *this);
    }

    Stm const &stm;
    DependencyMap &map;
    bool &normal;
};

struct AddProvide {
    void operator()(LitSymbolic const &lit) const {
        // Note: could be made an assertion
        if (lit.sign() == Sign::none) {
            provide.emplace_back(&lit.term());
        }
    }

    template <class T>
        requires Util::is_among_v<T, LitComparison, LitSymbolic, HdLitTheoryAtom>
    void operator()([[maybe_unused]] T const &lit) const {}

    void operator()(Lit const &lit) const { std::visit(*this, lit); }

    void operator()([[maybe_unused]] HdLitSimple const &lit) const { std::visit(*this, lit.lit()); }

    void operator()(HdLitDisjunction const &lit) const {
        for (auto const &elem : lit.elems()) {
            std::visit(
                [this]<class T>(T const &lit) {
                    if constexpr (Util::matches<T, CondLit>) {
                        std::visit(*this, lit.lit());
                    } else {
                        std::visit(*this, lit);
                    }
                },
                elem);
        }
    }

    template <class T>
        requires Util::is_among_v<T, HdLitSetAggregate, HdLitAggregate>
    void operator()(T const &lit) const {
        for (auto const &elem : lit.elems()) {
            operator()(elem.lit());
        }
    }

    void operator()(HdLit const &lit) const { std::visit(*this, lit); }
    template <class T>
        requires Util::is_among_v<T, StmWeakConstraint, StmShow, StmProject, StmEdge, StmHeuristic, StmTheory,
                                  StmOptimize, StmShowSig, StmProjectSig, StmDefined, StmScript, StmInclude, StmProgram,
                                  StmConst, StmComment>
    void operator()([[maybe_unused]] T const &stm) const {}

    void operator()(StmRule const &stm) const { std::visit(*this, stm.head()); }

    void operator()(StmExternal const &stm) const { provide.emplace_back(&stm.atom()); }

    ProvideVec &provide;
};

auto unify(Term const &a, Term const &b) -> bool {
    static_cast<void>(a);
    static_cast<void>(b);
    // X ~ Y
    // map[X] ~ map[Y]
    //
    // f(A) ~ f(B) -> A ~ B
    // f(A) ~ f(B) -> A ~ B
    return false;
}

struct Node {
    Stm const *stm;
    bool normal;
};

class DependencyGraph {
  public:
    DependencyGraph() = default;
    void add(std::vector<Stm> const &stms) {
        // add dependencies to the dependency map
        for (auto const &stm : stms) {
            nodes_.emplace_back(Node{&stm, true});
            std::visit(AddDepend{stm, map_, nodes_.back().normal}, stm);
        }
        // build the dependency graph
        ProvideVec provide;
        auto node_it = nodes_.begin();
        for (auto const &hd_stm : stms) {
            provide.clear();
            std::visit(AddProvide{provide}, hd_stm);
            for (auto const *hd_term : provide) {
                auto hd_sig = signature(*hd_term).value();
                if (auto it = map_.find(hd_sig); it != map_.end()) {
                    for (auto const &[bd_stm, bd_term, bd_sign] : it->second) {
                        static_cast<void>(bd_stm);
                        static_cast<void>(bd_sign);
                        if (unify(*hd_term, *bd_term)) {
                            throw std::logic_error("add dependency to positive/negative graph");
                        }
                    }
                }
            }
            ++node_it;
        }
    }

  private:
    DependencyMap map_;
    std::vector<Node> nodes_;
};

} // namespace

auto analyze(std::vector<Stm> const &stms) -> Components {
    auto gph = DependencyGraph{};
    gph.add(stms);
    throw std::runtime_error("implement me!!!");
}

} // namespace Gringo::Input
