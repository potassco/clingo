#include <algorithm>

#include <gringo/util/algorithm.hh>
#include <gringo/util/enum.hh>

#include <gringo/input/algo/check_syntax.hh>
#include <gringo/input/algo/print.hh>

namespace Gringo::Input {

namespace {

enum SyntaxCheck : unsigned {
    none = 0,
    project = 1,
    project_tuple = 2,
    is_const = 4,
};

GRINGO_ENUM_FLAGS(SyntaxCheck);

struct CheckSyntax {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &, SyntaxCheck check = SyntaxCheck::none) const -> bool = delete;

    // terms

    auto operator()(Term const &term, SyntaxCheck check = SyntaxCheck::none) const -> bool {
        return std::visit(*this, term, std::variant<SyntaxCheck>{check});
    }

    auto operator()(TermSymbol const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TermVariable const &term, SyntaxCheck check) const -> bool {
        if (test(check, SyntaxCheck::is_const)) {
            GRINGO_REPORT_LOC(log, error, term.loc()) << "variables not permitted in this context";
            return false;
        }
        return true;
    }

    auto operator()(Projection pro, SyntaxCheck check) const -> bool {
        if (!test(check, SyntaxCheck::project)) {
            GRINGO_REPORT_LOC(log, error, pro.loc()) << "projection not permitted in this context";
            return false;
        }
        return true;
    }

    auto operator()(Argument const &elem, SyntaxCheck check) const -> bool {
        return std::visit(*this, elem, std::variant<SyntaxCheck>{check});
    }

    auto operator()(ArgumentTuple const &tuple, SyntaxCheck check) const -> bool {
        return std::all_of(tuple.elems().begin(), tuple.elems().end(),
                           [this, check](auto const &project_or_term) { return operator()(project_or_term, check); });
    }

    auto operator()(TupleElement const &elem, SyntaxCheck check) const -> bool {
        if (!test(check, project_tuple)) {
            check &= ~SyntaxCheck::project;
        }
        return std::visit(*this, elem, std::variant<SyntaxCheck>{check});
    }

    auto operator()(TermTuple const &term, SyntaxCheck check) const -> bool {
        if (test(check, SyntaxCheck::is_const) && term.pool().size() != 1) {
            GRINGO_REPORT_LOC(log, error, term.loc()) << "pools not permitted in this context";
            return false;
        }
        return std::all_of(term.pool().begin(), term.pool().end(),
                           [this, check](auto const &tuple_or_term) { return operator()(tuple_or_term, check); });
    }

    auto operator()(TermFunction const &term, SyntaxCheck check) const -> bool {
        if (test(check, SyntaxCheck::is_const)) {
            if (term.external()) {
                GRINGO_REPORT_LOC(log, error, term.loc()) << "external functions not permitted in this context";
                return false;
            }
            if (term.pool().size() != 1) {
                GRINGO_REPORT_LOC(log, error, term.loc()) << "pools not permitted in this context";
                return false;
            }
        }
        return std::all_of(term.pool().begin(), term.pool().end(),
                           [this, check](auto const &tuple) { return operator()(tuple, check); });
    }

    auto operator()(TermAbs const &term, SyntaxCheck check) const -> bool {
        if (test(check, SyntaxCheck::is_const) && term.pool().size() != 1) {
            GRINGO_REPORT_LOC(log, error, term.loc()) << "pools not permitted in this context";
            return false;
        }
        return std::all_of(term.pool().begin(), term.pool().end(),
                           [this](auto const &term) { return operator()(term); });
    }

    auto operator()(TermUnary const &term, SyntaxCheck check) const -> bool {
        switch (term.op()) {
            case UnaryOperator::invert: {
                check &= ~SyntaxCheck::project_tuple;
                break;
            }
            case UnaryOperator::negate: {
                check &= ~SyntaxCheck::project;
                break;
            }
        }
        return operator()(*term.rhs(), check);
    }

    auto operator()(TermBinary const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(check);
        return operator()(*term.lhs()) && operator()(*term.rhs());
    }

    // theory terms

