#include <gringo/util/algorithm.hh>
#include <gringo/util/print.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/safety.hh>
#include <gringo/input/algo/visit_variables.hh>

namespace Gringo::Input {

namespace {

//! Get variables a term provides or depends on.
struct GetDep {

    void operator()(auto const &x, bool can_provide) const = delete;

    void operator()(Term const &term, bool can_provide) const {
        std::visit(*this, term, std::variant<bool>{can_provide});
    }

    void operator()(TermVariable const &term, bool can_provide) const {
        if (!ignore.contains(term.name())) {
            if (can_provide) {
                provide.emplace_back(term.name());
            } else {
                depend.emplace_back(term.name());
            }
        }
    }

    void operator()(TermSymbol const &term, bool can_provide) const {
        static_cast<void>(term);
        static_cast<void>(can_provide);
    }

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

    void operator()(TermAbs const &term, bool can_provide) const {
        static_cast<void>(can_provide);
        for (auto const &arg : term.pool()) {
            operator()(arg, false);
        }
    }

    void operator()(TermUnary const &term, bool can_provide) const {
        operator()(*term.rhs(), can_provide && term.op() == UnaryOperator::negate);
    }

    void operator()(TermBinary const &term, bool can_provide) const {
        if (auto var = is_linear(term); can_provide && var && !ignore.contains(*var)) {
            provide.emplace_back(*var);
        } else {
            operator()(*term.lhs(), false);
            operator()(*term.rhs(), false);
        }
    }

