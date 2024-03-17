#include "graph.hh"

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/dependency.hh>

#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

#include <forward_list>

// TODO: remove
#include <gringo/input/algo/print.hh>
#include <iostream>
// TODO: remove

namespace Gringo::Input {

namespace {

using Signature = std::tuple<String, size_t, bool>;
using Dependency = std::tuple<size_t, Term const *, bool>;

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
            map[sig].emplace_back(idx, &term, false);
        }
        if (test(type, DependencyType::negative) || lit.sign() != Sign::none) {
            map[sig].emplace_back(idx, &term, true);
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
            map[sig].emplace_back(idx, &stm.atom(), false);
        }
        std::for_each(stm.body().begin(), stm.body().end(), *this);
    }

    size_t idx;
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

    template <class T, class U> auto unify_(T const &a, U const &b) = delete;

    template <class T, class U> auto match_(Symbol const &a, U const &b) = delete;

  private:
    auto match_(SymbolSpan a, ArgumentTuple const &b) -> bool {
        if (a.size() != b.elems().size()) {
            return false;
        }
        auto it = b.elems().begin();
        for (auto sym : a) {
            if (!std::visit(
                    [this, sym]<class T>(T const &c) {
                        if constexpr (Util::matches<T, Projection>) {
                            return true;
                        } else {
                            return match_(sym, c);
                        }
                    },
                    *it++)) {
                return false;
            }
        }
        return true;
    }

    auto match_(Symbol const &a, Term const &b) -> bool {
        return std::visit(
            [&, this]<class T>(T const &v_b) -> bool {
                if constexpr (Util::matches<T, TermVariable>) {
                    terms_.emplace_front(TermSymbol{v_b.loc(), a});
                    auto [it, ins] = ass_.try_emplace(v_b.name(), &terms_.front());
                    return ins || unify_(b, *it->second);
                } else if constexpr (Util::matches<T, TermSymbol>) {
                    return a == v_b.value();
                } else if constexpr (Util::matches<T, TermTuple>) {
                    return a.type() == SymbolType::tuple &&
                           match_(a.args(), std::get<ArgumentTuple>(v_b.pool().at(0)).elems());
                } else if constexpr (Util::matches<T, TermFunction>) {
                    return a.type() == SymbolType::function && a.name() == v_b.name() && !v_b.external() &&
                           !a.has_classical_sign() && match_(a.args(), v_b.pool().at(0));
                } else if constexpr (Util::matches<T, TermAbs>) {
                    return a.type() == SymbolType::number && *a.num() >= 0;
                } else if constexpr (Util::matches<T, TermUnary>) {
                    if (v_b.op() == UnaryOperator::invert) {
                        return a.type() == SymbolType::number;
                    }
                    if (a.type() == SymbolType::number) {
                        return match_(store_.num(-*a.num()), v_b.rhs());
                    }
                    if (a.type() == SymbolType::function) {
                        return match_(*a.flip_classical_sign(), v_b.rhs());
                    }
                    return false;
                } else {
                    static_assert(Util::matches<T, TermBinary>);
                    if (a.type() != SymbolType::number) {
                        return false;
                    }
                    if (auto l_b = check_linear(v_b); l_b) {
                        auto c = *a.num() - *l_b->n();
                        return c % l_b->m() == 0 && match_(store_.num(c / l_b->m()), l_b->term_x());
                    }
                    return true;
                }
            },
            b);
    }

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
            [&, this]<class A, class B>(A const &v_a, B const &v_b) -> bool {
                // variables
                if constexpr (Util::matches<A, TermVariable> || Util::matches<B, TermVariable>) {
                    if constexpr (Util::matches<A, TermVariable>) {
                        // TODO: occurs check
                        auto [it, ins] = ass_.try_emplace(v_a.name(), &b);
                        return ins || unify_(b, *it->second);
                    } else {
                        return unify_(b, a);
                    }
                }
                // symbols
                else if constexpr (Util::matches<A, TermSymbol> || Util::matches<B, TermSymbol>) {
                    if constexpr (Util::matches<A, TermSymbol>) {
                        return match_(v_a.value(), b);
                    } else {
                        return match_(v_b.value(), a);
                    }
                }
                // tuples
                else if constexpr (Util::matches<A, TermTuple> || Util::matches<B, TermTuple>) {
                    if constexpr (Util::matches<A, TermTuple> && Util::matches<B, TermTuple>) {
                        assert(v_a.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(v_a.pool().front()));
                        assert(v_b.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(v_b.pool().front()));
                        return unify_(std::get<ArgumentTuple>(v_a.pool().at(0)),
                                      std::get<ArgumentTuple>(v_b.pool().at(0)));
                    }
                    return false;
                }
                // functions
                else if constexpr (Util::matches<A, TermFunction> || Util::matches<B, TermFunction>) {
                    if constexpr (Util::matches<A, TermFunction> && Util::matches<B, TermFunction>) {
                        assert(v_a.pool().size() == 1);
                        assert(v_b.pool().size() == 1);
                        if (v_a.name() != v_b.name()) {
                            return false;
                        }
                        return unify_(v_a.pool().at(0), v_b.pool().at(0));
                    }
                    return false;
                }
                // absolute terms
                else if constexpr (Util::matches<A, TermAbs> || Util::matches<B, TermAbs>) {
                    if constexpr (!Util::matches<A, TermAbs>) {
                        return unify_(b, a);
                    } else {
                        return !never_numeric(b);
                    }
                }
                // linear terms
                else if constexpr (Util::matches<A, TermBinary> || Util::matches<B, TermBinary>) {
                    if constexpr (!Util::matches<A, TermBinary>) {
                        return unify_(b, a);
                    } else if constexpr (Util::matches<B, TermBinary>) {
                        // handle linear terms
                        if (auto l_a = check_linear(v_a), l_b = check_linear(v_b); l_a && l_b) {
                            if (auto it = ass_.find(l_a->x()); it != ass_.end() && never_numeric(*it->second)) {
                                return false;
                            }
                            if (l_a->x() == l_b->x()) {
                                auto n = (*l_b->n() - *l_a->n());
                                auto d = (*l_a->m() - *l_b->m());
                                return n == 0 || (d != 0 && std::move(n) % std::move(d) == 0);
                            }
                            if (auto it = ass_.find(l_b->x()); it != ass_.end() && never_numeric(*it->second)) {
                                return false;
                            }
                            if (*l_a->m() % *l_b->m() == 0) {
                                return unify_(*l_a, *l_b);
                            }
                            if (*l_b->m() % *l_a->m() == 0) {
                                return unify_(*l_b, *l_a);
                            }
                        }
                        return true;
                    } else if constexpr (Util::matches<B, TermUnary>) {
                        if (auto l_a = check_linear(v_a); l_a && v_b.op() == UnaryOperator::negate) {
                            terms_.emplace_front(TermBinary{
                                l_a->term_mxn().loc(),
                                TermBinary{location(l_a->term_mx()),
                                           TermSymbol{location(l_a->term_m()), store_.num(-*l_a->m())},
                                           BinaryOperator::times, l_a->term_x()},
                                BinaryOperator::plus, TermSymbol{location(l_a->term_n()), store_.num(-*l_a->n())}});
                            // -a ~ --b
                            return unify_(terms_.front(), *v_b.rhs());
                        }
                        return !never_numeric(b);
                    } else {
                        static_assert(Util::matches<B, void>);
                    }
                }
                // unary terms
                else if constexpr (Util::matches<A, TermUnary> || Util::matches<B, TermUnary>) {
                    if constexpr (!Util::matches<A, TermUnary>) {
                        return unify_(b, a);
                    } else if constexpr (Util::matches<B, TermUnary>) {
                        if (v_a.op() == v_b.op()) {
                            return unify_(*v_a.rhs(), *v_b.rhs());
                        }
                        if (v_a.op() == UnaryOperator::invert) {
                            return !never_numeric(v_b.rhs());
                        }
                        return !never_numeric(v_a.rhs());
                    } else {
                        static_assert(Util::matches<B, void>);
                    }
                } else {
                    static_assert(Util::matches<B, void>);
                }
            },
            a, b);
    }

    Assignment ass_;
    // or a dequeue...
    std::forward_list<Term> terms_;
    SymbolStore &store_;
};

