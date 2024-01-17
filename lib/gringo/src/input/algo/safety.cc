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
        if (!ignore.contains(term.name)) {
            if (can_provide) {
                provide.emplace_back(term.name);
            } else {
                depend.emplace_back(term.name);
            }
        }
    }

    void operator()(TermSymbol const &term, bool can_provide) const {
        static_cast<void>(term);
        static_cast<void>(can_provide);
    }

    void operator()(TupleVec const &tuple, bool can_provide) const {
        for (auto const &tuple_elem : tuple) {
            if (auto const *term = std::get_if<Term>(&tuple_elem); term != nullptr) {
                operator()(*term, can_provide);
            }
        }
    }

    void operator()(TermTuple const &term, bool can_provide) const {
        for (auto const &elem : term.pool) {
            std::visit(*this, elem, std::variant<bool>{can_provide});
        }
    }

    void operator()(TermFunction const &term, bool can_provide) const {
        for (auto const &elem : term.pool) {
            operator()(elem, can_provide);
        }
    }

    void operator()(TermAbs const &term, bool can_provide) const {
        static_cast<void>(can_provide);
        for (auto const &arg : term.pool) {
            operator()(arg, false);
        }
    }

    void operator()(TermUnary const &term, bool can_provide) const {
        operator()(*term.rhs, can_provide && term.op == UnaryOperator::negate);
    }

    void operator()(TermBinary const &term, bool can_provide) const {
        can_provide = can_provide && is_linear(term);
        operator()(*term.lhs, can_provide);
        operator()(*term.rhs, can_provide);
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

    void operator()(Literal const &lit, bool can_provide) { std::visit(*this, lit, std::variant<bool>{can_provide}); }

    void operator()(LiteralBoolean const &lit, bool can_provide) {
        static_cast<void>(lit);
        static_cast<void>(can_provide);
        std::invoke(cb, StringVec{}, StringVec{}, false);
    }

    void operator()(LiteralRelation const &lit, bool can_provide) {
        auto add = [this, &lit](bool lhs, bool rhs) {
            StringVec provide;
            StringVec depend;
            GetDep{provided, provide, depend}(lit.lhs, lhs);
            GetDep{provided, provide, depend}(lit.rhs.front().second, rhs);
            if (!rhs || !provide.empty()) {
                std::invoke(cb, std::move(provide), std::move(depend), rhs);
            }
        };
        if (lit.rhs.front().first == Relation::equal && can_provide) {
            add(true, false);
            add(false, true);
        } else {
            add(false, false);
        }
    }

    void operator()(LiteralSymbolic const &lit, bool can_provide) {
        StringVec provide;
        StringVec depend;
        GetDep{provided, provide, depend}(lit.term, can_provide && lit.sign == Sign::none);
        std::invoke(cb, std::move(provide), std::move(depend), false);
    }

    // body literals

    void operator()(BodyLiteral const &lit, bool can_provide) {
        std::visit(*this, lit, std::variant<bool>{can_provide});
    }

    void operator()(SimpleBodyLiteral const &lit, bool can_provide) { operator()(lit.lit, can_provide); }

    void operator()(Conjunction const &lit, bool can_provide) {
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

    void operator()(BodyAggregate const &lit, bool can_provide) {
        VariableVec provide;
        VariableVec depend;
        // TODO: aggregate has to be brought into this form in unpool_relations
        can_provide =
            can_provide && lit.sign == Sign::none && !lit.rhs && lit.lhs && lit.lhs->second == Relation::equal;
        if (lit.lhs) {
            GetDep{provided, provide, depend}(lit.lhs->first, can_provide);
        }
        if (lit.rhs) {
            GetDep{provided, provide, depend}(lit.rhs->second, false);
        }
        for (auto const &elem : lit.elems) {
            visit_variables(elem, [this, &depend](Location const &loc, auto const &var) {
                static_cast<void>(loc);
                if (global.contains(var)) {
                    depend.emplace_back(var);
                }
            });
        }
        std::invoke(cb, std::move(provide), std::move(depend), false);
    }

    void operator()(BodySetAggregate const &lit, bool can_provide) {
        static_cast<void>(lit);
        static_cast<void>(can_provide);
        throw std::runtime_error("unpool must be called before safety checking");
    }

    void operator()(BodyTheoryAtom const &lit, bool can_provide) {
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

[[nodiscard]] auto flip(Literal const &lit) -> Literal {
    auto const &rel = std::get<LiteralRelation>(lit);
    auto const &[sym, rhs] = rel.rhs.front();
    assert(sym == Relation::equal && rel.rhs.size() == 1);
    return LiteralRelation{rel.loc, rel.sign, rhs, Util::make_vec<Guard>(Guard{sym, rel.lhs})};
}

[[nodiscard]] auto flip(BodyLiteral const &lit) -> BodyLiteral { return flip(std::get<SimpleBodyLiteral>(lit).lit); }

[[nodiscard]] auto is_provided(VariableSet const &provided, auto const &vars) {
    return std::all_of(vars.begin(), vars.end(), [&provided](auto const &var) { return provided.contains(var); });
}

template <class T> using PrepareResult = std::pair<Util::ResultVec<T>, VariableSet>;

template <class Span>
[[nodiscard]] auto prepare_lits(Logger &log, Span const &lits, VariableSet const &global, VariableSet const &bound,
                                VariableSet const &extra = VariableSet{}) -> PrepareResult<typename Span::value_type> {
    auto res = PrepareResult<typename Span::value_type>{lits, VariableSet{}};

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
                if (&res_body.currrent() == it->lit && !it->swap) {
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

template <class T> void vv_(std::vector<T> const &vec, VarVisitFun fun) {
    for (auto const &term : vec) {
        vv_(term, fun);
    }
}

template <class T> void vv_(Util::immutable_vector<T> const &vec, VarVisitFun fun) {
    vv_(vec.vector(), std::move(fun));
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
    auto operator()(TheoryElementVec const &elems) {
        auto res_elems = Util::ResultVec{elems};
        for (auto const &elem : elems) {
            auto [res_cond, provided] = prepare_lits(log, elem.cond, VariableSet{}, bound);
            if (!res_cond.complete() || !check_provided(bound, provided, elem.tuple)) {
                report_local(log, bound, provided, elem);
                break;
            }
            if (res_cond) {
                res_elems.replace(elem.loc, elem.tuple, res_cond.value());
            } else {
                res_elems.keep();
            }
        }
        return res_elems;
    }

    auto operator()(HeadLiteral const &hlit) -> Util::ResultState<HeadLiteral> { return std::visit(*this, hlit); }

    auto operator()(SimpleHeadLiteral const &hlit) -> Util::ResultState<HeadLiteral> {
        static_cast<void>(hlit);
        return {true};
    }

    auto operator()(Disjunction const &hlit) -> Util::ResultState<HeadLiteral> {
        auto res_elems = Util::ResultVec{hlit.elems};
        for (auto const &elem : hlit.elems) {
            if (auto const *clit = std::get_if<ConditionalLiteral>(&elem); clit != nullptr) {
                auto [res_cond, provided] = prepare_lits(log, clit->cond, VariableSet{}, bound);
                if (!res_cond.complete() || !check_provided(bound, provided, clit->lit)) {
                    report_local(log, bound, provided, *clit);
                    return {false};
                }
                if (res_cond) {
                    res_elems.replace(ConditionalLiteral{clit->loc, clit->lit, res_cond.value()});
                } else {
                    res_elems.keep();
                }
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true, Disjunction{hlit.loc, std::move(res_elems).value()}};
        }
        return {true};
    }

    auto operator()(HeadAggregate const &hlit) -> Util::ResultState<HeadLiteral> {
        auto res_elems = Util::ResultVec{hlit.elems};
        for (auto const &elem : hlit.elems) {
            auto [res_cond, provided] = prepare_lits(log, elem.cond, VariableSet{}, bound);
            if (!res_cond.complete() || !check_provided(bound, provided, elem.tuple, elem.lit)) {
                report_local(log, bound, provided, elem);
                return {false};
            }
            if (res_cond) {
                res_elems.replace(elem.loc, elem.tuple, elem.lit, res_cond.value());
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true, HeadAggregate{hlit.loc, hlit.lhs, hlit.fun, std::move(res_elems).value(), hlit.rhs}};
        }
        return {true};
    }

    auto operator()(HeadSetAggregate const &hlit) -> Util::ResultState<HeadLiteral> {
        static_cast<void>(hlit);
        throw std::runtime_error("unpool must be called before checking safety");
    }

    auto operator()(HeadTheoryAtom const &hlit) -> Util::ResultState<HeadLiteral> {
        auto res_elems = operator()(hlit.elems);
        if (!res_elems.complete()) {
            return {false};
        }
        if (res_elems) {
            return {true, HeadTheoryAtom{hlit.loc, hlit.name, std::move(res_elems).value(), hlit.rhs}};
        }
        return {true};
    }

    auto operator()(BodyLiteral const &blit) -> Util::ResultState<BodyLiteral> { return std::visit(*this, blit); }

    auto operator()(SimpleBodyLiteral const &blit) -> Util::ResultState<BodyLiteral> {
        static_cast<void>(blit);
        return {true};
    }

    auto operator()(Conjunction const &blit) -> Util::ResultState<BodyLiteral> {
        auto [res_cond, provided] = prepare_lits(log, blit.lit.cond, VariableSet{}, bound);
        if (!res_cond.complete() || !check_provided(bound, provided, blit.lit.lit)) {
            report_local(log, bound, provided, blit.lit);
            return {false};
        }

        if (res_cond) {
            return {true, Conjunction{ConditionalLiteral{blit.lit.loc, blit.lit.lit, std::move(res_cond).value()}}};
        }
        return {true};
    }

    auto operator()(BodyAggregate const &blit) -> Util::ResultState<BodyLiteral> {
        auto res_elems = Util::ResultVec{blit.elems};
        for (auto const &elem : blit.elems) {
            auto [res_cond, provided] = prepare_lits(log, elem.cond, VariableSet{}, bound);
            if (!res_cond.complete() || !check_provided(bound, provided, elem.tuple)) {
                report_local(log, bound, provided, elem);
                return {false};
            }
            if (res_cond) {
                res_elems.replace(elem.loc, elem.tuple, res_cond.value());
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return {true,
                    BodyAggregate{blit.loc, blit.sign, blit.lhs, blit.fun, std::move(res_elems).value(), blit.rhs}};
        }
        return {true};
    }

    auto operator()(BodySetAggregate const &blit) -> Util::ResultState<BodyLiteral> {
        static_cast<void>(blit);
        throw std::runtime_error("unpool must be called before checking safety");
    }

    auto operator()(BodyTheoryAtom const &blit) -> Util::ResultState<BodyLiteral> {
        auto res_elems = operator()(blit.elems);
        if (!res_elems.complete()) {
            return {false};
        }
        if (res_elems) {
            return {true, BodyTheoryAtom{blit.loc, blit.sign, blit.name, std::move(res_elems).value(), blit.rhs}};
        }
        return {true};
    }

    Logger &log;
    VariableSet const &bound;
};

struct CheckGlobal {
    template <bool pass_intermediate = false, class F>
    auto check_body(auto const &stm, F build, Term const *atom = nullptr) -> Util::ResultState<Statement> {
        // check body
        VariableSet extra;
        if (atom != nullptr) {
            extra = select_variables(*atom);
        }
        auto [res_body, provided] = prepare_lits(log, stm.body, global, VariableSet{}, extra);
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

    auto operator()(Statement const &stm) -> Util::ResultState<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) -> Util::ResultState<Statement> {
        return check_body<true>(stm, [this, &stm](auto &provided, auto res_body) -> Util::ResultState<Statement> {
            // check nested head
            auto [state_head, res_head] = CheckLocal{log, provided}(stm.head);
            if (!state_head) {
                return {false};
            }

            // construct new rule if necessary
            if (res_body || res_head) {
                return {true, Rule{stm.loc, std::move(res_head).value_or(stm.head), std::move(res_body).value()}};
            }
            return {true};
        });
    }

    auto operator()(TheoryDefinition const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StatementOptimize const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        throw std::runtime_error("unpool must be called before safety checking");
    }

    auto operator()(StatementWeakConstraint const &stm) -> Util::ResultState<Statement> {
        return check_body(stm, [&stm](auto body) {
            return StatementWeakConstraint{stm.loc, std::move(body), stm.tuple};
        });
    }

    auto operator()(StatementShow const &stm) -> Util::ResultState<Statement> {
        return check_body(stm, [&stm](auto body) { return StatementShow{stm.loc, stm.term, std::move(body)}; });
    }

    auto operator()(StatementShowSig const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StatementProject const &stm) -> Util::ResultState<Statement> {
        return check_body(
            stm,
            [&stm](auto body) {
                return StatementProject{stm.loc, stm.term, std::move(body)};
            },
            &stm.term);
    }

    auto operator()(StatementProjectSig const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StatementDefined const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StatementExternal const &stm) -> Util::ResultState<Statement> {
        return check_body(stm, [&stm](auto body) {
            return StatementExternal{stm.loc, stm.term, std::move(body), stm.type};
        });
    }

    auto operator()(StatementEdge const &stm) -> Util::ResultState<Statement> {
        return check_body(stm, [&stm](auto body) { return StatementEdge{stm.loc, stm.edges, std::move(body)}; });
    }

    auto operator()(StatementHeuristic const &stm) -> Util::ResultState<Statement> {
        return check_body(
            stm,
            [&stm](auto body) {
                return StatementHeuristic{stm.loc, stm.atom, std::move(body), stm.type, stm.prio, stm.mod};
            },
            &stm.atom);
    }

    auto operator()(StatementScript const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StatementInclude const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StatementProgram const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(StatementConst const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    auto operator()(Comment const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        return {true};
    }

    Logger &log;
    VariableSet const &global;
};

} // namespace

auto check_safety(Logger &log, Statement const &stm) -> Util::ResultState<Statement> {
    VariableSet global = select_variables(stm, VariableContext::global);
    return CheckGlobal{log, global}(stm);
}

} // namespace Gringo::Input
