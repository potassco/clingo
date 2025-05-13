#include <clingo/input/print.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/safety.hh>
#include <clingo/input/rewrite/visit_variables.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

namespace CppClingo::Input {

namespace {

//! Get variables a term provides or depends on.
class GetDep {
  public:
    GetDep(const VariableSet &ignore, StringVec &provide, StringVec &depend)
        : ignore_{&ignore}, provide_{&provide}, depend_{&depend} {}

    void operator()(auto const &x, bool can_provide) const = delete;

    void operator()(Term const &term, bool can_provide) const {
        std::visit(*this, term, std::variant<bool>{can_provide});
    }

    void operator()(TermVariable const &term, bool can_provide) const {
        if (!ignore_->contains(term.name())) {
            if (can_provide) {
                provide_->emplace_back(term.name());
            } else {
                depend_->emplace_back(term.name());
            }
        }
    }

    void operator()([[maybe_unused]] TermSymbol const &term, [[maybe_unused]] bool can_provide) const {}

    void operator()(ArgumentTuple const &tuple, bool can_provide) const {
        for (auto const &tuple_elem : tuple.elems()) {
            if (auto const *term = std::get_if<Term>(&tuple_elem); term != nullptr) {
                operator()(*term, can_provide);
            }
        }
    }

    void operator()(TermTuple const &term, bool can_provide) const {
        for (auto const &elem : term.pool()) {
            std::visit(*this, elem, std::variant<bool>{can_provide});
        }
    }

    void operator()(TermFunction const &term, bool can_provide) const {
        for (auto const &elem : term.pool()) {
            operator()(elem, can_provide);
        }
    }

    void operator()(TermAbs const &term, [[maybe_unused]] bool can_provide) const {
        for (auto const &arg : term.pool()) {
            operator()(arg, false);
        }
    }

    void operator()(TermUnary const &term, bool can_provide) const {
        operator()(*term.rhs(), can_provide && term.op() == UnaryOperator::minus);
    }

    void operator()(TermBinary const &term, bool can_provide) const {
        if (auto lin = check_linear(term); can_provide && lin && !ignore_->contains(lin->x())) {
            provide_->emplace_back(lin->x());
        } else {
            operator()(*term.lhs(), false);
            operator()(*term.rhs(), false);
        }
    }

