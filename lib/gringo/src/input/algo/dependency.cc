#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/dependency.hh>

#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

#include <forward_list>

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

using Assignment = Util::unordered_map<String, Term const *>;

class Unifier {
  public:
    Unifier(SymbolStore &store) : store_{store} {}
    auto unify(Term const &a, Term const &b) -> bool {
        ass_.clear();
        return unify_(a, b);
    }

  private:
    auto unify_(ArgumentTuple const &a, ArgumentTuple const &b) -> bool {
        if (a.elems().size() != b.elems().size()) {
            return false;
        }
        for (auto it = a.elems().begin(), jt = b.elems().begin(), ie = a.elems().end(); it != ie; ++it, ++jt) {
            if (!std::visit(
                    [this]<class EA, class EB>(EA const &a, EB const &b) {
                        if constexpr (Util::matches<EA, Projection> || Util::matches<EB, Projection>) {
                            return true;
                        } else {
                            return unify_(a, b);
                        }
                    },
                    *it, *jt)) {
                return false;
            }
        }
        return true;
    }

    static auto check_linear_(TermBinary const &term)
        -> std::optional<std::tuple<TermBinary const &, TermSymbol const &, TermVariable const &, TermSymbol const &>> {
        if (term.op() != BinaryOperator::plus) {
            return std::nullopt;
        }
        auto const *mul = std::get_if<TermBinary>(&term.lhs().get());
        if (mul == nullptr || mul->op() != BinaryOperator::times) {
            return std::nullopt;
        }
        auto const *n = std::get_if<TermSymbol>(&term.rhs().get());
        if (n == nullptr || n->value().type() != SymbolType::number) {
            return std::nullopt;
        }
        auto const *m = std::get_if<TermSymbol>(&mul->lhs().get());
        if (m == nullptr || m->value().type() != SymbolType::number || *m->value().num() == 0) {
            return std::nullopt;
        }
        auto const *v = std::get_if<TermVariable>(&mul->rhs().get());
        if (v == nullptr) {
            return std::nullopt;
        }
        return std::forward_as_tuple(*mul, *m, *v, *n);
    }

    auto unify_(Term const &a, Term const &b) -> bool {
        if (a == b) {
            return true;
        }
        return std::visit(
            [&, this]<class A, class B>(A const &a_v, B const &b_v) {
                if constexpr (Util::matches<A, TermSymbol> && Util::matches<B, TermSymbol>) {
                    // TODO: maybe remove!!!
                    return false;
                }
                if constexpr (Util::matches<A, TermVariable>) {
                    // TODO: occurs check
                    if (auto [it, ins] = ass_.try_emplace(a_v.name(), &b); !ins) {
                        return unify_(b, *it->second);
                    }
                    return true;
                }
                if constexpr (Util::matches<B, TermVariable>) {
                    return unify_(b, a);
                }
                if constexpr (Util::matches<A, TermTuple> && Util::matches<B, TermTuple>) {
                    assert(a_v.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(a_v.pool().front()));
                    assert(b_v.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(b_v.pool().front()));
                    return unify_(std::get<ArgumentTuple>(a_v.pool().at(0)), std::get<ArgumentTuple>(b_v.pool().at(0)));
                }
                if constexpr (Util::matches<A, TermFunction> && Util::matches<B, TermFunction>) {
                    assert(a_v.pool().size() == 1);
                    assert(b_v.pool().size() == 1);
                    if (a_v.name() != b_v.name()) {
                        return false;
                    }
                    return unify_(a_v.pool().at(0), b_v.pool().at(0));
                }
                if constexpr (Util::matches<A, TermAbs> && Util::matches<B, TermAbs>) {
                    // can unify anything
                }
                if constexpr (Util::matches<A, TermUnary> && Util::matches<B, TermUnary>) {
                    // negated symbols have to be handled
                }
                if constexpr (Util::matches<A, TermBinary> && Util::matches<B, TermBinary>) {
                    // in general just returns true but (certain) linear terms are handled more cleverly
                    if (auto lin_a = check_linear_(a_v), lin_b = check_linear_(b_v); lin_a && lin_b) {
                        auto [tmx_a, tm_a, tx_a, tn_a] = *lin_a;
                        auto [tmx_b, tm_b, tx_b, tn_b] = *lin_b;
                        auto m_a = tm_a.value().num();
                        auto x_a = tx_a.name();
                        auto n_a = tn_a.value().num();
                        auto m_b = tm_b.value().num();
                        auto x_b = tx_b.name();
                        auto n_b = tn_b.value().num();
                        // TODO: make sure that neither x_a nor x_b is symbolic:
                        // - lookup x_a and x_b in assignment
                        // - make sure that the assignment is not symbolic
                        if (x_a == x_b) {
                            auto n = (*n_b - *n_a);
                            auto d = (*m_a - *m_b);
                            return n == 0 || (d != 0 && std::move(n) % std::move(d) == 0);
                        }
                        auto unify_linear = [this](auto const &tmxn, auto const &tmx, auto const &tm, auto const &tx,
                                                   auto const &tn, auto const &var, auto const &m_a, auto const &m_b,
                                                   auto const &n_a, auto const &n_b) {
                            auto n = *n_a - *n_b;
                            if (n % *m_b != 0) {
                                return true;
                            }
                            // var = m_a / m_b * tx + (n_a - n_b) / m_b
                            auto m = *m_a / *m_b;
                            n /= *m_b;
                            terms_.emplace_front(
                                TermBinary{tmxn.loc(),
                                           TermBinary{tmx.loc(), TermSymbol{tm.loc(), store_.num(std::move(m))},
                                                      BinaryOperator::times, tx},
                                           BinaryOperator::plus, TermSymbol{tn.loc(), store_.num(std::move(n))}});
                            return unify_(var, terms_.front());
                        };
                        if (*m_a % *m_b == 0) {
                            return unify_linear(a_v, tmx_a, tm_a, tx_a, tn_a, tx_b, m_a, m_b, n_a, n_b);
                        }
                        if (*m_b % *m_a == 0) {
                            return unify_linear(b_v, tmx_b, tm_b, tx_b, tn_b, tx_a, m_b, m_a, n_b, n_a); // NOLINT
                        }
                    }
                    return true;
                }
                return false;
            },
            a, b);
    }

    Assignment ass_;
    std::forward_list<Term> terms_;
    SymbolStore &store_;
};

struct Node {
    Stm const *stm;
    bool normal;
};

class DependencyGraph {
  public:
    DependencyGraph(SymbolStore &store) : store_{store} {}
    void add(std::vector<Stm> const &stms) {
        // add dependencies to the dependency map
        for (auto const &stm : stms) {
            nodes_.emplace_back(Node{&stm, true});
            std::visit(AddDepend{stm, map_, nodes_.back().normal}, stm);
        }
        // build the dependency graph
        ProvideVec provide;
        Unifier unifier{store_};
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
                        // Variables in different contexts have to be renamed.
                        if (unifier.unify(*hd_term, *bd_term)) {
                            throw std::logic_error("add dependency to positive/negative graph");
                        }
                    }
                }
            }
            ++node_it;
        }
    }

  private:
    SymbolStore &store_;
    DependencyMap map_;
    std::vector<Node> nodes_;
};

} // namespace

auto analyze(SymbolStore &store, std::vector<Stm> const &stms) -> Components {
    auto gph = DependencyGraph{store};
    gph.add(stms);
    throw std::runtime_error("implement me!!!");
}

} // namespace Gringo::Input
