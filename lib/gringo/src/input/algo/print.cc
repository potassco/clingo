#include <cstring>
#include <sstream>

#include <gringo/util/algorithm.hh>
#include <gringo/util/print.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/print.hh>

namespace Gringo::Input {

namespace {

auto is_theory_operator(std::string_view name) -> bool {
    return (!name.empty() && std::strchr("/!<=>+-*\\?&@|:;~^.", name.front()) != nullptr) || (name == "not");
}

//! Enumeration of term positions.
enum class OperatorPosition : int {
    left,  //!< The term is directly on the right-hand-side of a term.
    right, //!< The term is directly on the left-hand-side of a term.
    none   //!< No position information.
};

auto associativity(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::dots:
        case BinaryOperator::xor_:
        case BinaryOperator::or_:
        case BinaryOperator::and_:
        case BinaryOperator::plus:
        case BinaryOperator::minus:
        case BinaryOperator::times:
        case BinaryOperator::div:
        case BinaryOperator::mod: {
            return OperatorPosition::left;
        }
        case BinaryOperator::pow: {
            break;
        }
    }
    return OperatorPosition::right;
}

auto priority(BinaryOperator op) -> unsigned int {
    switch (op) {
        case BinaryOperator::dots: {
            return 1;
        }
        case BinaryOperator::xor_: {
            return 2;
        }
        case BinaryOperator::or_: {
            return 3;
        }
        case BinaryOperator::and_: {
            return 4;
        }
        case BinaryOperator::plus:
        case BinaryOperator::minus: {
            return 5; // NOLINT
        }
        case BinaryOperator::times:
        case BinaryOperator::div:
        case BinaryOperator::mod: {
            return 6; // NOLINT
        }
        case BinaryOperator::pow: {
            break;
        }
    }
    return 8; // NOLINT
}

auto priority(UnaryOperator op) -> unsigned int {
    static_cast<void>(op);
    return priority(BinaryOperator::times) + 1;
}

auto operator<<(std::ostream &out, TheoryAtomType type) -> std::ostream & {
    switch (type) {
        case TheoryAtomType::head: {
            out << "head";
            break;
        }
        case TheoryAtomType::body: {
            out << "body";
            break;
        }
        case TheoryAtomType::any: {
            out << "any";
            break;
        }
        case TheoryAtomType::directive: {
            out << "directive";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, OptimizeType type) -> std::ostream & {
    switch (type) {
        case OptimizeType::maximize: {
            out << "#maximize";
            break;
        }
        case OptimizeType::minimize: {
            out << "#minimize";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, ConstType type) -> std::ostream & {
    switch (type) {
        case ConstType::default_: {
            out << "default";
            break;
        }
        case ConstType::override_: {
            out << "override";
            break;
        }
    }
    return out;
}

auto left_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::tuple: {
            return '(';
        }
        case TheoryTermTupleType::set: {
            return '{';
        }
        case TheoryTermTupleType::list: {
            break;
        }
    }
    return '[';
}

auto right_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::tuple: {
            return ')';
        }
        case TheoryTermTupleType::set: {
            return '}';
        }
        case TheoryTermTupleType::list: {
            break;
        }
    }
    return ']';
}

struct Print {
    // protect ourselves -> no unintended overloads

    template <class T> void operator()(T const &x) const = delete;

    // generic

    void apply_to_range_with(auto const &rng, char const *sep, auto const &fun) const {
        bool comma = false;
        for (auto const &x : rng) {
            if (comma) {
                out << sep;
            } else {
                comma = true;
            }
            fun(x);
        }
    }

    void apply_to_range(auto const &rng, char const *sep = ",") const {
        apply_to_range_with(rng, sep, [this](auto const &x) { this->operator()(x); });
    }

    void visit_range(auto const &rng, char const *sep = ",") const {
        apply_to_range_with(rng, sep, [this](auto const &x) { std::visit(*this, x); });
    }

    void print_range(auto const &rng, char const *sep = ",") const {
        apply_to_range_with(rng, sep, [this](auto const &x) { out << x; });
    }

    // term

    void operator()(Term const &term) const { std::visit(*this, term); }