  private:
    VariableSet const *ignore_;
    StringVec *provide_;
    StringVec *depend_;
};

//! Turn literals into nodes that depend or provide variables.
template <class CB> class MakeNode {
  public:
    MakeNode(CB cb, VariableSet const &global, VariableSet const &provided)
        : cb_{std::move(cb)}, global_{&global}, provided_{&provided} {}

    // literals

    void operator()(Lit const &lit, bool can_provide) { std::visit(*this, lit, std::variant<bool>{can_provide}); }

    void operator()([[maybe_unused]] LitBool const &lit, [[maybe_unused]] bool can_provide) {
        std::invoke(cb_, StringVec{}, StringVec{}, false);
    }

    void operator()(LitComparison const &lit, bool can_provide) {
        auto add = [this, &lit](bool lhs, bool rhs) {
            StringVec provide;
            StringVec depend;
            GetDep{*provided_, provide, depend}(lit.lhs(), lhs);
            GetDep{*provided_, provide, depend}(lit.rhs().front().second, rhs);
            if (!rhs || !provide.empty()) {
                std::invoke(cb_, std::move(provide), std::move(depend), rhs);
            }
        };
        if (lit.rhs().front().first == Relation::equal && can_provide) {
            add(true, false);
            add(false, true);
        } else {
            add(false, false);
        }
    }

    void operator()(LitSymbolic const &lit, bool can_provide) {
        StringVec provide;
        StringVec depend;
        GetDep{*provided_, provide, depend}(lit.term(), can_provide && lit.sign() == Sign::none);
        std::invoke(cb_, std::move(provide), std::move(depend), false);
    }

    // body literals

    void operator()(BdLit const &lit, bool can_provide) { std::visit(*this, lit, std::variant<bool>{can_provide}); }

    void operator()(BdLitSimple const &lit, bool can_provide) { operator()(lit.lit(), can_provide); }

    void operator()(BdLitConjunction const &lit, [[maybe_unused]] bool can_provide) {
        VariableVec depend;
        visit_variables(
            lit,
            [this, &depend]([[maybe_unused]] Location const &loc, auto const &var) {
                if (global_->contains(var)) {
                    depend.emplace_back(var);
                }
            },
            VariableContext::all);
        std::invoke(cb_, StringVec{}, std::move(depend), false);
    }

    void operator()(BdLitAggregate const &lit, bool can_provide) {
        auto const &lhs = lit.lhs();
        auto const &rhs = lit.rhs();
        VariableVec provide;
        VariableVec depend;
        can_provide = can_provide && lit.sign() == Sign::none && !rhs && lhs && lhs->second == Relation::equal;
        if (lhs) {
            GetDep{*provided_, provide, depend}(lhs->first, can_provide);
        }
        if (rhs) {
            GetDep{*provided_, provide, depend}(rhs->second, false);
        }
        for (auto const &elem : lit.elems()) {
            visit_variables(elem, [this, &depend]([[maybe_unused]] Location const &loc, auto const &var) {
                if (global_->contains(var)) {
                    depend.emplace_back(var);
                }
            });
        }
        std::invoke(cb_, std::move(provide), std::move(depend), false);
    }

    void operator()([[maybe_unused]] BdLitSetAggregate const &lit, [[maybe_unused]] bool can_provide) {
        throw std::runtime_error("unpool must be called before safety checking");
    }

    void operator()(BdLitTheoryAtom const &lit, [[maybe_unused]] bool can_provide) {
        VariableVec depend;
        visit_variables(
            lit,
            [this, &depend]([[maybe_unused]] Location const &loc, auto const &var) {
                if (global_->contains(var)) {
                    depend.emplace_back(var);
                }
            },
            VariableContext::all);
        std::invoke(cb_, StringVec{}, std::move(depend), false);
    }

  private:
    CB cb_;
    VariableSet const *global_;
    VariableSet const *provided_;
};

template <class Lit> struct Node {
    Node(Lit const &lit, size_t done, StringVec provide, StringVec depend, bool swap)
        : lit{&lit}, done{done}, provide{std::move(provide)}, depend{std::move(depend)}, swap{swap} {}
    Lit const *lit;
    size_t done;
    StringVec provide;
    StringVec depend;
    bool swap;
};
template <class Lit> using NodeVec = std::vector<Node<Lit>>;

[[nodiscard]] auto flip(Lit const &lit) -> Lit {
    auto const &rel = std::get<LitComparison>(lit);
    auto const &[sym, rhs] = rel.rhs().front();
    assert(sym == Relation::equal && rel.rhs().size() == 1);
    return LitComparison{rel.loc(), rel.sign(), rhs, Util::make_vec<Guard>(Guard{sym, rel.lhs()})};
}

[[nodiscard]] auto flip(BdLit const &lit) -> BdLit {
    return flip(std::get<BdLitSimple>(lit).lit());
}

[[nodiscard]] auto is_provided(VariableSet const &provided, auto const &vars) {
    return std::ranges::all_of(vars,
                               [&provided](auto const &var) { return var.starts_with("$") || provided.contains(var); });
}

template <class Span> using PrepareResult = std::pair<decltype(Util::ResultVec{std::declval<Span>()}), VariableSet>;

template <class Span>
[[nodiscard]] auto prepare_lits(Logger &log, Span const &lits, VariableSet const &global, VariableSet const &bound,
                                VariableSet const &extra = VariableSet{}) -> PrepareResult<Span> {
    auto res = PrepareResult<Span>{lits, VariableSet{}};

    auto &[res_body, provided] = res;
    auto nodes = NodeVec<typename std::decay_t<decltype(lits)>::value_type>{};
    auto done = std::vector<bool>{};

    for (auto const &var : extra) {
        provided.insert(var);
    }

    nodes.reserve(2 * lits.size());
    done.resize(lits.size(), false);
    size_t index = 0;
    GRINGO_REPORT(log, trace) << "literal dependencies";
    for (auto const &lit : lits) {
        auto add_node = [&log, &lit, &nodes, &index](StringVec provide, StringVec depend, bool swap) {
            GRINGO_REPORT(log, trace) << "  " << lit << ", {" << Util::p_range(provide) << "}, {"
                                      << Util::p_range(depend) << "}";
            nodes.emplace_back(lit, index, std::move(provide), std::move(depend), swap);
        };
        MakeNode{add_node, global, bound}(lit, true);
        ++index;
    }

    GRINGO_REPORT(log, trace) << "literal order";
    for (auto it = nodes.begin(); it != nodes.end();) {
        auto jt = std::stable_partition(nodes.begin(), nodes.end(),
                                        [&provided](auto const &node) { return is_provided(provided, node.depend); });
        if (jt == it) {
            break;
        }
        for (; it != jt; ++it) {
            if (!done[it->done]) {
                GRINGO_REPORT(log, trace) << "  " << *it->lit;
                done[it->done] = true;
                provided.insert(it->provide.begin(), it->provide.end());
                if (&res_body.current() == it->lit && !it->swap) {
                    res_body.keep();
                } else {
                    res_body.replace(it->swap ? flip(*it->lit) : *it->lit);
                }
            }
        }
    }

    return res;
}

void vv_(auto const &x, VarVisitFun fun) {
    visit_variables(x, std::move(fun));
}

template <class T> void vv_(Util::immutable_array<T> const &vec, VarVisitFun const &fun) {
    for (auto const &term : vec) {
        vv_(term, fun);
    }
}

auto check_provided(VariableSet const &bound, VariableSet const &provided, auto &&...args) -> bool {
    VariableVec depend;
    (vv_(args,
         [&bound, &depend]([[maybe_unused]] Location const &loc, auto const &var) {
             if (!bound.contains(var)) {
                 depend.emplace_back(var);
             }
         }),
     ...);
    return is_provided(provided, depend);
}

void report(Logger &log, VariableSet const &vars, VariableSet const &bound, auto const &x) {
    VariableVec unsafe;
    unsafe.reserve(vars.size() - bound.size());
    std::copy_if(vars.begin(), vars.end(), std::back_inserter(unsafe),
                 [&bound](auto const &var) { return !bound.contains(var); });
    std::ranges::sort(unsafe);
    unsafe.erase(std::ranges::unique(unsafe).begin(), unsafe.end());
    GRINGO_REPORT_LOC(log, error, location(x)) << "unsafe variables in:\n"
                                               << "  " << x << "\n"
                                               << "note: the following variables are unsafe:\n"
                                               << "  " << Util::p_range(unsafe, ", ");
}

auto report_local(Logger &log, VariableSet const &global, VariableSet const &bound, auto const &x) {
    auto local = select_variables(x);
    for (auto const &var : global) {
        local.erase(var);
    }
    report(log, local, bound, x);
}

//! Check safety of local variables.
class CheckLocal {
  public:
    CheckLocal(Logger &log, const VariableSet &bound) : log_{&log}, bound_{&bound} {}