struct Node {
    Stm const *stm = nullptr;
    std::vector<std::pair<size_t, bool>> depend = {};
    size_t scc = std::numeric_limits<size_t>::max();
    size_t idx = 0;
    bool normal = true;
};

auto build_nodes(SymbolStore &store_, std::vector<Stm> const &stms) -> std::vector<Node> {
    DependencyMap map_;
    std::vector<Node> nodes;
    nodes.reserve(stms.size());
    // add dependencies to the dependency map
    auto i = size_t{0};
    for (auto const &stm : stms) {
        nodes.emplace_back(&stm, std::vector<std::pair<size_t, bool>>{}, std::numeric_limits<size_t>::max(), 0, true);
        std::visit(AddDepend{i++, map_, nodes.back().normal}, stm);
    }
    // build the dependency graph
    ProvideVec provide;
    Unifier unifier{store_};
    i = 0;
    for (auto const &hd_stm : stms) {
        provide.clear();
        std::visit(AddProvide{provide}, hd_stm);
        for (auto const *hd_term : provide) {
            auto hd_sig = signature(*hd_term).value();
            if (auto it = map_.find(hd_sig); it != map_.end()) {
                for (auto const &[bd_idx, bd_term, bd_sign] : it->second) {
                    // TODO: variables in different contexts have to be renamed.
                    std::cerr << "unify: " << *hd_term << " ~ " << *bd_term << " = ";
                    if (unifier.unify(*hd_term, *bd_term)) {
                        nodes[bd_idx].depend.emplace_back(i, bd_sign);
                        std::cerr << "true" << std::endl;
                    } else {
                        std::cerr << "false" << std::endl;
                    }
                }
            }
        }
        ++i;
    }
    return nodes;
}

} // namespace

