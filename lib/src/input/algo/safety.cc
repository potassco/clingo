#include <util/algorithm.hh>

#include <input/algo/analyze.hh>
#include <input/algo/safety.hh>
#include <input/algo/visit_variables.hh>

// TODO: remove

#include <input/algo/print.hh>
#include <iostream>
#include <util/print.hh>

namespace Gringo::Input {

// TODO:
// - check whether statements are safe
// - maybe even order rule bodies
//   - maybe try to stay close to the given rule
//   - maybe give preference to comparisons (or at least assignments)
// - checking:
//   - [(literal, provide, depend)]
//   - pick if depend <= provided
//   - set provided = provide + provided
//   - it only makes sense to add assignments once!
//     - they should be added in both directions to the check list
//     - their "second direction" should not be added to a ordered body

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
    // TODO: in nested contexts should not depend on global variables
    // there should be a vector of already bound variables
    void operator()(Literal const &lit, bool can_provide) { std::visit(*this, lit, std::variant<bool>{can_provide}); }

    void operator()(LiteralBoolean const &lit, bool can_provide) {
        static_cast<void>(lit);
        static_cast<void>(can_provide);
        std::invoke(cb, StringVec{}, StringVec{});
    }

    void operator()(LiteralRelation const &lit, bool can_provide) {
        auto add = [this, &lit](bool lhs, bool rhs) {
            StringVec provide;
            StringVec depend;
            GetDep{provided, provide, depend}(lit.lhs, lhs);
            GetDep{provided, provide, depend}(lit.rhs.front().second, rhs);
            if (!rhs || provide.empty()) {
                std::invoke(cb, std::move(provide), std::move(depend));
            }
        };
        if (lit.rhs.front().first == Relation::equal && can_provide) {
            // Note: might somehow have to indicate direction...
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
        std::invoke(cb, std::move(provide), std::move(depend));
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
        std::invoke(cb, StringVec{}, std::move(depend));
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
        std::invoke(cb, std::move(provide), std::move(depend));
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
        std::invoke(cb, StringVec{}, std::move(depend));
    }

    CB cb;
    VariableSet const &global;
    VariableSet const &provided;
};

template <class Lit> struct Node {
    Node(Literal const &lit, StringVec provide, StringVec depend) {
        static_cast<void>(lit);
        static_cast<void>(provide);
        static_cast<void>(depend);
    }
};
template <class Lit> using NodeVec = std::vector<Node<Lit>>;

struct CheckSafety {
    auto operator()(auto const &stm) -> bool {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(Statement const &stm) -> bool { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) -> bool {
        std::cerr << "dep for " << stm << std::endl;
        for (auto const &lit : stm.body) {
            auto add_node = [&lit](StringVec provide, StringVec depend) {
                std::cerr << "  "
                          << "[" << lit << ", {" << Util::p_range{provide} << "}, {" << Util::p_range{depend} << "}]"
                          << std::endl;
                static_cast<void>(provide);
                static_cast<void>(depend);
            };
            VariableSet global = select_variables(stm, VariableContext::global);
            VariableSet provided;
            MakeNode{add_node, global, provided}(lit);
        }
        return true;
    }

    VariableSet const &global;
};

} // namespace

auto check_safety(Statement const &stm) -> bool {
    VariableSet global;
    return CheckSafety{global}(stm);
}

} // namespace Gringo::Input