    auto operator()(TheoryTerm const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermSymbol const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermVariable const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermTuple const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermFunction const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermUnparsed const &term, SyntaxCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    // literals

    auto operator()(Lit const &lit, SyntaxCheck check) const -> bool {
        return std::visit(*this, lit, std::variant<SyntaxCheck>{check});
    }

    auto operator()(LitBool const &lit, SyntaxCheck check) const -> bool {
        static_cast<void>(lit);
        static_cast<void>(check);
        return true;
    }

    auto operator()(LitComparison const &lit, SyntaxCheck check) const -> bool {
        static_cast<void>(check);
        return operator()(lit.lhs()) && std::all_of(lit.rhs().begin(), lit.rhs().end(),
                                                    [this](auto &guard) { return operator()(guard.second); });
    }

    auto operator()(LitSymbolic const &lit, SyntaxCheck check) const -> bool {
        if (lit.sign() != Sign::none) {
            check = SyntaxCheck::project | SyntaxCheck::project_tuple;
        }
        return operator()(lit.term(), check);
    }

    // conditional literal

    auto operator()(LitArray const &lits) const -> bool {
        return std::all_of(lits.begin(), lits.end(), [this](auto const &lit) {
            return operator()(lit, SyntaxCheck::project | SyntaxCheck::project_tuple);
        });
    }

    auto operator()(CondLit const &lit, SyntaxCheck check = SyntaxCheck::project | SyntaxCheck::project_tuple) const
        -> bool {
        return this->operator()(lit.lit(), check) && operator()(lit.cond());
    }

    // aggregate

    auto operator()(LGuard guard) const -> bool { return !guard.has_value() || operator()(guard->first); }

    auto operator()(RGuard guard) const -> bool { return !guard.has_value() || operator()(guard->second); }

    auto operator()(TermArray const &terms) const -> bool {
        return std::all_of(terms.begin(), terms.end(), [this](auto const &term) { return operator()(term); });
    }

    template <bool HasSign> auto operator()(SetAggregate<HasSign> const &lit) const -> bool {
        return std::all_of(lit.elems().begin(), lit.elems().end(),
                           [this](auto const &elem) {
                               return this->operator()(elem.lit(),
                                                       HasSign ? SyntaxCheck::project | SyntaxCheck::project_tuple
                                                               : SyntaxCheck::none) &&
                                      operator()(elem.cond());
                           }) &&
               operator()(lit.lhs()) && operator()(lit.rhs());
    }

    // theory

    template <bool HasSign> auto operator()(TheoryAtom<HasSign> const &atom) const -> bool {
        return operator()(atom.name()) &&
               std::all_of(atom.elems().begin(), atom.elems().end(),
                           [this](auto const &elem) { return this->operator()(elem.cond()); });
    }

    // head literal

    auto operator()(HdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> bool { return operator()(lit.lit(), SyntaxCheck::none); }

    auto operator()(HdLitDisjunction const &lit) const -> bool {
        return std::all_of(lit.elems().begin(), lit.elems().end(), [this](auto const &elem) {
            return std::visit([this](auto const &elem) { return this->operator()(elem, SyntaxCheck::none); }, elem);
        });
    }

    auto operator()(HdLitAggregate const &lit) const -> bool {
        return std::all_of(lit.elems().begin(), lit.elems().end(),
                           [this](auto const &elem) {
                               return operator()(elem.tuple()) && operator()(elem.lit(), SyntaxCheck::none) &&
                                                                  operator()(elem.cond());
                           }) &&
               operator()(lit.lhs()) && operator()(lit.rhs());
    }

    // body literal

    auto operator()(BdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> bool {
        return operator()(lit.lit(), SyntaxCheck::project | SyntaxCheck::project_tuple);
    }

    auto operator()(BdLitConjunction const &lit) const -> bool { return operator()(lit.lit()); }

    auto operator()(BdLitAggregate const &lit) const -> bool {
        return std::all_of(lit.elems().begin(), lit.elems().end(),
                           [this](auto const &elem) { return operator()(elem.tuple()) && operator()(elem.cond()); }) &&
               operator()(lit.lhs()) && operator()(lit.rhs());
    }

    // statement

    auto operator()(std::optional<Term> const &term) const -> bool { return (!term || operator()(*term)); }

    auto operator()(BdLitArray const &lits) const -> bool { return std::all_of(lits.begin(), lits.end(), *this); }

    auto operator()(Stm const &stm) const -> bool { return std::visit(*this, stm); }

    auto operator()(StmRule const &stm) const -> bool {
        return operator()(stm.head()) && std::all_of(stm.body().begin(), stm.body().end(), *this);
    }

    auto operator()(StmTheory const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    auto operator()(OptimizeTuple const &tuple) const -> bool {
        return operator()(tuple.weight()) && operator()(tuple.terms()) && operator()(tuple.prio());
    }

    auto operator()(StmOptimize const &stm) const -> bool {
        return std::all_of(stm.elems().begin(), stm.elems().end(),
                           [this](auto const &elem) { return operator()(elem.first) && operator()(elem.second); });
    }

    auto operator()(StmWeakConstraint const &stm) const -> bool {
        return std::all_of(stm.body().begin(), stm.body().end(), *this) && operator()(stm.tuple());
    }

    auto operator()(StmShow const &stm) const -> bool { return operator()(stm.term()) && operator()(stm.body()); }

    auto operator()(StmShowSig const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    auto operator()(StmProject const &stm) const -> bool { return operator()(stm.term()) && operator()(stm.body()); }

    auto operator()(StmProjectSig const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    auto operator()(StmDefined const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    auto operator()(StmExternal const &stm) const -> bool {
        return operator()(stm.term()) && operator()(stm.body()) && operator()(stm.type());
    }

    auto operator()(Edge const &edge) const -> bool { return operator()(edge.src()) && operator()(edge.dst()); }

    auto operator()(StmEdge const &stm) const -> bool {
        return std::all_of(stm.edges().begin(), stm.edges().end(), *this) && operator()(stm.body());
    }

    auto operator()(StmHeuristic const &stm) const -> bool {
        return operator()(stm.atom()) && operator()(stm.body()) && operator()(stm.type()) && operator()(stm.prio()) &&
                                                                                             operator()(stm.weight());
    }

    auto operator()(StmScript const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    auto operator()(StmInclude const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    auto operator()(StmProgram const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    auto operator()(StmConst const &stm) const -> bool { return operator()(stm.value(), SyntaxCheck::is_const); }

    auto operator()(StmComment const &stm) const -> bool {
        static_cast<void>(stm);
        return true;
    }

    Logger &log;
};

} // namespace

auto check_term(Logger &log, Term const &term) -> bool { return CheckSyntax{log}(term); }

auto check_literal(Logger &log, Lit const &lit) -> bool {
    return CheckSyntax{log}(lit, SyntaxCheck::project | SyntaxCheck::project_tuple);
}

auto check_head_literal(Logger &log, HdLit const &lit) -> bool { return CheckSyntax{log}(lit); }

auto check_body_literal(Logger &log, BdLit const &lit) -> bool { return CheckSyntax{log}(lit); }

auto check_statement(Logger &log, Stm const &stm) -> bool { return CheckSyntax{log}(stm); }

} // namespace Gringo::Input
