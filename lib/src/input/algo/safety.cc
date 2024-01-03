#include <util/algorithm.hh>

#include <input/algo/analyze.hh>
#include <input/algo/safety.hh>
#include <input/algo/visit_variables.hh>

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

template <class CB> struct MakeNode {
    MakeNode(CB cb, VariableSet const &global, VariableSet const &provided)
        : cb{std::move(cb)}, global{global}, provided{provided} {}

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

    // BodyTheoryAtom

    void operator()(BodyLiteral const &lit) { std::visit(*this, lit); }

    void operator()(SimpleBodyLiteral const &lit) { operator()(lit.lit, true); }

    void operator()(Conjunction const &lit) {
        VariableVec depend;
        visit_variables(
            lit,
            [this, &depend](Location const &loc, auto const &var) {
                static_cast<void>(loc);
                if (global.contains(var)) {
                    depend.emplace_back(var);
                }
            },
            VariableContext::global);
        std::invoke(cb, StringVec{}, std::move(depend), false);
    }

    void operator()(BodyAggregate const &lit) {
        VariableVec provide;
        VariableVec depend;
        // TODO: aggregate has to be brought into this form in unpool_relations
        bool can_provide = lit.sign == Sign::none && !lit.rhs && lit.lhs && lit.lhs->second == Relation::equal;
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

    void operator()(BodySetAggregate const &lit) {
        static_cast<void>(lit);
        throw std::runtime_error("unpool must be called before safety checking");
    }

    void operator()(BodyTheoryAtom const &lit) {
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

struct CheckSafety {
    auto operator()(auto const &stm) -> Util::ResultState<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(Statement const &stm) -> Util::ResultState<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) -> Util::ResultState<Statement> {
        // TODO: check nested contexts
        NodeVec<BodyLiteral> nodes;
        std::vector<bool> done;
        nodes.reserve(2 * stm.body.size());
        done.resize(stm.body.size(), false);
        size_t index = 0;
        for (auto const &lit : stm.body) {
            auto add_node = [&lit, &nodes, &index](StringVec provide, StringVec depend, bool swap) {
                nodes.emplace_back(lit, index, std::move(provide), std::move(depend), swap);
            };
            VariableSet global = select_variables(stm, VariableContext::global);
            VariableSet provided;
            MakeNode{add_node, global, provided}(lit);
            ++index;
        }
        VariableSet provided;
        auto is_provided = [&provided](auto const &vars) {
            return std::all_of(vars.begin(), vars.end(),
                               [&provided](auto const &var) { return provided.contains(var); });
        };

        auto res_body = Util::ResultVec{stm.body};
        for (auto it = nodes.begin(); it != nodes.end();) {
            auto jt = std::stable_partition(nodes.begin(), nodes.end(),
                                            [&is_provided](auto const &node) { return is_provided(node.depend); });
            if (jt == it) {
                return false;
            }
            for (; it != jt; ++it) {
                if (!done[it->done]) {
                    done[it->done] = true;
                    provided.insert(it->provide.begin(), it->provide.end());
                    if (&res_body.currrent() == it->lit) {
                        res_body.keep();
                    } else if (it->swap) {
                        auto const &rel = std::get<LiteralRelation>(std::get<SimpleBodyLiteral>(*it->lit).lit);
                        auto const &[sym, rhs] = rel.rhs.front();
                        assert(sym == Relation::equal && rel.rhs.size() == 1);
                        res_body.replace(Literal{
                            LiteralRelation{rel.loc, rel.sign, rhs, Util::make_vec<Guard>(Guard{sym, rel.lhs})}});
                    } else {
                        res_body.replace(*it->lit);
                    }
                }
            }
        }

        VariableVec depend;
        visit_variables(
            stm.head,
            [this, &depend](Location const &loc, auto const &var) {
                static_cast<void>(loc);
                if (global.contains(var)) {
                    depend.emplace_back(var);
                }
            },
            VariableContext::all);

        if (!is_provided(depend)) {
            return {false};
        }
        if (res_body) {
            return {true, Rule{stm.loc, stm.head, std::move(res_body).value()}};
        }
        return {true};
    }

    VariableSet const &global;
};

} // namespace

auto check_safety(Statement const &stm) -> Util::ResultState<Statement> {
    VariableSet global = select_variables(stm, VariableContext::global);
    return CheckSafety{global}(stm);
}

} // namespace Gringo::Input
