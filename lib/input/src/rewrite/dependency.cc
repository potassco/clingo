#include "transform.hh"

#include <clingo/input/print.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/dependency.hh>

#include <clingo/util/graph.hh>
#include <clingo/util/macro.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/type_traits.hh>
#include <clingo/util/unordered_map.hh>

#include <deque>

namespace CppClingo::Input {

namespace {

using Dependency = std::tuple<size_t, Term const *, bool>;

using DependencyMap = Util::unordered_map<Sig, std::vector<Dependency>>;
using ProvideVec = std::vector<Term const *>;

enum class DependencyType : uint8_t {
    positive = 1,
    negative = 2,
};
CLINGO_ENABLE_BITSET_ENUM(DependencyType);

auto safe_sig(Term const &term) -> std::tuple<String, size_t, bool> {
    return signature(term).value(); // NOLINT(bugprone-unchecked-optional-access)
}

//! Builder for the dependencies between statements.
class AddDepend {
  public:
    AddDepend(size_t idx, DependencyMap &map, bool &normal) : idx_{idx}, map_{&map}, normal_{&normal} {}

    void operator()(LitSymbolic const &lit, DependencyType type) const {
        auto const &term = lit.term();
        auto sig = safe_sig(term);
        if (intersects(type, DependencyType::positive) && lit.sign() == Sign::none) {
            map_->operator[](sig).emplace_back(idx_, &term, false);
        }
        if (intersects(type, DependencyType::negative) || lit.sign() != Sign::none) {
            map_->operator[](sig).emplace_back(idx_, &term, true);
        }
    }

    template <class T>
        requires Util::is_among_v<T, LitComparison, LitBool>
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
        *normal_ = false;
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
        *normal_ = false;
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
        *normal_ = false;
        for (auto const &elem : lit.elems()) {
            operator()(elem.cond(), DependencyType::positive);
        }
    }

    void operator()(BdLit const &lit) const { std::visit(*this, lit); }

    template <class T>
        requires Util::is_among_v<T, StmTheory, StmOptimize, StmShowNothing, StmShowSig, StmProjectSig, StmDefined,
                                  StmScript, StmInclude, StmProgram, StmConst, StmParts, StmComment>
    void operator()([[maybe_unused]] T const &stm) const {}

    template <class T>
        requires Util::is_among_v<T, StmRule, StmWeakConstraint, StmShow, StmProject, StmExternal, StmEdge,
                                  StmHeuristic>
    void operator()(T const &stm) const {
        if constexpr (Util::matches<T, StmRule>) {
            std::visit(*this, stm.head());
        }
        if constexpr (Util::matches<T, StmProject, StmHeuristic>) {
            auto sig = safe_sig(stm.atom());
            map_->operator[](sig).emplace_back(idx_, &stm.atom(), false);
        }
        std::for_each(stm.body().begin(), stm.body().end(), *this);
    }

  private:
    size_t idx_;
    DependencyMap *map_;
    bool *normal_;
};

class AddProvide {
  public:
    AddProvide(ProvideVec &provide) : provide_{&provide} {}

    void operator()(LitSymbolic const &lit) const {
        // Note: could be made an assertion
        if (lit.sign() == Sign::none) {
            provide_->emplace_back(&lit.term());
        }
    }

    template <class T> void operator()(T const &lit) const {
        if constexpr (Util::is_among_v<T, HdLitSetAggregate, HdLitAggregate>) {
            for (auto const &elem : lit.elems()) {
                operator()(elem.lit());
            }
        } else {
            static_assert(Util::is_among_v<T, LitComparison, LitBool, HdLitTheoryAtom, StmWeakConstraint, StmShow,
                                           StmProject, StmEdge, StmHeuristic, StmTheory, StmOptimize, StmShowNothing,
                                           StmShowSig, StmProjectSig, StmDefined, StmScript, StmInclude, StmProgram,
                                           StmConst, StmParts, StmComment>);
        }
    }

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

    void operator()(HdLit const &lit) const { std::visit(*this, lit); }

    void operator()(StmRule const &stm) const { std::visit(*this, stm.head()); }

    void operator()(StmExternal const &stm) const { provide_->emplace_back(&stm.atom()); }

