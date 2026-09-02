#include <clingo/input/print.hh>

#include <clingo/input/rewrite/check_syntax.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/enum.hh>
#include <clingo/util/macro.hh>
#include <clingo/util/type_traits.hh>

#include <algorithm>

namespace CppClingo::Input {

namespace {

enum class SyntaxCheck : uint8_t {
    none = 0,
    project = 1,
    project_tuple = 2,
    is_const = 4,
};

CLINGO_ENABLE_BITSET_ENUM(SyntaxCheck);

struct CheckSyntax {
  public:
    CheckSyntax(Logger &log) : log_{&log} {}

    // expressions that can be ignored

    template <class T> auto operator()([[maybe_unused]] T const &x) const -> bool {
        static_assert(Util::is_among_v<T, StmScript, StmInclude, StmProgram, StmComment, StmShowNothing, StmShowSig,
                                       StmTheory, StmDefined, StmParts, StmProjectSig>);
        return true;
    }

    template <class T> auto operator()([[maybe_unused]] T const &x, [[maybe_unused]] SyntaxCheck check) const -> bool {
        static_assert(Util::is_among_v<T, LitBool, TheoryTerm, TermSymbol, FormatFieldLiteral>);
        return true;
    }

    // terms

    auto operator()(Term const &term, SyntaxCheck check = SyntaxCheck::none) const -> bool {
        return std::visit(*this, term, std::variant<SyntaxCheck>{check});
    }

    auto operator()(TermVariable const &term, SyntaxCheck check) const -> bool {
        if (intersects(check, SyntaxCheck::is_const)) {
            CLINGO_REPORT_LOC(*log_, error, term.loc()) << "variables not permitted in this context";
            return false;
        }
        return true;
    }

    auto operator()(FormatFieldExpression const &field, SyntaxCheck check) const -> bool {
        return operator()(*field.lhs(), check);
    }

    auto operator()(TermFormatString const &term, SyntaxCheck check) const -> bool {
        check -= check = SyntaxCheck::project | SyntaxCheck::project_tuple;
        return std::ranges::all_of(term.elems(), [this, check](auto const &elem) {
            return std::visit(*this, elem, std::variant<SyntaxCheck>{check});
        });
    }

    auto operator()(Projection const &pro, SyntaxCheck check) const -> bool {
        if (!intersects(check, SyntaxCheck::project)) {
            CLINGO_REPORT_LOC(*log_, error, pro.loc()) << "projection not permitted in this context";
            return false;
        }
        return true;
    }

    auto operator()(Argument const &elem, SyntaxCheck check) const -> bool {
        return std::visit(*this, elem, std::variant<SyntaxCheck>{check});
    }

    auto operator()(ArgumentTuple const &tuple, SyntaxCheck check) const -> bool {
        return std::ranges::all_of(
            tuple.elems(), [this, check](auto const &project_or_term) { return operator()(project_or_term, check); });
    }

    auto operator()(TupleElement const &elem, SyntaxCheck check) const -> bool {
        if (!intersects(check, SyntaxCheck::project_tuple)) {
            check &= ~SyntaxCheck::project;
        }
        return std::visit(*this, elem, std::variant<SyntaxCheck>{check});
    }

    auto operator()(TermTuple const &term, SyntaxCheck check) const -> bool {
        if (intersects(check, SyntaxCheck::is_const) && term.pool().size() != 1) {
            CLINGO_REPORT_LOC(*log_, error, term.loc()) << "pools not permitted in this context";
            return false;
        }
        return std::ranges::all_of(
            term.pool(), [this, check](auto const &tuple_or_term) { return operator()(tuple_or_term, check); });
    }

    auto operator()(TermFunction const &term, SyntaxCheck check) const -> bool {
        if (intersects(check, SyntaxCheck::is_const)) {
            if (term.external()) {
                CLINGO_REPORT_LOC(*log_, error, term.loc()) << "external functions not permitted in this context";
                return false;
            }
            if (term.pool().size() != 1) {
                CLINGO_REPORT_LOC(*log_, error, term.loc()) << "pools not permitted in this context";
                return false;
            }
        }
        return std::ranges::all_of(term.pool(), [this, check](auto const &tuple) { return operator()(tuple, check); });
    }

    auto operator()(TermAbs const &term, SyntaxCheck check) const -> bool {
        if (intersects(check, SyntaxCheck::is_const) && term.pool().size() != 1) {
            CLINGO_REPORT_LOC(*log_, error, term.loc()) << "pools not permitted in this context";
            return false;
        }
        return std::ranges::all_of(term.pool(), [this](auto const &term) { return operator()(term); });
    }

    auto operator()(TermUnary const &term, SyntaxCheck check) const -> bool {
        switch (term.op()) {
            case UnaryOperator::negate: {
                check &= ~SyntaxCheck::project_tuple;
                break;
            }
            case UnaryOperator::minus: {
                check &= ~SyntaxCheck::project;
                break;
            }
        }
        return operator()(*term.rhs(), check);
    }

    auto operator()(TermBinary const &term, [[maybe_unused]] SyntaxCheck check) const -> bool {
        return operator()(*term.lhs()) && operator()(*term.rhs());
    }

    // theory terms

    // literals

    auto operator()(Lit const &lit, SyntaxCheck check) const -> bool {
        return std::visit(*this, lit, std::variant<SyntaxCheck>{check});
    }

    auto operator()(LitComparison const &lit, [[maybe_unused]] SyntaxCheck check) const -> bool {
        return operator()(lit.lhs()) &&
               std::ranges::all_of(lit.rhs(), [this](auto &guard) { return operator()(guard.second); });
    }