auto analyze(SymbolStore &store, std::vector<Stm> const &stms) -> Components {
    std::cerr << "analyze..." << std::endl;
    auto nodes = build_nodes(store, stms);
    // build graph considering positive and negative dependencies
    auto graph = Graph{};
    graph.ensure_size(nodes.size());
    auto i = size_t{0};
    for (auto const &node : nodes) {
        for (auto [j, sign] : node.depend) {
            graph.add_edge(i, j);
        }
        ++i;
    }
    graph.tarjan([&, num_scc = size_t{0}](auto const &scc) mutable {
        // tag sccs
        std::cerr << "scc:";
        size_t n = 0;
        for (auto i : scc) {
            nodes[i].scc = num_scc;
            nodes[i].idx = n;
            std::cerr << " " << i;
            ++n;
        }
        std::cerr << std::endl;
        // build graph considering only positive dependencies
        auto sub_graph = Graph{};
        sub_graph.ensure_size(n);
        for (auto i : scc) {
            for (auto const &[j, sign] : nodes[i].depend) {
                if (!sign && nodes[j].scc == num_scc) {
                    sub_graph.add_edge(nodes[i].idx, nodes[j].idx);
                }
            }
        }
        sub_graph.tarjan([&](auto const &sub_scc) {
            std::cerr << "  sub scc:";
            for (auto i : sub_scc) {
                std::cerr << " " << scc[i];
            }
            std::cerr << std::endl;
        });
        ++num_scc;
    });
    throw std::runtime_error("implement me!!!");
}

} // namespace Gringo::Input