  private:
    ProvideVec *provide_;
};

using Assignment = Util::unordered_map<String, Term const *>;

class Unifier {
  public:
    Unifier(SymbolStore &store) : store_{&store} {}
    auto unify(Term const &a, Term const &b) -> bool {
        ass_.clear();
        return unify_(a, b);
    }

    template <class T, class U> auto unify_(T const &a, U const &b) = delete;

    template <class T, class U> auto match_(Symbol const &a, U const &b) = delete;

  private:
    auto occurs_check_(String name, ArgumentTuple const &tup) -> bool {
        return std::ranges::all_of(tup.elems(), [name, this](auto const &arg) {
            if (auto const *term = std::get_if<Term>(&arg); term != nullptr) {
                return occurs_check_(name, *term);
            }
            return false;
        });
    }

    auto occurs_check_(String name, Term const &b) -> bool {
        // Note: assumes that arithmetic operations are wrapped within tuples/functions
        return std::visit(
            [&, this]<class T>(T const &v_b) -> bool {
                if constexpr (Util::matches<T, TermVariable>) {
                    return name != v_b.name();
                } else if constexpr (Util::matches<T, TermSymbol>) {
                    return true;
                } else if constexpr (Util::matches<T, TermTuple>) {
                    return std::ranges::all_of(v_b.pool(), [name, this](auto const &x) {
                        return std::visit([name, this](auto const &x) { return this->occurs_check_(name, x); }, x);
                    });
                } else if constexpr (Util::matches<T, TermFunction>) {
                    return std::ranges::all_of(v_b.pool(),
                                               [name, this](auto const &x) { return this->occurs_check_(name, x); });
                } else if constexpr (Util::matches<T, TermAbs>) {
                    return std::ranges::all_of(v_b.pool(),
                                               [name, this](auto const &x) { return this->occurs_check_(name, x); });
                } else if constexpr (Util::matches<T, TermUnary>) {
                    return occurs_check_(name, *v_b.rhs());
                } else {
                    static_assert(Util::matches<T, TermBinary>);
                    return occurs_check_(name, *v_b.lhs()) && occurs_check_(name, *v_b.rhs());
                }
            },
            b);
    }

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
                    terms_.emplace_back(TermSymbol{v_b.loc(), a});
                    auto [it, ins] = ass_.try_emplace(v_b.name(), &terms_.back());
                    return ins || match_(a, *it->second);
                } else if constexpr (Util::matches<T, TermSymbol>) {
                    return a == v_b.value();
                } else if constexpr (Util::matches<T, TermTuple>) {
                    return a.type() == SymbolType::tuple &&
                           match_(a.args(), std::get<ArgumentTuple>(v_b.pool().at(0)).elems());
                } else if constexpr (Util::matches<T, TermFunction>) {
                    return a.type() == SymbolType::function && a.name() == v_b.name() && !v_b.external() &&
                           !a.has_classical_sign() && match_(a.args(), v_b.pool().at(0));
                } else if constexpr (Util::matches<T, TermAbs>) {
                    return a.type() == SymbolType::number && a.num() >= 0;
                } else if constexpr (Util::matches<T, TermUnary>) {
                    if (v_b.op() == UnaryOperator::negate) {
                        return a.type() == SymbolType::number;
                    }
                    if (a.type() == SymbolType::number) {
                        return match_(store_->num_ref(-a.num()), v_b.rhs());
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
                        auto c = a.num() - l_b->n();
                        return c % l_b->m() == 0 && match_(store_->num_ref(c / l_b->m()), l_b->term_x());
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
        auto n = a.n() - b.n();
        if (n % b.m() != 0) {
            return true;
        }
        // var = m_a / m_b * tx + (n_a - n_b) / m_b
        auto m = a.m() / b.m();
        n /= b.m();
        terms_.emplace_back(TermBinary{
            a.term_mxn().loc(),
            TermBinary{location(a.term_mx()), TermSymbol{location(a.term_m()), store_->num_ref(std::move(m))},
                       BinaryOperator::times, a.term_x()},
            BinaryOperator::plus, TermSymbol{location(a.term_n()), store_->num_ref(std::move(n))}});
        auto [it, ins] = ass_.try_emplace(b.x(), &terms_.back());
        return ins || unify_(terms_.back(), *it->second);
    }

    auto unify_(Term const &a, Term const &b) -> bool {
        return std::visit(
            [&, this]<class A, class B>(A const &v_a, B const &v_b) -> bool {
                // variables
                if constexpr (Util::matches<A, TermVariable> || Util::matches<B, TermVariable>) {
                    if constexpr (Util::matches<A, TermVariable>) {
                        if constexpr (Util::matches<B, TermAbs>) {
                            return true;
                        } else if constexpr (Util::matches<B, TermUnary, TermBinary>) {
                            terms_.emplace_back(TermBinary{
                                v_a.loc(),
                                TermBinary{v_a.loc(), TermSymbol{v_a.loc(), store_->num_ref(Number(1))},
                                           BinaryOperator::times, a},
                                BinaryOperator::plus, TermSymbol{v_a.loc(), CppClingo::SymbolStore::num_ref(0)}});
                            return unify_(terms_.back(), b);
                        } else {
                            if (a == b) {
                                return true;
                            }
                            auto [it, ins] = ass_.try_emplace(v_a.name(), &b);
                            return ins ? occurs_check_(v_a.name(), b) : unify_(b, *it->second);
                        }
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
                                auto n = (l_b->n() - l_a->n());
                                auto d = (l_a->m() - l_b->m());
                                if (d != 0 && n % d == 0) {
                                    terms_.emplace_back(
                                        TermSymbol{v_a.loc(), store_->num_ref(std::move(n) / std::move(d))});
                                    return unify_(l_a->term_x(), terms_.back());
                                }
                                return n == 0;
                            }
                            if (auto it = ass_.find(l_b->x()); it != ass_.end() && never_numeric(*it->second)) {
                                return false;
                            }
                            if (l_a->m() % l_b->m() == 0) {
                                return unify_(*l_a, *l_b);
                            }
                            if (l_b->m() % l_a->m() == 0) {
                                return unify_(*l_b, *l_a);
                            }
                        }
                        return true;
                    } else if constexpr (Util::matches<B, TermUnary>) {
                        if (auto l_a = check_linear(v_a); l_a && v_b.op() == UnaryOperator::minus) {
                            terms_.emplace_back(TermBinary{
                                l_a->term_mxn().loc(),
                                TermBinary{location(l_a->term_mx()),
                                           TermSymbol{location(l_a->term_m()), store_->num_ref(-l_a->m())},
                                           BinaryOperator::times, l_a->term_x()},
                                BinaryOperator::plus, TermSymbol{location(l_a->term_n()), store_->num_ref(-l_a->n())}});
                            // -a ~ --b
                            return unify_(terms_.back(), *v_b.rhs());
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
                        if (v_a.op() == UnaryOperator::negate) {
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
    std::deque<Term> terms_;
    SymbolStore *store_;
};

struct Edge {
    //! The statement index.
    size_t index;
    //! The body atom representation.
    Term const *bd;
    //! The head atom representation.
    Term const *hd;
    //! Whether the edge is negative.
    bool is_negative;

    Edge(size_t index, Term const *from, Term const *to, bool is_negative)
        : index(index), bd(from), hd(to), is_negative(is_negative) {}
};

struct Node {
    //! The list of incoming edges.
    std::vector<Edge> depend;
    //! The scc index of the node.
    size_t scc = std::numeric_limits<size_t>::max();
    //! The refined scc index of the node.
    size_t sub_scc = std::numeric_limits<size_t>::max();
    //! The running index in the refined scc.
    size_t idx = 0;
    //! Whether the head of the statement is derived normally.
    //!
    //! This is the case for normal rules but not for choices, externals, etc.
    bool normal = true;
};

class VariableRenamer : public Transformer<VariableRenamer> {
  public:
    VariableRenamer(NameGen &gen, Util::unordered_map<String, String> &map) : gen_{&gen}, map_{&map} {}
    [[nodiscard]] auto accept(TermVariable const &var) const -> std::optional<Term> {
        auto [it, ins] = map_->try_emplace(var.name(), String{});
        if (ins) {
            it.value() = gen_->new_name();
        }
        return var.update(a_name = it.value());
    }

  private:
    NameGen *gen_;
    Util::unordered_map<String, String> *map_;
};

auto rename(SymbolStore &store, Term const &term) -> std::optional<Term> {
    auto gen = NameGen{store, {}, "#U"};
    auto map = Util::unordered_map<String, String>{};
    return VariableRenamer{gen, map}.transform(term);
}

auto build_nodes(SymbolStore &store, std::vector<Stm> const &stms) -> std::vector<Node> {
    auto map = DependencyMap{};
    auto nodes = std::vector<Node>{};
    nodes.reserve(stms.size());
    // add dependencies to the dependency map
    auto i = size_t{0};
    for (auto const &stm : stms) {
        nodes.emplace_back();
        std::visit(AddDepend{i++, map, nodes.back().normal}, stm);
    }
    // build the dependency graph
    auto provide = ProvideVec{};
    auto unifier = Unifier{store};
    i = 0;
    for (auto const &hd_stm : stms) {
        provide.clear();
        std::visit(AddProvide{provide}, hd_stm);
        for (auto const *hd_term : provide) {
            auto hd_term_r = rename(store, *hd_term);
            auto const *hd_term_u = hd_term_r ? &*hd_term_r : hd_term;
            auto hd_sig = safe_sig(*hd_term_u);
            if (auto it = map.find(hd_sig); it != map.end()) {
                for (auto const &[bd_idx, bd_term, bd_sign] : it->second) {
                    if (unifier.unify(*hd_term_u, *bd_term)) {
                        // bd_term is provided by statement i
                        // in principle one could also store
                        // that hd_term updates the index of bd_term
                        // this would avoid having to update later on
                        // depending on how indices are designed
                        // edges within components are sufficient
                        nodes[bd_idx].depend.emplace_back(i, bd_term, hd_term, bd_sign);
                    }
                }
            }
        }
        ++i;
    }
    return nodes;
}

template <class T> void encode_html(T const &stm, std::ostream &out) {
    auto oss = std::stringstream{};
    oss << stm;
    for (auto c : oss.str()) {
        switch (c) {
            case '&': {
                out << "&amp;";
                break;
            }
            case '\"': {
                out << "&quot;";
                break;
            }
            case '\'': {
                out << "&apos;";
                break;
            }
            case '<': {
                out << "&lt;";
                break;
            }
            case '>': {
                out << "&gt;";
                break;
            }
            default: {
                out << c;
                break;
            }
        }
    }
}

} // namespace

auto analyze(SymbolStore &store, StmVec const &stms, std::vector<Stm const *> *srcs) -> Components {
    assert(srcs == nullptr || stms.size() == srcs->size());
    auto nodes = build_nodes(store, stms);
    // build graph considering positive and negative dependencies
    auto graph = Util::Graph{};
    graph.ensure_size(nodes.size());
    auto i = size_t{0};
    for (auto const &node : nodes) {
        for (auto [j, _bd_term, _hd_term, sign] : node.depend) {
            graph.add_edge(i, j);
        }
        ++i;
    }
    auto comps = Components{};
    graph.tarjan([&, num_scc = size_t{0}](auto const &scc) mutable {
        comps.emplace_back();
        auto &sub_comps = comps.back();
        // tag sccs
        auto n = size_t{0};
        for (auto i : scc) {
            nodes[i].scc = num_scc;
            nodes[i].idx = n;
            ++n;
        }
        // build graph considering only positive dependencies
        auto sub_graph = Util::Graph{};
        sub_graph.ensure_size(n);
        for (auto i : scc) {
            for (auto const &[j, _bd_term, _hd_term, sign] : nodes[i].depend) {
                if (!sign && nodes[j].scc == num_scc) {
                    sub_graph.add_edge(nodes[i].idx, nodes[j].idx);
                }
            }
        }
        sub_graph.tarjan([&, num_sub_scc = size_t{0}](auto const &sub_scc) mutable {
            auto comp = Component{};
            comp.stms.reserve(sub_scc.size());
            if (srcs != nullptr) {
                comp.srcs.reserve(sub_scc.size());
            }
            for (auto i : sub_scc) {
                nodes[scc[i]].sub_scc = num_sub_scc;
            }
            for (auto i : sub_scc) {
                auto const &stm = stms[scc[i]];
                if (!nodes[scc[i]].normal) {
                    comp.type -= ComponentType::positive;
                }
                comp.stms.emplace_back(&stm);
                if (srcs != nullptr) {
                    comp.srcs.emplace_back((*srcs)[scc[i]]);
                }
                for (auto const &[idx, bd_term, hd_term, sign] : nodes[scc[i]].depend) {
                    auto const &node = nodes[idx];
                    comp.depend.emplace(safe_sig(*bd_term));
                    if (std::tie(node.scc, node.sub_scc) >= std::tie(num_scc, num_sub_scc)) {
                        if (sign) {
                            comp.type -= ComponentType::positive;
                        } else {
                            comp.type -= ComponentType::single_pass;
                        }
                        auto &hd_terms = comp.incomplete[bd_term];
                        if (node.sub_scc == num_sub_scc) {
                            hd_terms.emplace(hd_term);
                        }
                    }
                }
            }
            sub_comps.emplace_back(std::move(comp));
            ++num_sub_scc;
        });
        ++num_scc;
    });
    return comps;
}

auto unify(SymbolStore &store, Term const &a, Term const &b) -> bool {
    return Unifier{store}.unify(a, b);
}

void visualize(Components const &comps, std::ostream &out) {
    out << "digraph {\n";
    auto i = size_t{0};
    for (auto const &sub_comps : comps) {
        out << "  subgraph cluster_" << i << " {\n";
        out << "    label = \"component " << i << "\";\n";
        auto j = size_t{0};
        for (auto const &comp : sub_comps) {
            out << "    subgraph cluster_" << i << "_" << j << " {\n";
            out << "      label = \"subcomponent " << j << "\";\n";
            out << "      stms_" << i << "_" << j << " [label=<";
            out << "statements[" << (intersects(comp.type, ComponentType::positive) ? "positive" : "negative") << ", "
                << (intersects(comp.type, ComponentType::single_pass) ? "single-pass" : "multi-pass") << "]:";
            for (auto const &stm : comp.stms) {
                out << "<br/>";
                encode_html(*stm, out);
            }
            if (!comp.incomplete.empty()) {
                out << "<br/>incomplete:";
                bool comma = false;
                Util::ordered_set<std::reference_wrapper<Term const>> incomplete;
                for (auto const &[bd, hd] : comp.incomplete) {
                    if (incomplete.emplace(*bd).second) {
                        if (comma) {
                            out << ",";
                        } else {
                            comma = true;
                        }
                        out << " ";
                        encode_html(*bd, out);
                    }
                }
            }
            out << ">];\n";
            out << "    }\n";
            ++j;
        }
        out << "  }\n";
        ++i;
    }
    if (!comps.empty()) {
        bool comma = false;
        out << "  ";
        for (auto i = size_t{0}; i < comps.size(); ++i) {
            for (auto j = size_t{0}; j < comps[i].size(); ++j) {
                if (comma) {
                    out << " -> ";
                } else {
                    comma = true;
                }
                out << "stms_" << i << "_" << j;
            }
        }
        out << " [style=\"invis\"];\n";
    }
    out << "}\n";
}

namespace {

class AnalyzeVisitor {
  public:
    AnalyzeVisitor(SharedSigSet &provide, Util::ordered_map<SharedSig, Location> &depend)
        : provide_{&provide}, depend_{&depend} {}

    // simple literals
    void visit([[maybe_unused]] LitBool const &lit, [[maybe_unused]] bool head) {}
    void visit([[maybe_unused]] LitComparison const &lit, [[maybe_unused]] bool head) {}
    void visit(LitSymbolic const &lit, bool head) { add_(lit.term(), head && lit.sign() == Sign::none); }

    // elements
    void visit(CondLit const &lit, bool head) {
        visit(lit.lit(), head);
        visit(lit.cond(), false);
    }
    void visit(SetAggregateElement const &elem, bool head) {
        visit(elem.lit(), head);
        visit(elem.cond(), false);
    }
    void visit(HdLitAggregateElement const &elem) {
        visit(elem.lit(), true);
        visit(elem.cond(), false);
    }
    void visit(TheoryElement const &elem) { visit(elem.cond(), false); }
    void visit(BdLitAggregateElement const &elem) { visit(elem.cond(), false); }
    void visit(OptimizeElement const &elem) { visit(elem.cond(), false); }

    // head literals
    void visit(HdLitSimple const &hd_lit) { visit(hd_lit.lit(), true); }
    void visit(HdLitDisjunction const &hd_lit) { visit(hd_lit.elems(), true); }
    void visit(HdLitAggregate const &hd_lit) { visit(hd_lit.elems()); }
    void visit(HdLitSetAggregate const &hd_lit) { visit(hd_lit.elems(), true); }
    void visit(HdLitTheoryAtom const &hd_lit) { visit(hd_lit.elems()); }

    // body literals
    void visit(BdLitSimple const &bd_lit) { visit(bd_lit.lit(), false); }
    void visit(BdLitConjunction const &bd_lit) { visit(bd_lit.lit(), false); }
    void visit(BdLitAggregate const &bd_lit) { visit(bd_lit.elems()); }
    void visit(BdLitSetAggregate const &bd_lit) { visit(bd_lit.elems(), false); }
    void visit(BdLitTheoryAtom const &bd_lit) { visit(bd_lit.elems()); }

    // statements
    void visit(StmRule const &stm) {
        visit(stm.head());
        visit(stm.body());
    }
    void visit([[maybe_unused]] StmTheory const &stm) {}
    void visit(StmOptimize const &stm) { visit(stm.elems()); }
    void visit(StmWeakConstraint const &stm) { visit(stm.body()); }
    void visit(StmShow const &stm) { visit(stm.body()); }
    void visit([[maybe_unused]] StmShowNothing const &stm) {}
    void visit(StmShowSig const &stm) {
        depend_->emplace(SharedSig{stm.name(), stm.arity(), stm.sign()}, location(stm));
    }
    void visit(StmProject const &stm) { visit(stm.body()); }
    void visit(StmProjectSig const &stm) {
        depend_->emplace(SharedSig{stm.name(), stm.arity(), stm.sign()}, location(stm));
    }
    void visit(StmDefined const &stm) { provide_->emplace(stm.name(), stm.arity(), stm.sign()); }
    void visit(StmExternal const &stm) {
        add_(stm.atom(), true);
        visit(stm.body());
    }
    void visit(StmEdge const &stm) { visit(stm.body()); }
    void visit(StmHeuristic const &stm) {
        add_(stm.atom(), false);
        visit(stm.body());
    }
    void visit([[maybe_unused]] StmScript const &stm) {}
    void visit([[maybe_unused]] StmInclude const &stm) {}
    void visit([[maybe_unused]] StmProgram const &stm) {}
    void visit([[maybe_unused]] StmConst const &stm) {}
    void visit([[maybe_unused]] StmParts const &stm) {}
    void visit([[maybe_unused]] StmComment const &stm) {}

    template <class... T, class... A> void visit(std::variant<T...> const &expr, A... args) {
        std::visit(*this, expr, std::variant<A>(args)...);
    }

    template <class T, class... A> void visit(Util::immutable_array<T> const &expr, A... args) {
        for (auto const &elem : expr) {
            visit(elem, args...);
        }
    }

    template <class E, class... A> void operator()(E const &expr, A... args) { return visit(expr, args...); }

  private:
    void add_(Term const &atom, bool head) const {
        if (auto osig = signature(atom)) {
            if (head) {
                provide_->emplace(get<0>(*osig), get<1>(*osig), get<2>(*osig));
            } else {
                depend_->try_emplace(SharedSig{get<0>(*osig), get<1>(*osig), get<2>(*osig)}, location(atom));
            }
        }
    }

    SharedSigSet *provide_;
    Util::ordered_map<SharedSig, Location> *depend_;
};

} // namespace

void analyze(Stm const &stm, SharedSigSet &provide, Util::ordered_map<SharedSig, Location> &depend) {
    AnalyzeVisitor{provide, depend}.visit(stm);
}

} // namespace CppClingo::Input