    void operator()(TermSymbol const &term) const {
        char const *lp = "";
        char const *rp = "";
        if (no_leading_op && term.value().has_sign()) {
            lp = "(";
            rp = ")";
        }
        out << lp << term.value() << rp;
    }

    void operator()(TermVariable const &term) const { out << term.name(); }

    void operator()(Projection const &x) const { out << x; }

    void operator()(Argument const &elem) const { std::visit(*this, elem); }

    void operator()(TermTuple const &term) const {
        auto const &pool = term.pool();
        if (pool.size() == 1 && std::holds_alternative<Term>(pool.front())) {
            std::visit(*this, std::get<Term>(term.pool().front()));
        } else {
            out << "(";
            apply_to_range_with(pool, ";", [this](auto const &term_or_tuple) {
                std::visit(
                    [this]<class T>(T const &x) {
                        if constexpr (std::is_same_v<T, Term>) {
                            Print{out}(x);
                        } else if constexpr (std::is_same_v<T, ArgumentTuple>) {
                            Print{out}.apply_to_range(x.elems());
                            if (x.elems().size() == 1) {
                                out << ",";
                            }
                        }
                    },
                    term_or_tuple);
            });
            out << ")";
        }
    }

    void operator()(TermFunction const &term) const {
        if (term.external()) {
            out << "@";
        }
        out << term.name();
        auto const &pool = term.pool();
        if (pool.size() != 1 || !pool.front().elems().empty()) {
            out << "(";
            apply_to_range_with(pool, ";", [this](auto const &tuple) { Print{out}.apply_to_range(tuple.elems()); });
            out << ")";
        }
    }

    void operator()(TermAbs const &term) const {
        out << "|";
        Print{out}.visit_range(term.pool(), ";");
        out << "|";
    }

    void operator()(TermUnary const &term) const {
        char const *lp = "";
        char const *rp = "";
        auto op = term.op();
        // No need to consider associativity/position because the unary priority is
        // different from all binary ones.
        if (no_leading_op || (priority(op) < prio)) {
            lp = "(";
            rp = ")";
        }
        out << lp << op;
        std::visit(Print{out, OperatorPosition::none, priority(op), true}, *term.rhs());
        out << rp;
    }

    void operator()(TermBinary const &term) const {
        char const *lp = "";
        char const *rp = "";
        auto op = term.op();
        bool lhs_no_leading_op = no_leading_op;
        // We assume that operators with the same priority have the same associativity.
        if (priority(op) < prio || (prio == priority(op) && associativity(op) != pos)) {
            lp = "(";
            rp = ")";
            lhs_no_leading_op = false;
        }
        out << lp;
        std::visit(Print{out, OperatorPosition::left, priority(op), lhs_no_leading_op}, *term.lhs());
        out << op;
        std::visit(Print{out, OperatorPosition::right, priority(op), true}, *term.rhs());
        out << rp;
    }

    // theory terms

    void operator()(TheoryTerm const &term) const { std::visit(*this, term); }

    void operator()(TheoryTermSymbol const &term) const { out << term.value(); }

    void operator()(TheoryTermVariable const &term) const { out << term.name(); }

    void operator()(TheoryTermTuple const &term) const {
        out << left_bracket(term.type());
        visit_range(term.elems());
        if (term.type() == TheoryTermTupleType::tuple && term.elems().size() == 1) {
            out << ",";
        }
        out << right_bracket(term.type());
    }

    void operator()(TheoryTermFunction const &term) const {
        size_t n = term.args().size();
        if (is_theory_operator(term.name().view()) && 0 < n && n < 3) {
            out << "(";
            if (n == 2) {
                out << term.args().front() << " ";
            }
            out << term.name();
            out << " " << term.args().back();
            out << ")";
        } else {
            out << term.name();
            if (n > 0) {
                out << "(";
                visit_range(term.args());
                out << ")";
            }
        }
    }

    void operator()(TheoryTermUnparsed const &term) const {
        auto elems = term.elems();
        bool needs_parens = elems.size() != 1 || !elems.front().first.empty();
        if (needs_parens) {
            out << "(";
        }
        apply_to_range_with(elems, " ", [this](auto const &elem) {
            for (auto const &op : elem.first) {
                out << op << " ";
            }
            operator()(elem.second);
        });
        if (needs_parens) {
            out << ")";
        }
    }