    VariableSet const &ignore;
    StringVec &provide;
    StringVec &depend;
};

//! Turn literals into nodes that depend or provide variables.
template <class CB> struct MakeNode {
    MakeNode(CB cb, VariableSet const &global, VariableSet const &provided)
        : cb{std::move(cb)}, global{global}, provided{provided} {}

    // literals

    void operator()(Lit const &lit, bool can_provide) { std::visit(*this, lit, std::variant<bool>{can_provide}); }

    void operator()(LitBool const &lit, bool can_provide) {
        static_cast<void>(lit);
        static_cast<void>(can_provide);
        std::invoke(cb, StringVec{}, StringVec{}, false);
    }

    void operator()(LitComparison const &lit, bool can_provide) {
        auto add = [this, &lit](bool lhs, bool rhs) {
            StringVec provide;
            StringVec depend;
            GetDep{provided, provide, depend}(lit.lhs(), lhs);
            GetDep{provided, provide, depend}(lit.rhs().front().second, rhs);
            if (!rhs || !provide.empty()) {
                std::invoke(cb, std::move(provide), std::move(depend), rhs);
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
        GetDep{provided, provide, depend}(lit.term(), can_provide && lit.sign() == Sign::none);
        std::invoke(cb, std::move(provide), std::move(depend), false);
    }

    // body literals

    void operator()(BdLit const &lit, bool can_provide) { std::visit(*this, lit, std::variant<bool>{can_provide}); }

    void operator()(BdLitSimple const &lit, bool can_provide) { operator()(lit.lit(), can_provide); }

    void operator()(BdLitConjunction const &lit, bool can_provide) {
        static_cast<void>(can_provide);
        VariableVec depend;
        visit_variables(
            lit,
            [this, &depend](Location const &loc, auto const &var) {
                static_cast<void>(loc);
                if (global.contains(var)) {
                    depend.emplace_back(var);
                }
            },
            VariableContext::all);
        std::invoke(cb, StringVec{}, std::move(depend), false);
    }

    void operator()(BdLitAggregate const &lit, bool can_provide) {
        VariableVec provide;
        VariableVec depend;
        // TODO: aggregate has to be brought into this form in unpool_relations
        can_provide =
            can_provide && lit.sign() == Sign::none && !lit.rhs() && lit.lhs() && lit.lhs()->second == Relation::equal;
        if (lit.lhs()) {
            GetDep{provided, provide, depend}(lit.lhs()->first, can_provide);
        }
        if (lit.rhs()) {
            GetDep{provided, provide, depend}(lit.rhs()->second, false);
        }
        for (auto const &elem : lit.elems()) {
            visit_variables(elem, [this, &depend](Location const &loc, auto const &var) {
                static_cast<void>(loc);
                if (global.contains(var)) {
                    depend.emplace_back(var);
                }
            });
        }
        std::invoke(cb, std::move(provide), std::move(depend), false);
    }

    void operator()(BdLitSetAggregate const &lit, bool can_provide) {
        static_cast<void>(lit);
        static_cast<void>(can_provide);
        throw std::runtime_error("unpool must be called before safety checking");
    }

    void operator()(BdLitTheoryAtom const &lit, bool can_provide) {
        static_cast<void>(can_provide);
        VariableVec depend;
        visit_variables(
            lit,
            [this, &depend](Location const &loc, auto const &var) {
                static_cast<void>(loc);
                if (global.contains(var)) {
                    depend.emplace_back(var);
                }
            },
            VariableContext::all);
        std::invoke(cb, StringVec{}, std::move(depend), false);
    }

    CB cb;
    VariableSet const &global;
    VariableSet const &provided;
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

[[nodiscard]] auto flip(BdLit const &lit) -> BdLit { return flip(std::get<BdLitSimple>(lit).lit()); }

[[nodiscard]] auto is_provided(VariableSet const &provided, auto const &vars) {
    return std::all_of(vars.begin(), vars.end(),
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
            GRINGO_REPORT(log, trace) << "  " << lit << ", {" << Util::p_range{provide} << "}, {"
                                      << Util::p_range{depend} << "}";
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

void vv_(auto const &x, VarVisitFun fun) { visit_variables(x, std::move(fun)); }

template <class T> void vv_(Util::immutable_array<T> const &vec, VarVisitFun fun) {
    for (auto const &term : vec) {
        vv_(term, fun);
    }
}

auto check_provided(VariableSet const &bound, VariableSet const &provided, auto &&...args) -> bool {
    VariableVec depend;
    (vv_(args,
         [&bound, &depend](Location const &loc, auto const &var) {
             static_cast<void>(loc);
             if (!bound.contains(var)) {
                 depend.emplace_back(var);
             }
         }),
     ...);
    return is_provided(provided, depend);
}

auto report(Logger &log, VariableSet const &vars, VariableSet const &bound, auto const &x) {
    VariableVec unsafe;
    unsafe.reserve(vars.size() - bound.size());
    std::copy_if(vars.begin(), vars.end(), std::back_inserter(unsafe),
                 [&bound](auto const &var) { return !bound.contains(var); });
    std::sort(unsafe.begin(), unsafe.end());
    unsafe.erase(std::unique(unsafe.begin(), unsafe.end()), unsafe.end());
    GRINGO_REPORT_LOC(log, error, location(x)) << "unsafe variables in:\n"
                                               << "  " << x << "\n"
                                               << "note: the following variables are unsafe:\n"
                                               << "  " << Util::p_range{unsafe, ", "};
}

auto report_local(Logger &log, VariableSet const &global, VariableSet const &bound, auto const &x) {
    auto local = select_variables(x);
    for (auto const &var : global) {
        local.erase(var);
    }
    report(log, local, bound, x);
}

//! Check safety of local variables.
struct CheckLocal {
    auto operator()(TheoryElementArray const &elems) {
        auto res_elems = Util::ResultVec{elems};
        for (auto const &elem : elems) {
            auto [res_cond, provided] = prepare_lits(log, elem.cond(), VariableSet{}, bound);
            if (!res_cond.complete() || !check_provided(bound, provided, elem.tuple())) {
                report_local(log, bound, provided, elem);
                break;
            }
            if (res_cond) {
                res_elems.replace(elem.loc(), elem.tuple(), res_cond.value());
            } else {
                res_elems.keep();
            }
        }
        return res_elems;
    }

    auto operator()(HdLit const &hlit) -> Util::ResultState<HdLit> { return std::visit(*this, hlit); }

    auto operator()(HdLitSimple const &hlit) -> Util::ResultState<HdLit> {
        static_cast<void>(hlit);
        return {true};
    }

    auto operator()(HdLitDisjunction const &hlit) -> Util::ResultState<HdLit> {
        auto res_elems = Util::ResultVec{hlit.elems()};
        for (auto const &elem : hlit.elems()) {
            if (auto const *clit = std::get_if<CondLit>(&elem); clit != nullptr) {
                auto [res_cond, provided] = prepare_lits(log, clit->cond(), VariableSet{}, bound);
                if (!res_cond.complete() || !check_provided(bound, provided, clit->lit())) {
                    report_local(log, bound, provided, *clit);
                    return {false};
                }
                if (res_cond) {
                    res_elems.replace(CondLit{clit->loc(), clit->lit(), res_cond.value()});
                } else {
                    res_elems.keep();
                }
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true, HdLitDisjunction{hlit.loc(), std::move(res_elems).value()}};
        }
        return {true};
    }

    auto operator()(HdLitAggregate const &hlit) -> Util::ResultState<HdLit> {
        auto res_elems = Util::ResultVec{hlit.elems()};
        for (auto const &elem : hlit.elems()) {
            auto [res_cond, provided] = prepare_lits(log, elem.cond(), VariableSet{}, bound);
            if (!res_cond.complete() || !check_provided(bound, provided, elem.tuple(), elem.lit())) {
                report_local(log, bound, provided, elem);
                return {false};
            }
            if (res_cond) {
                res_elems.replace(elem.loc(), elem.tuple(), elem.lit(), res_cond.value());
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true, HdLitAggregate{hlit.loc(), hlit.lhs(), hlit.fun(), std::move(res_elems).value(), hlit.rhs()}};
        }
        return {true};
    }

    auto operator()(HdLitSetAggregate const &hlit) -> Util::ResultState<HdLit> {
        static_cast<void>(hlit);
        throw std::runtime_error("unpool must be called before checking safety");
    }

    auto operator()(HdLitTheoryAtom const &hlit) -> Util::ResultState<HdLit> {
        auto res_elems = operator()(hlit.elems());
        if (!res_elems.complete()) {
            return {false};
        }
        if (res_elems) {
            return {true, HdLitTheoryAtom{hlit.loc(), hlit.name(), std::move(res_elems).value(), hlit.rhs()}};
        }
        return {true};
    }

    auto operator()(BdLit const &blit) -> Util::ResultState<BdLit> { return std::visit(*this, blit); }

    auto operator()(BdLitSimple const &blit) -> Util::ResultState<BdLit> {
        static_cast<void>(blit);
        return {true};
    }

    auto operator()(BdLitConjunction const &blit) -> Util::ResultState<BdLit> {
        auto [res_cond, provided] = prepare_lits(log, blit.lit().cond(), VariableSet{}, bound);
        if (!res_cond.complete() || !check_provided(bound, provided, blit.lit().lit())) {
            report_local(log, bound, provided, blit.lit());
            return {false};
        }

        if (res_cond) {
            return {true, BdLitConjunction{CondLit{blit.lit().loc(), blit.lit().lit(), std::move(res_cond).value()}}};
        }
        return {true};
    }

    auto operator()(BdLitAggregate const &blit) -> Util::ResultState<BdLit> {
        auto res_elems = Util::ResultVec{blit.elems()};
        for (auto const &elem : blit.elems()) {
            auto [res_cond, provided] = prepare_lits(log, elem.cond(), VariableSet{}, bound);
            if (!res_cond.complete() || !check_provided(bound, provided, elem.tuple())) {
                report_local(log, bound, provided, elem);
                return {false};
            }
            if (res_cond) {
                res_elems.replace(elem.loc(), elem.tuple(), res_cond.value());
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true, BdLitAggregate{blit.loc(), blit.sign(), blit.lhs(), blit.fun(), std::move(res_elems).value(),
                                         blit.rhs()}};
        }
        return {true};
    }

    auto operator()(BdLitSetAggregate const &blit) -> Util::ResultState<BdLit> {
        static_cast<void>(blit);
        throw std::runtime_error("unpool must be called before checking safety");
    }

    auto operator()(BdLitTheoryAtom const &blit) -> Util::ResultState<BdLit> {
        auto res_elems = operator()(blit.elems());
        if (!res_elems.complete()) {
            return {false};
        }
        if (res_elems) {
            return {true,
                    BdLitTheoryAtom{blit.loc(), blit.sign(), blit.name(), std::move(res_elems).value(), blit.rhs()}};
        }
        return {true};
    }

    Logger &log;
    VariableSet const &bound;
};

struct CheckGlobal {
    template <bool pass_intermediate = false, class F>
    auto check_body(auto const &stm, F build, Term const *atom = nullptr) -> Util::ResultState<Stm> {
        // check body
        VariableSet extra;
        if (atom != nullptr) {
            extra = select_variables(*atom);
        }
        auto [res_body, provided] = prepare_lits(log, stm.body(), global, VariableSet{}, extra);
        if (!res_body.complete() || !is_provided(provided, global)) {
            report(log, global, provided, stm);
            return {false};
        }

        // check nested body
        auto res_body_nested = Util::ResultVec{res_body.value()};
        for (auto const &lit : res_body.value()) {
            auto [res_state, res_lit] = CheckLocal{log, provided}(lit);
            if (!res_state) {
                return {false};
            }
            res_body_nested.update(std::move(res_lit));
        }
        if (res_body_nested) {
            res_body.as_optional() = std::move(res_body_nested).as_optional();
        }
        if constexpr (pass_intermediate) {
            return build(provided, std::move(res_body));
        } else {
            if (res_body) {
                return {true, build(std::move(res_body).value())};
            }
            return {true};
        }
    }

    auto operator()(Stm const &stm) -> Util::ResultState<Stm> { return std::visit(*this, stm); }

    auto operator()(StmRule const &stm) -> Util::ResultState<Stm> {
        return check_body<true>(stm, [this, &stm](auto &provided, auto res_body) -> Util::ResultState<Stm> {
            // check nested head
            auto [state_head, res_head] = CheckLocal{log, provided}(stm.head());
            if (!state_head) {
                return {false};
            }

            // construct new rule if necessary
            if (res_body || res_head) {
                return {true,
                        StmRule{stm.loc(), std::move(res_head).value_or(stm.head()), std::move(res_body).value()}};
            }
            return {true};
        });
    }

    auto operator()(StmTheory const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmOptimize const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        throw std::runtime_error("unpool must be called before safety checking");
    }

    auto operator()(StmWeakConstraint const &stm) -> Util::ResultState<Stm> {
        return check_body(stm, [&stm](auto body) {
            return StmWeakConstraint{stm.loc(), std::move(body), stm.tuple()};
        });
    }

    auto operator()(StmShow const &stm) -> Util::ResultState<Stm> {
        return check_body(stm, [&stm](auto body) { return StmShow{stm.loc(), stm.term(), std::move(body)}; });
    }

    auto operator()(StmShowSig const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmProject const &stm) -> Util::ResultState<Stm> {
        return check_body(
            stm,
            [&stm](auto body) {
                return StmProject{stm.loc(), stm.term(), std::move(body)};
            },
            &stm.term());
    }

    auto operator()(StmProjectSig const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmDefined const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmExternal const &stm) -> Util::ResultState<Stm> {
        return check_body(stm, [&stm](auto body) {
            return StmExternal{stm.loc(), stm.term(), std::move(body), stm.type()};
        });
    }

    auto operator()(StmEdge const &stm) -> Util::ResultState<Stm> {
        return check_body(stm, [&stm](auto body) { return StmEdge{stm.loc(), stm.edges(), std::move(body)}; });
    }

    auto operator()(StmHeuristic const &stm) -> Util::ResultState<Stm> {
        return check_body(
            stm,
            [&stm](auto body) {
                return StmHeuristic{stm.loc(), stm.atom(), std::move(body), stm.weight(), stm.prio(), stm.type()};
            },
            &stm.atom());
    }

    auto operator()(StmScript const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmInclude const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmProgram const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmConst const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StmComment const &stm) -> Util::ResultState<Stm> {
        static_cast<void>(stm);
        return {true};
    }

    Logger &log;
    VariableSet const &global;
};

} // namespace

auto check_safety(Logger &log, Stm const &stm) -> Util::ResultState<Stm> {
    VariableSet global = select_variables(stm, VariableContext::global);
    return CheckGlobal{log, global}(stm);
}

} // namespace Gringo::Input
