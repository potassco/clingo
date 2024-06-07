#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/print.hh>

#include <gringo/util/algorithm.hh>
#include <gringo/util/print.hh>

#include <cstring>
#include <sstream>

namespace Gringo::Input {

namespace {

//! Enumeration of term positions.
enum class OperatorPosition : uint8_t {
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

auto priority([[maybe_unused]] UnaryOperator op) -> unsigned int { return priority(BinaryOperator::times) + 1; }

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

class Print {
  public:
    Print(std::ostream &out, OperatorPosition pos = OperatorPosition::none, unsigned int prio = 0,
          bool no_leading_op = false)
        : out_{&out}, pos_{pos}, prio_{prio}, no_leading_op_{no_leading_op} {}

    // protect ourselves -> no unintended overloads

    template <class T> void operator()(T const &x) const = delete;

    // generic

    void apply_to_range_with(auto const &rng, char const *sep, auto const &fun) const {
        bool comma = false;
        for (auto const &x : rng) {
            if (comma) {
                *out_ << sep;
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
        apply_to_range_with(rng, sep, [this](auto const &x) { *out_ << x; });
    }

    // term

    void operator()(Term const &term) const { std::visit(*this, term); }

    void operator()(TermSymbol const &term) const {
        char const *lp = "";
        char const *rp = "";
        if (no_leading_op_ && term.value().has_sign()) {
            lp = "(";
            rp = ")";
        }
        *out_ << lp << term.value() << rp;
    }

    void operator()(TermVariable const &term) const { *out_ << term.name(); }

    void operator()(Projection const &x) const { *out_ << x; }

    void operator()(Argument const &elem) const { std::visit(*this, elem); }

    void operator()(TermTuple const &term) const {
        auto const &pool = term.pool();
        if (pool.size() == 1 && std::holds_alternative<Term>(pool.front())) {
            std::visit(*this, std::get<Term>(term.pool().front()));
        } else {
            *out_ << "(";
            apply_to_range_with(pool, ";", [this](auto const &term_or_tuple) {
                std::visit(
                    [this]<class T>(T const &x) {
                        if constexpr (std::is_same_v<T, Term>) {
                            Print{*out_}(x);
                        } else if constexpr (std::is_same_v<T, ArgumentTuple>) {
                            Print{*out_}.apply_to_range(x.elems());
                            if (x.elems().size() == 1) {
                                *out_ << ",";
                            }
                        }
                    },
                    term_or_tuple);
            });
            *out_ << ")";
        }
    }

    void operator()(TermFunction const &term) const {
        if (term.external()) {
            *out_ << "@";
        }
        *out_ << term.name();
        auto const &pool = term.pool();
        if (pool.size() != 1 || !pool.front().elems().empty()) {
            *out_ << "(";
            apply_to_range_with(pool, ";", [this](auto const &tuple) { Print{*out_}.apply_to_range(tuple.elems()); });
            *out_ << ")";
        }
    }

    void operator()(TermAbs const &term) const {
        *out_ << "|";
        Print{*out_}.visit_range(term.pool(), ";");
        *out_ << "|";
    }

    void operator()(TermUnary const &term) const {
        char const *lp = "";
        char const *rp = "";
        auto op = term.op();
        // No need to consider associativity/position because the unary priority is
        // different from all binary ones.
        if (no_leading_op_ || (priority(op) < prio_)) {
            lp = "(";
            rp = ")";
        }
        *out_ << lp << op;
        std::visit(Print{*out_, OperatorPosition::none, priority(op), true}, *term.rhs());
        *out_ << rp;
    }

    void operator()(TermBinary const &term) const {
        char const *lp = "";
        char const *rp = "";
        auto op = term.op();
        bool lhs_no_leading_op = no_leading_op_;
        // We assume that operators with the same priority have the same associativity.
        if (priority(op) < prio_ || (prio_ == priority(op) && associativity(op) != pos_)) {
            lp = "(";
            rp = ")";
            lhs_no_leading_op = false;
        }
        *out_ << lp;
        std::visit(Print{*out_, OperatorPosition::left, priority(op), lhs_no_leading_op}, *term.lhs());
        *out_ << op;
        std::visit(Print{*out_, OperatorPosition::right, priority(op), true}, *term.rhs());
        *out_ << rp;
    }

    // theory terms

    void operator()(TheoryTerm const &term) const { std::visit(*this, term); }

    void operator()(TheoryTermSymbol const &term) const { *out_ << term.value(); }

    void operator()(TheoryTermVariable const &term) const { *out_ << term.name(); }

    void operator()(TheoryTermTuple const &term) const {
        *out_ << left_bracket(term.type());
        visit_range(term.elems());
        if (term.type() == TheoryTermTupleType::tuple && term.elems().size() == 1) {
            *out_ << ",";
        }
        *out_ << right_bracket(term.type());
    }

    void operator()(TheoryTermFunction const &term) const {
        size_t n = term.args().size();
        if (is_theory_operator(term.name().view()) && 0 < n && n < 3) {
            *out_ << "(";
            if (n == 2) {
                *out_ << term.args().front() << " ";
            }
            *out_ << term.name();
            *out_ << " " << term.args().back();
            *out_ << ")";
        } else {
            *out_ << term.name();
            if (n > 0) {
                *out_ << "(";
                visit_range(term.args());
                *out_ << ")";
            }
        }
    }

    void operator()(TheoryTermUnparsed const &term) const {
        const auto &elems = term.elems();
        bool needs_parens = elems.size() != 1 || !elems.front().ops().empty();
        if (needs_parens) {
            *out_ << "(";
        }
        apply_to_range_with(elems, " ", [this](auto const &elem) {
            for (auto const &op : elem.ops()) {
                *out_ << op << " ";
            }
            operator()(elem.term());
        });
        if (needs_parens) {
            *out_ << ")";
        }
    }

    // literals

    void operator()(Lit const &lit) const { std::visit(*this, lit); }

    void operator()(LitBool const &lit) const { *out_ << lit.sign() << (lit.value() ? "#true" : "#false"); }

    void operator()(LitComparison const &lit) const {
        *out_ << lit.sign() << lit.lhs();
        for (auto const &[rel, term] : lit.rhs()) {
            *out_ << rel << term;
        }
    }

    void operator()(LitSymbolic const &lit) const { *out_ << lit.sign() << lit.term(); }

    // conditional literal

    void operator()(LitArray const &lits) const { visit_range(lits, ", "); }

    void operator()(CondLit const &lit) const {
        operator()(lit.lit());
        *out_ << ": ";
        operator()(lit.cond());
    }

    void operator()(SetAggregateElement const &elem) const {
        operator()(elem.lit());
        if (!elem.cond().empty()) {
            *out_ << ": ";
            operator()(elem.cond());
        }
    }

    template <bool HasSign> void operator()(SetAggregate<HasSign> const &aggr) const {
        if constexpr (HasSign) {
            *out_ << aggr.sign();
        }
        if (auto const &lhs = aggr.lhs(); lhs) {
            *out_ << lhs->first << " " << lhs->second << " ";
        }
        *out_ << "{ ";
        apply_to_range(aggr.elems(), "; ");
        *out_ << (aggr.elems().empty() ? "}" : " }");
        if (auto const &rhs = aggr.rhs(); rhs) {
            *out_ << " " << rhs->first << " " << rhs->second;
        }
    }

    void operator()(TheoryElement const &elem) const {
        visit_range(elem.tuple());
        if (!elem.cond().empty() || elem.tuple().empty()) {
            *out_ << ": ";
            visit_range(elem.cond(), ", ");
        }
    }

    template <bool HasSign> void operator()(TheoryAtom<HasSign> const &atom) const {
        if constexpr (HasSign) {
            *out_ << atom.sign();
        }
        *out_ << "&" << atom.name();
        auto const &elems = atom.elems();
        if (!elems.empty() || atom.rhs().has_value()) {
            *out_ << " { ";
            apply_to_range(elems, "; ");
            *out_ << (elems.empty() ? "}" : " }");
        }
        if (auto const &rhs = atom.rhs(); rhs) {
            *out_ << " " << rhs->op() << " ";
            operator()(rhs->term());
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
        *out_ << ": ";
        operator()(elem.lit());
        if (!elem.cond().empty()) {
            *out_ << ": ";
            visit_range(elem.cond(), ", ");
        }
    }

    void operator()(HdLitAggregate const &lit) const {
        if (auto const &lhs = lit.lhs(); lhs) {
            *out_ << lhs->first << " " << lhs->second << " ";
        }
        *out_ << lit.fun() << " { ";
        apply_to_range_with(lit.elems(), "; ", *this);
        *out_ << (lit.elems().empty() ? "}" : " }");
        if (auto const &rhs = lit.rhs(); rhs) {
            *out_ << " " << rhs->first << " " << rhs->second;
        }
    }

    // body literals

    void operator()(BdLit const &lit) const { std::visit(*this, lit); }

    void operator()(BdLitSimple const &lit) const { operator()(lit.lit()); }

    void operator()(BdLitConjunction const &lit) const { operator()(lit.lit()); }

    void operator()(BdLitAggregateElement const &elem) const {
        visit_range(elem.tuple());
        if (!elem.cond().empty()) {
            *out_ << ": ";
            visit_range(elem.cond(), ", ");
        }
    }

    void operator()(BdLitAggregate const &lit) const {
        *out_ << lit.sign();
        if (auto const &lhs = lit.lhs(); lhs) {
            *out_ << lhs->first << " " << lhs->second << " ";
        }
        *out_ << lit.fun() << " { ";
        apply_to_range_with(lit.elems(), "; ", *this);
        *out_ << (lit.elems().empty() ? "}" : " }");
        if (auto const &rhs = lit.rhs(); rhs) {
            *out_ << " " << rhs->first << " " << rhs->second;
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
            *out_ << stm.head();
        }
        if (empty_head || !stm.body().empty()) {
            *out_ << " :- ";
            visit_range(stm.body(), "; ");
        }
        *out_ << ".";
    }

    void operator()(TheoryOpDefinition const &def) const {
        *out_ << def.op() << " : " << def.prio() << ", ";
        switch (def.type()) {
            case TheoryOpType::unary: {
                *out_ << "unary";
                break;
            }
            case TheoryOpType::binary_left: {
                *out_ << "binary, left";
                break;
            }
            case TheoryOpType::binary_right: {
                *out_ << "binary, right";
                break;
            }
        }
    }

    void operator()(TheoryTermDefinition const &def, char const *pre = "  ") const {
        *out_ << pre << def.name() << " {";
        if (def.op_defs().empty()) {
            *out_ << " }";
        } else if (def.op_defs().size() == 1) {
            *out_ << " ";
            operator()(def.op_defs().front());
            *out_ << " }";
        } else {
            *out_ << "\n";
            apply_to_range_with(def.op_defs(), ";\n", [this](auto &op_def) {
                *out_ << "    ";
                operator()(op_def);
            });
            *out_ << "\n  }";
        }
    }

    void operator()(TheoryRGuardDefinition const &def) const {
        *out_ << "{";
        print_range(def.ops(), ",");
        *out_ << "}, " << def.term();
    }

    void operator()(TheoryAtomDefinition const &def, char const *pre = "  ") const {
        *out_ << pre << "&" << def.name() << "/" << def.arity() << ": " << def.term() << ", ";
        if (auto const &rhs = def.rhs(); rhs) {
            operator()(*rhs);
            *out_ << ", ";
        }
        *out_ << def.type();
    }

    void operator()(StmTheory const &stm) const {
        *out_ << "#theory " << stm.name() << (stm.term_defs().empty() && stm.atom_defs().empty() ? " { " : " {\n");
        apply_to_range(stm.term_defs(), ";\n");
        if (!stm.term_defs().empty()) {
            if (!stm.atom_defs().empty()) {
                *out_ << ";";
            }
            *out_ << "\n";
        }
        apply_to_range(stm.atom_defs(), ";\n");
        if (!stm.atom_defs().empty()) {
            *out_ << "\n";
        }
        *out_ << "}.";
    }

    void operator()(OptimizeTuple const &tuple) const {
        *out_ << tuple.weight();
        if (auto const &prio = tuple.prio(); prio) {
            *out_ << "@";
            operator()(*prio);
        }
        if (!tuple.terms().empty()) {
            *out_ << ",";
            visit_range(tuple.terms());
        }
    }

    void operator()(OptimizeElement const &elem) const {
        operator()(elem.tuple());
        if (!elem.cond().empty()) {
            *out_ << ": ";
            visit_range(elem.cond(), ", ");
        }
    }

    void operator()(StmOptimize const &stm) const {
        *out_ << stm.type() << " { ";
        apply_to_range(stm.elems(), "; ");
        *out_ << (stm.elems().empty() ? "}" : " }") << ".";
    }

    void operator()(StmWeakConstraint const &stm) const {
        auto const &tuple = stm.tuple();
        *out_ << " :~ ";
        visit_range(stm.body(), "; ");
        *out_ << ". [" << tuple.weight();
        if (auto const &prio = tuple.prio(); prio) {
            *out_ << "@" << *prio;
        }
        if (!tuple.terms().empty()) {
            *out_ << ",";
            visit_range(tuple.terms());
        }
        *out_ << "]";
    }

    void operator()(StmShow const &stm) const {
        char const *lp = "";
        char const *rp = "";
        if (check_type(stm.term(), TermCheckType::sig, nullptr)) {
            lp = "(";
            rp = ")";
        }
        *out_ << "#show " << lp << stm.term() << rp << ": ";
        visit_range(stm.body(), "; ");
        *out_ << ".";
    }

    void operator()(StmShowSig const &stm) const {
        *out_ << "#show " << (stm.sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void operator()(StmProject const &stm) const {
        *out_ << "#project " << stm.atom() << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        *out_ << ".";
    }

    void operator()(StmProjectSig const &stm) const {
        *out_ << "#project " << (stm.sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void operator()(StmDefined const &stm) const {
        *out_ << "#defined " << (stm.sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void operator()(StmExternal const &stm) const {
        *out_ << "#external " << stm.atom() << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        *out_ << ".";
        if (auto const &type = stm.type(); type) {
            *out_ << " [" << *type << "]";
        }
    }

    void operator()(Edge const &edge) const {
        operator()(edge.src());
        *out_ << ",";
        operator()(edge.dst());
    }

    void operator()(StmEdge const &stm) const {
        *out_ << "#edge (";
        apply_to_range(stm.edges(), ";");
        *out_ << ")" << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        *out_ << ".";
    }

    void operator()(StmHeuristic const &stm) const {
        *out_ << "#heuristic " << stm.atom() << (stm.body().empty() ? "" : ": ");
        visit_range(stm.body(), "; ");
        *out_ << ". [" << stm.weight();
        if (auto const &prio = stm.prio(); prio) {
            *out_ << "@" << *prio;
        }
        *out_ << "," << stm.type() << "]";
    }

    void operator()(StmScript const &stm) const { *out_ << "#script (" << stm.type() << ")" << stm.value() << "#end."; }

    void operator()(StmInclude const &stm) const {
        if (stm.type() == IncludeType::inbuild) {
            *out_ << "#include <" << stm.value() << ">.";
        } else {
            *out_ << "#include ";
            Util::print_quoted(*out_, stm.value());
            *out_ << ".";
        }
    }

    void operator()(StmProgram const &stm) const {
        *out_ << "#program " << stm.name();
        if (!stm.args().empty()) {
            *out_ << "(";
            print_range(stm.args());
            *out_ << ")";
        }
        *out_ << ".";
    }

    void operator()(StmConst const &stm) const {
        *out_ << "#const " << stm.name() << "=";
        operator()(stm.value());
        *out_ << ". [" << stm.type() << "]";
    }

    void operator()(StmComment const &stm) const { *out_ << stm.value(); }

  private:
    std::ostream *out_;
    OperatorPosition pos_;
    unsigned int prio_;
    bool no_leading_op_;
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

auto operator<<(std::ostream &out, [[maybe_unused]] Projection const &projection) -> std::ostream & {
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