    // literals

    void operator()(Lit const &lit) const { std::visit(*this, lit); }

    void operator()(LitBool const &lit) const { out << lit.sign() << (lit.value() ? "#true" : "#false"); }

    void operator()(LitComparison const &lit) const {
        out << lit.sign() << lit.lhs();
        for (auto const &[rel, term] : lit.rhs()) {
            out << rel << term;
        }
    }

    void operator()(LitSymbolic const &lit) const { out << lit.sign() << lit.term(); }

    // conditional literal

    void operator()(LitArray const &lits) const { visit_range(lits, ", "); }

    void operator()(CondLit const &lit) const {
        operator()(lit.lit());
        out << ": ";
        operator()(lit.cond());
    }

    void operator()(SetAggregateElement const &elem) const {
        operator()(elem.lit());
        if (!elem.cond().empty()) {
            out << ": ";
            operator()(elem.cond());
        }
    }

    template <bool HasSign> void operator()(SetAggregate<HasSign> const &aggr) const {
        if constexpr (HasSign) {
            out << aggr.sign();
        }
        if (aggr.lhs().has_value()) {
            out << aggr.lhs()->first << " " << aggr.lhs()->second << " ";
        }
        out << "{ ";
        apply_to_range(aggr.elems(), "; ");
        out << (aggr.elems().empty() ? "}" : " }");
        if (aggr.rhs().has_value()) {
            out << " " << aggr.rhs()->first << " " << aggr.rhs()->second;
        }
    }

    void operator()(TheoryElement const &elem) const {
        visit_range(elem.tuple());
        if (!elem.cond().empty() || elem.tuple().empty()) {
            out << ": ";
            visit_range(elem.cond(), ", ");
        }
    }

    template <bool HasSign> void operator()(TheoryAtom<HasSign> const &atom) const {
        if constexpr (HasSign) {
            out << atom.sign();
        }
        out << "&" << atom.name();
        auto const &elems = atom.elems();
        if (!elems.empty() || atom.rhs().has_value()) {
            out << " { ";
            apply_to_range(elems, "; ");
            out << (elems.empty() ? "}" : " }");
        }
        if (atom.rhs().has_value()) {
            out << " " << atom.rhs().value().first << " ";
            operator()(atom.rhs().value().second);
        }
    }

    // head literals

    void operator()(HdLit const &lit) const { std::visit(*this, lit); }

    void operator()(HdLitSimple const &lit) const { operator()(lit.lit()); }

    void operator()(HdLitDisjunction const &lit) const {
        apply_to_range_with(lit.elems(), "; ", [this](auto const &elem) { std::visit(*this, elem); });
    }

    void operator()(HdLitAggregateElement const &elem) const {
        visit_range(elem.tuple());
        out << ": ";
        operator()(elem.lit());
        if (!elem.cond().empty()) {
            out << ": ";
            visit_range(elem.cond(), ", ");
        }
    }

    void operator()(HdLitAggregate const &lit) const {
        auto const &lhs = lit.lhs();
        auto const &rhs = lit.rhs();
        if (lhs.has_value()) {
            out << lhs->first << " " << lhs->second << " ";
        }
        out << lit.fun() << " { ";
        apply_to_range_with(lit.elems(), "; ", *this);
        out << (lit.elems().empty() ? "}" : " }");
        if (rhs.has_value()) {
            out << " " << rhs->first << " " << rhs->second;
        }
    }

    // body literals

    void operator()(BdLit const &lit) const { std::visit(*this, lit); }

    void operator()(BdLitSimple const &lit) const { operator()(lit.lit()); }

    void operator()(BdLitConjunction const &lit) const { operator()(lit.lit()); }

    void operator()(BdLitAggregateElement const &elem) const {
        visit_range(elem.tuple());
        if (!elem.cond().empty()) {
            out << ": ";
            visit_range(elem.cond(), ", ");
        }
    }

