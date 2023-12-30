#include <util/algorithm.hh>

#include <input/algo/analyze.hh>
#include <input/algo/safety.hh>

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
        if (can_provide) {
            provide.emplace_back(term.name);
        } else {
            depend.emplace_back(term.name);
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

    StringVec &provide;
    StringVec &depend;
};

template <class Lit> struct Node {
    Node(Literal const &lit, StringVec provide, StringVec depend) {
        static_cast<void>(lit);
        static_cast<void>(provide);
        static_cast<void>(depend);
    }
};

template <class Lit> struct MakeNode {
    auto operator()(Literal const &lit, bool can_provide) -> Node<Lit> {
        return std::visit(*this, lit, std::variant<bool>{can_provide});
    }

    auto operator()(LiteralBoolean const &lit, bool can_provide) -> Node<Lit> {
        static_cast<void>(can_provide);
        return {lit, {}, {}};
    }

    auto operator()(LiteralRelation const &lit, bool can_provide) -> Node<Lit> {
        static_cast<void>(can_provide);
        if (lit.rhs.front().first == Relation::equal) {
            StringVec provide;
            StringVec depend;
            GetDep{provide, depend}(lit.lhs, can_provide && lit.sign == Sign::none);
            // we need two nodes
        }
        throw std::logic_error("fix relation");
        // return {lit, {}, {}};
    }

    auto operator()(LiteralSymbolic const &lit, bool can_provide) -> Node<Lit> {
        StringVec provide;
        StringVec depend;
        GetDep{provide, depend}(lit.term, can_provide && lit.sign == Sign::none);
        return {lit, std::move(provide), std::move(depend)};
    }
};

} // namespace

} // namespace Gringo::Input