    auto operator()(LitSymbolic const &lit, SyntaxCheck check) const -> bool {
        if (lit.sign() != Sign::none) {
            check = SyntaxCheck::project | SyntaxCheck::project_tuple;
        }
        return operator()(lit.term(), check);
    }

    // conditional literal

    auto operator()(LitArray const &lits) const -> bool {
        return std::ranges::all_of(lits, [this](auto const &lit) {
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
        return std::ranges::all_of(terms, [this](auto const &term) { return operator()(term); });
    }

    template <bool HasSign> auto operator()(SetAggregate<HasSign> const &lit) const -> bool {
        return std::ranges::all_of(lit.elems(),
                                   [this](auto const &elem) {
                                       return this->operator()(elem.lit(), HasSign ? SyntaxCheck::project |
                                                                                         SyntaxCheck::project_tuple
                                                                                   : SyntaxCheck::none) &&
                                              operator()(elem.cond());
                                   }) &&
               operator()(lit.lhs()) && operator()(lit.rhs());
    }

    // theory

    template <bool HasSign> auto operator()(TheoryAtom<HasSign> const &atom) const -> bool {
        return operator()(atom.name()) &&
               std::ranges::all_of(atom.elems(), [this](auto const &elem) { return this->operator()(elem.cond()); });
    }

    // head literal

    auto operator()(HdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> bool { return operator()(lit.lit(), SyntaxCheck::none); }

    auto operator()(HdLitDisjunction const &lit) const -> bool {
        return std::ranges::all_of(lit.elems(), [this](auto const &elem) {
            return std::visit([this](auto const &elem) { return this->operator()(elem, SyntaxCheck::none); }, elem);
        });
    }

    auto operator()(HdLitAggregate const &lit) const -> bool {
        return std::ranges::all_of(lit.elems(),
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
        return std::ranges::all_of(
                   lit.elems(),
                   [this](auto const &elem) { return operator()(elem.tuple()) && operator()(elem.cond()); }) &&
               operator()(lit.lhs()) && operator()(lit.rhs());
    }

    auto operator()(BdLitSort const &lit) const -> bool {
        auto const *tuple = std::get_if<TermTuple>(&lit.outputs());
        auto const *outputs = tuple != nullptr && tuple->pool().size() == 1
                                  ? std::get_if<ArgumentTuple>(&tuple->pool().front())
                                  : nullptr;
        if (lit.sign() != Sign::none || outputs == nullptr || outputs->elems().size() != 2) {
            CLINGO_REPORT_LOC(*log_, error, lit.loc())
                << "sort literal requires an unnegated pair on the left-hand side";
            return false;
        }
        return operator()(lit.outputs()) && std::ranges::all_of(lit.elems(), [this](auto const &elem) {
                   return elem.tuple().size() == 1 && operator()(elem.tuple()) && operator()(elem.cond());
               });
    }

    // statement

    auto operator()(std::optional<Term> const &term) const -> bool { return (!term || operator()(*term)); }

    auto operator()(BdLitArray const &lits) const -> bool { return std::ranges::all_of(lits, *this); }

    auto operator()(Stm const &stm) const -> bool { return std::visit(*this, stm); }

    auto operator()(StmRule const &stm) const -> bool {
        return operator()(stm.head()) && std::ranges::all_of(stm.body(), *this);
    }

    auto operator()(OptimizeTuple const &tuple) const -> bool {
        return operator()(tuple.weight()) && operator()(tuple.terms()) && operator()(tuple.prio());
    }

    auto operator()(StmOptimize const &stm) const -> bool {
        return std::ranges::all_of(
            stm.elems(), [this](auto const &elem) { return operator()(elem.tuple()) && operator()(elem.cond()); });
    }

    auto operator()(StmWeakConstraint const &stm) const -> bool {
        return std::ranges::all_of(stm.body(), *this) && operator()(stm.tuple());
    }

    auto operator()(StmShow const &stm) const -> bool { return operator()(stm.term()) && operator()(stm.body()); }

    auto operator()(StmProject const &stm) const -> bool { return operator()(stm.atom()) && operator()(stm.body()); }

    auto operator()(StmExternal const &stm) const -> bool {
        return operator()(stm.atom()) && operator()(stm.body()) && operator()(stm.type());
    }

    auto operator()(Edge const &edge) const -> bool { return operator()(edge.src()) && operator()(edge.dst()); }

    auto operator()(StmEdge const &stm) const -> bool {
        return std::ranges::all_of(stm.edges(), *this) && operator()(stm.body());
    }

    auto operator()(StmHeuristic const &stm) const -> bool {
        return operator()(stm.atom()) && operator()(stm.body()) && operator()(stm.type()) && operator()(stm.prio()) &&
                                                                                             operator()(stm.weight());
    }

    auto operator()(StmConst const &stm) const -> bool { return operator()(stm.value(), SyntaxCheck::is_const); }

  private:
    Logger *log_;
};

} // namespace

auto check_term(Logger &log, Term const &term) -> bool {
    return CheckSyntax{log}(term);
}

auto check_literal(Logger &log, Lit const &lit) -> bool {
    return CheckSyntax{log}(lit, SyntaxCheck::project | SyntaxCheck::project_tuple);
}

auto check_head_literal(Logger &log, HdLit const &lit) -> bool {
    return CheckSyntax{log}(lit);
}

auto check_body_literal(Logger &log, BdLit const &lit) -> bool {
    return CheckSyntax{log}(lit);
}

auto check_statement(Logger &log, Stm const &stm) -> bool {
    return CheckSyntax{log}(stm);
}

} // namespace CppClingo::Input