    void operator()(BdLitAggregate const &lit) const {
        out << lit.sign();
        if (lit.lhs().has_value()) {
            out << lit.lhs()->first << " " << lit.lhs()->second << " ";
        }
        out << lit.fun() << " { ";
        apply_to_range_with(lit.elems(), "; ", *this);
        out << (lit.elems().empty() ? "}" : " }");
        if (lit.rhs().has_value()) {
            out << " " << lit.rhs()->first << " " << lit.rhs()->second;
        }
    }

    // visit statements

    void operator()(Stm const &stm) const { std::visit(*this, stm); }

    void operator()(StmRule const &stm) const {
        bool empty_head = std::visit(
            []<class T>(T const &head) {
                if constexpr (std::is_same_v<T, HdLitDisjunction>) {
                    return head.elems().empty();
                } else if constexpr (std::is_same_v<T, HdLitSimple>) {
                    auto val = is_boolean(head.lit());
                    return val && !*val;
                }
                return false;
            },
            stm.head());
        if (!empty_head) {
            out << stm.head();
        }
        if (empty_head || !stm.body().empty()) {
            out << " :- ";
            visit_range(stm.body(), "; ");
        }
        out << ".";
    }

    void operator()(TheoryOpDefinition const &def) const {
        out << def.op() << " : " << def.prio() << ", ";
        switch (def.type()) {
            case TheoryOpType::unary: {
                out << "unary";
                break;
            }
            case TheoryOpType::binary_left: {
                out << "binary, left";
                break;
            }
            case TheoryOpType::binary_right: {
                out << "binary, right";
                break;
            }
        }
    }

    void operator()(TheoryTermDefinition const &def, char const *pre = "  ") const {
        out << pre << def.name() << " {";
        if (def.op_defs().empty()) {
            out << " }";
        } else if (def.op_defs().size() == 1) {
            out << " ";
            operator()(def.op_defs().front());
            out << " }";
        } else {
            out << "\n";
            apply_to_range_with(def.op_defs(), ";\n", [this](auto &op_def) {
                out << "    ";
                operator()(op_def);
            });
            out << "\n  }";
        }
    }

    void operator()(TheoryRGuardDefinition const &def) const {
        out << "{";
        print_range(def.first);
        out << "}, " << def.second;
    }

    void operator()(TheoryAtomDefinition const &def, char const *pre = "  ") const {
        out << pre << "&" << def.name() << "/" << def.arity() << ": " << def.term() << ", ";
        if (def.rhs()) {
            operator()(*def.rhs());
            out << ", ";
        }
        out << def.type();
    }

    void operator()(StmTheory const &stm) const {
        out << "#theory " << stm.name() << (stm.term_defs().empty() && stm.atom_defs().empty() ? " { " : " {\n");
        apply_to_range(stm.term_defs(), ";\n");
        if (!stm.term_defs().empty()) {
            if (!stm.atom_defs().empty()) {
                out << ";";
            }
            out << "\n";
        }
        apply_to_range(stm.atom_defs(), ";\n");
        if (!stm.atom_defs().empty()) {
            out << "\n";
        }
        out << "}.";
    }

    void operator()(OptimizeTuple const &tuple) const {
        out << tuple.weight();
        if (tuple.prio()) {
            out << "@";
            operator()(*tuple.prio());
        }
        if (!tuple.terms().empty()) {
            out << ",";
            visit_range(tuple.terms());
        }
    }

    void operator()(OptimizeElement const &elem) const {
        operator()(elem.first);
        if (!elem.second.empty()) {
            out << ": ";
            visit_range(elem.second, ", ");
        }
    }

    void operator()(StmOptimize const &stm) const {
        out << stm.type() << " { ";
        apply_to_range(stm.elems(), "; ");
        out << (stm.elems().empty() ? "}" : " }") << ".";
    }

    void operator()(StmWeakConstraint const &stm) const {
        auto const &tuple = stm.tuple();
        out << " :~ ";
        visit_range(stm.body(), "; ");
        out << ". [" << tuple.weight();
        if (tuple.prio()) {
            out << "@" << *tuple.prio();
        }
        if (!tuple.terms().empty()) {
            out << ",";
            visit_range(tuple.terms());
        }
        out << "]";
    }