    [[nodiscard]] auto handle_cond(auto const &elem, auto fun, auto... attr) const {
        auto [res_cond, provided] = prepare_lits(*log_, elem.cond(), VariableSet{}, *bound_);
        if (!res_cond.complete() ||
            !check_provided(*bound_, provided, elem.template get_value<decltype(attr)::tag>()...)) {
            report_local(*log_, *bound_, provided, elem);
            return fun(false, Util::ResultVec{elem.cond()});
        }
        return fun(true, std::move(res_cond));
    }

    [[nodiscard]] auto handle_elem(auto const &elem, auto &res_elems, auto... attr) const -> bool {
        return handle_cond(
            elem,
            [&](bool res, auto res_cond) {
                res_elems.update(elem.rewrite(a_cond = std::move(res_cond)));
                return res;
            },
            std::move(attr)...);
    }

    template <class T> [[nodiscard]] auto handle_elems(auto const &lit, auto... attr) const -> Util::ResultState<T> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            if (!handle_elem(elem, res_elems, attr...)) {
                return {false};
            }
        }
        return {true, lit.rewrite(a_elems = std::move(res_elems))};
    }

    auto operator()(HdLit const &lit) const -> Util::ResultState<HdLit> { return std::visit(*this, lit); }

    auto operator()([[maybe_unused]] HdLitSimple const &lit) const -> Util::ResultState<HdLit> { return {true}; }

