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

class LinearTerm {
  public:
    [[nodiscard]] auto m() const -> NumberRef { return std::get<TermSymbol>(m_).value().num(); }
    [[nodiscard]] auto n() const { return std::get<TermSymbol>(n_).value().num(); }
    [[nodiscard]] auto x() const { return std::get<TermVariable>(x_).name(); }
    [[nodiscard]] auto term_m() const { return m_; }
    [[nodiscard]] auto term_n() const { return n_; }
    [[nodiscard]] auto term_x() const { return x_; }
    [[nodiscard]] auto term_mx() const { return mx_; }
    [[nodiscard]] auto term_mxn() const { return mxn_; }

    friend auto check_linear(TermBinary const &term) -> std::optional<LinearTerm>;

  private:
    LinearTerm(TermBinary const &mxn, Term const &mx, Term const &m, Term const &x, Term const &n)
        : mxn_{mxn}, mx_{mx}, m_{m}, x_{x}, n_{n} {}

    TermBinary const &mxn_;
    Term const &mx_;
    Term const &m_;
    Term const &x_;
    Term const &n_;
};

auto check_linear(TermBinary const &term) -> std::optional<LinearTerm> {
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
    return LinearTerm{term, term.lhs().get(), mul->lhs().get(), mul->rhs().get(), term.rhs().get()};
}

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

    auto unify_(LinearTerm const &a, LinearTerm const &b) -> bool {
        auto n = *a.n() - *b.n();
        if (n % *b.m() != 0) {
            return true;
        }
        // var = m_a / m_b * tx + (n_a - n_b) / m_b
        auto m = *a.m() / *b.m();
        n /= *b.m();
        terms_.emplace_front(
            TermBinary{a.term_mxn().loc(),
                       TermBinary{location(a.term_mx()), TermSymbol{location(a.term_m()), store_.num(std::move(m))},
                                  BinaryOperator::times, a.term_x()},
                       BinaryOperator::plus, TermSymbol{location(a.term_n()), store_.num(std::move(n))}});
        return unify_(b.term_x(), terms_.front());
    }

    auto unify_(Term const &a, Term const &b) -> bool {
        if (a == b) {
            return true;
        }
        return std::visit(
            [&, this]<class A, class B>(A const &v_a, B const &v_b) {
                if constexpr (Util::matches<A, TermSymbol> && Util::matches<B, TermSymbol>) {
                    // TODO: maybe remove!!!
                    return false;
                }
                if constexpr (Util::matches<A, TermVariable>) {
                    // TODO: occurs check
                    if (auto [it, ins] = ass_.try_emplace(v_a.name(), &b); !ins) {
                        return unify_(b, *it->second);
                    }
                    return true;
                }
                if constexpr (Util::matches<B, TermVariable>) {
                    return unify_(b, a);
                }
                if constexpr (Util::matches<A, TermTuple> && Util::matches<B, TermTuple>) {
                    assert(v_a.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(v_a.pool().front()));
                    assert(v_b.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(v_b.pool().front()));
                    return unify_(std::get<ArgumentTuple>(v_a.pool().at(0)), std::get<ArgumentTuple>(v_b.pool().at(0)));
                }
                if constexpr (Util::matches<A, TermFunction> && Util::matches<B, TermFunction>) {
                    assert(v_a.pool().size() == 1);
                    assert(v_b.pool().size() == 1);
                    if (v_a.name() != v_b.name()) {
                        return false;
                    }
                    return unify_(v_a.pool().at(0), v_b.pool().at(0));
                }
                if constexpr (Util::matches<A, TermAbs> && Util::matches<B, TermAbs>) {
                    // can unify anything
                }
                if constexpr (Util::matches<A, TermUnary> && Util::matches<B, TermUnary>) {
                    // negated symbols have to be handled
                }
                if constexpr (Util::matches<A, TermBinary> && Util::matches<B, TermBinary>) {
                    // in general just returns true but (certain) linear terms are handled more cleverly
                    if (auto l_a = check_linear(v_a), l_b = check_linear(v_b); l_a && l_b) {
                        // TODO: make sure that neither x_a nor x_b is symbolic:
                        // - lookup x_a and x_b in assignment
                        // - make sure that the assignment is not symbolic
                        if (l_a->x() == l_b->x()) {
                            auto n = (*l_b->n() - *l_a->n());
                            auto d = (*l_a->m() - *l_b->m());
                            return n == 0 || (d != 0 && std::move(n) % std::move(d) == 0);
                        }
                        if (*l_a->m() % *l_b->m() == 0) {
                            return unify_(*l_a, *l_b);
                        }
                        if (*l_b->m() % *l_a->m() == 0) {
                            return unify_(*l_b, *l_a);
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