    void operator()(StmShow const &stm) const {
        char const *lp = "";
        char const *rp = "";
        if (check_type(stm.term(), TermCheckType::sig, nullptr)) {
            lp = "(";
            rp = ")";
        }
        out << "#show " << lp << stm.term() << rp << ": ";
        visit_range(stm.body(), "; ");
        out << ".";
    }

    void operator()(StmShowSig const &stm) const {
        out << "#show " << (stm.sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void operator()(StmProject const &stm) const {
        out << "#project " << stm.term() << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        out << ".";
    }

    void operator()(StmProjectSig const &stm) const {
        out << "#project " << (stm.sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void operator()(StmDefined const &stm) const {
        out << "#defined " << (stm.sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void operator()(StmExternal const &stm) const {
        out << "#external " << stm.term() << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        out << ".";
        if (stm.type().has_value()) {
            out << " [" << stm.type().value() << "]";
        }
    }

    void operator()(Edge const &edge) const {
        operator()(edge.src());
        out << ",";
        operator()(edge.dst());
    }

    void operator()(StmEdge const &stm) const {
        out << "#edge (";
        apply_to_range(stm.edges(), ";");
        out << ")" << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        out << ".";
    }

    void operator()(StmHeuristic const &stm) const {
        out << "#heuristic " << stm.atom() << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        out << ". [" << stm.weight();
        if (stm.prio().has_value()) {
            out << "@" << stm.prio().value();
        }
        out << "," << stm.type() << "]";
    }

    void operator()(StmScript const &stm) const { out << "#script (" << stm.type() << ")" << stm.value() << "#end."; }

    void operator()(StmInclude const &stm) const {
        if (stm.type() == IncludeType::inbuild) {
            out << "#include <" << stm.value() << ">.";
        } else {
            out << "#include ";
            Util::print_quoted(out, stm.value());
            out << ".";
        }
    }

    void operator()(StmProgram const &stm) const {
        out << "#program " << stm.name();
        if (!stm.args().empty()) {
            out << "(";
            print_range(stm.args());
            out << ")";
        }
        out << ".";
    }

    void operator()(StmConst const &stm) const {
        out << "#const " << stm.name() << "=";
        operator()(stm.value());
        out << ". [" << stm.type() << "]";
    }

    void operator()(StmComment const &stm) const { out << stm.value(); }

    std::ostream &out;
    OperatorPosition pos = OperatorPosition::none;
    unsigned int prio = 0;
    bool no_leading_op = false;
};

} // namespace

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream & {
    out << (op == UnaryOperator::negate ? "-" : "~");
    return out;
}

auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream & {
    switch (op) {
        case BinaryOperator::dots: {
            out << "..";
            break;
        }
        case BinaryOperator::xor_: {
            out << "^";
            break;
        }
        case BinaryOperator::or_: {
            out << "?";
            break;
        }
        case BinaryOperator::and_: {
            out << "&";
            break;
        }
        case BinaryOperator::plus: {
            out << "+";
            break;
        }
        case BinaryOperator::minus: {
            out << "-";
            break;
        }
        case BinaryOperator::times: {
            out << "*";
            break;
        }
        case BinaryOperator::div: {
            out << "/";
            break;
        }
        case BinaryOperator::mod: {
            out << "\\";
            break;
        }
        case BinaryOperator::pow: {
            out << "**";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, Relation op) -> std::ostream & {
    switch (op) {
        case Relation::less: {
            out << "<";
            break;
        }
        case Relation::less_equal: {
            out << "<=";
            break;
        }
        case Relation::greater: {
            out << ">";
            break;
        }
        case Relation::greater_equal: {
            out << ">=";
            break;
        }
        case Relation::equal: {
            out << "=";
            break;
        }
        case Relation::inequal: {
            out << "!=";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, Sign sign) -> std::ostream & {
    switch (sign) {
        case Sign::none: {
            break;
        }
        case Sign::once: {
            out << "not ";
            break;
        }
        case Sign::twice: {
            out << "not not ";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream & {
    switch (fun) {
        case AggregateFunction::count: {
            out << "#count";
            break;
        }
        case AggregateFunction::sum: {
            out << "#sum";
            break;
        }
        case AggregateFunction::sump: {
            out << "#sum+";
            break;
        }
        case AggregateFunction::min: {
            out << "#min";
            break;
        }
        case AggregateFunction::max: {
            out << "#max";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, Position const &pos) -> std::ostream & {
    out << pos.file << ":" << pos.line << ":" << pos.column;
    return out;
}

auto operator<<(std::ostream &out, Location const &loc) -> std::ostream & {
    out << loc.begin << "-";
    if (loc.end.file != loc.begin.file) {
        out << loc.end;
    } else if (loc.end.line != loc.begin.line) {
        out << loc.end.line << ":" << loc.end.column;
    } else {
        out << loc.end.column;
    }
    return out;
}

// terms

auto operator<<(std::ostream &out, Projection const &projection) -> std::ostream & {
    static_cast<void>(projection);
    out << "*";
    return out;
}

auto operator<<(std::ostream &out, TermVariable const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TermSymbol const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TermAbs const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TermUnary const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TermBinary const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TermTuple const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TermFunction const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

// theory terms

auto operator<<(std::ostream &out, TheoryTermVariable const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TheoryTermSymbol const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TheoryTermTuple const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TheoryTermFunction const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TheoryTermUnparsed const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

// aggregates

auto operator<<(std::ostream &out, CondLit const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, SetAggregateElement const &elem) -> std::ostream & {
    Print{out}(elem);
    return out;
}

auto operator<<(std::ostream &out, TheoryElement const &elem) -> std::ostream & {
    Print{out}(elem);
    return out;
}

auto operator<<(std::ostream &out, HdLitAggregateElement const &elem) -> std::ostream & {
    Print{out}(elem);
    return out;
}

auto operator<<(std::ostream &out, BdLitAggregateElement const &elem) -> std::ostream & {
    Print{out}(elem);
    return out;
}

// literals

auto operator<<(std::ostream &out, LitBool const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, LitComparison const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, LitSymbolic const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, Lit const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

// head literals

auto operator<<(std::ostream &out, HdLitSimple const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, HdLitDisjunction const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, HdLitSetAggregate const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, HdLitAggregate const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, HdLitTheoryAtom const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, HdLit const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

// body literals

auto operator<<(std::ostream &out, BdLitSimple const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, BdLitConjunction const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, BdLitSetAggregate const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, BdLitAggregate const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, BdLitTheoryAtom const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, BdLit const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

// statement elements

auto operator<<(std::ostream &out, OptimizeTuple const &tuple) -> std::ostream & {
    Print{out}(tuple);
    return out;
}

auto operator<<(std::ostream &out, OptimizeElement const &elem) -> std::ostream & {
    Print{out}(elem);
    return out;
}

auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream & {
    Print{out}(def);
    return out;
}

auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream & {
    Print{out}(def, "");
    return out;
}

auto operator<<(std::ostream &out, TheoryRGuardDefinition const &def) -> std::ostream & {
    Print{out}(def);
    return out;
}

auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream & {
    Print{out}(def, "");
    return out;
}

auto operator<<(std::ostream &out, Edge const &edge) -> std::ostream & {
    Print{out}(edge);
    return out;
}

// statements

auto operator<<(std::ostream &out, StmRule const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmTheory const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmOptimize const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmWeakConstraint const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmShow const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmShowSig const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmProject const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmProjectSig const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmDefined const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmExternal const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmEdge const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmHeuristic const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmScript const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmInclude const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmProgram const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmConst const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, StmComment const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream & {
    Print{out}(stm);
    return out;
}

auto to_string(Term const &term) -> std::string {
    std::ostringstream oss;
    oss << term;
    return oss.str();
}

auto to_string(TheoryTerm const &term) -> std::string {
    std::ostringstream oss;
    oss << term;
    return oss.str();
}

auto to_string(Lit const &lit) -> std::string {
    std::ostringstream oss;
    oss << lit;
    return oss.str();
}

auto to_string(HdLit const &lit) -> std::string {
    std::ostringstream out;
    out << lit;
    return out.str();
}

auto to_string(BdLit const &lit) -> std::string {
    std::ostringstream out;
    out << lit;
    return out.str();
}

auto to_string(Stm const &stm) -> std::string {
    std::ostringstream out;
    out << stm;
    return out.str();
}

} // namespace Gringo::Input