    auto operator()(HdLitDisjunction const &lit) const -> Util::ResultState<HdLit> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            if (auto const *clit = std::get_if<CondLit>(&elem); clit != nullptr) {
                if (!handle_elem(*clit, res_elems, a_lit)) {
                    return {false};
                }
            } else {
                res_elems.keep();
            }
        }
        return {true, lit.rewrite(a_elems = std::move(res_elems))};
    }

    auto operator()(HdLitAggregate const &lit) const -> Util::ResultState<HdLit> {
        return handle_elems<HdLit>(lit, a_tuple, a_lit);
    }

    auto operator()(BdLit const &lit) const -> Util::ResultState<BdLit> { return std::visit(*this, lit); }

    auto operator()([[maybe_unused]] BdLitSimple const &lit) const -> Util::ResultState<BdLit> { return {true}; }

    auto operator()(BdLitConjunction const &lit) const -> Util::ResultState<BdLit> {
        return handle_cond(
            lit.lit(),
            [&lit](bool res, auto res_cond) -> Util::ResultState<BdLit> {
                return {res, lit.lit().rewrite(a_cond = std::move(res_cond))};
            },
            a_lit);
    }

    auto operator()(BdLitAggregate const &lit) const -> Util::ResultState<BdLit> {
        return handle_elems<BdLit>(lit, a_tuple);
    }

    template <bool sign>
    auto operator()([[maybe_unused]] SetAggregate<sign> const &lit) const
        -> Util::ResultState<std::conditional_t<sign, BdLit, HdLit>> {
        throw std::runtime_error("unpool must be called before checking safety");
    }

    template <bool sign>
    auto operator()(TheoryAtom<sign> const &lit) const -> Util::ResultState<std::conditional_t<sign, BdLit, HdLit>> {
        return handle_elems<std::conditional_t<sign, BdLit, HdLit>>(lit, a_tuple);
    }

  private:
    Logger *log_;
    VariableSet const *bound_;
};

class CheckGlobal {
  public:
    CheckGlobal(Logger &log, const VariableSet &global) : log_{&log}, global_{&global} {}

    template <class S, class F>
    auto handle_body(S const &stm, F build, Term const *atom = nullptr) const -> Util::ResultState<Stm> {
        // check body
        VariableSet extra;
        if (atom != nullptr) {
            extra = select_variables(*atom);
        }
        auto [res_body, provided] = prepare_lits(*log_, stm.body(), *global_, VariableSet{}, extra);
        if (!res_body.complete() || !is_provided(provided, *global_)) {
            report(*log_, *global_, provided, stm);
            return {false};
        }

        // check nested body
        auto res_body_nested = Util::ResultVec{res_body.value()};
        for (auto const &lit : res_body.value()) {
            auto [res_state, res_lit] = CheckLocal{*log_, provided}(lit);
            if (!res_state) {
                return {false};
            }
            res_body_nested.update(std::move(res_lit));
        }
        if (res_body_nested) {
            res_body.as_optional() = std::move(res_body_nested).as_optional();
        }
        return build(provided, std::move(res_body));
    }

    template <class S> auto handle_body(S const &stm, Term const *atom = nullptr) const -> Util::ResultState<Stm> {
        return handle_body(
            stm,
            [&stm]([[maybe_unused]] auto &provided, auto res_body) -> Util::ResultState<Stm> {
                return {true, stm.rewrite(a_body = std::move(res_body))};
            },
            atom);
    }

    auto operator()(Stm const &stm) const -> Util::ResultState<Stm> { return std::visit(*this, stm); }

    auto operator()(StmRule const &stm) const -> Util::ResultState<Stm> {
        return handle_body(stm, [this, &stm](auto &provided, auto res_body) -> Util::ResultState<Stm> {
            // check nested head
            auto [state_head, res_head] = CheckLocal{*log_, provided}(stm.head());
            if (!state_head) {
                return {false};
            }

            return {true, stm.rewrite(a_head = std::move(res_head), a_body = std::move(res_body))};
        });
    }

    auto operator()([[maybe_unused]] StmOptimize const &stm) const -> Util::ResultState<Stm> {
        throw std::runtime_error("unpool must be called before safety checking");
    }

    template <class T> auto operator()(T const &stm) const -> Util::ResultState<Stm> {
        if constexpr (Util::is_among_v<T, StmWeakConstraint, StmShow, StmExternal, StmEdge>) {
            return handle_body(stm);
        } else if constexpr (Util::is_among_v<T, StmProject, StmHeuristic>) {
            return handle_body(stm, &stm.atom());
        } else {
            static_assert(Util::is_among_v<T, StmTheory, StmShowNothing, StmShowSig, StmProjectSig, StmDefined,
                                           StmScript, StmInclude, StmProgram, StmConst, StmParts, StmComment>);
            return {true};
        }
    }

  private:
    Logger *log_;
    VariableSet const *global_;
};

} // namespace

auto check_safety(Logger &log, Stm const &stm) -> Util::ResultState<Stm> {
    VariableSet global = select_variables(stm, VariableContext::global);
    return CheckGlobal{log, global}(stm);
}

} // namespace CppClingo::Input
