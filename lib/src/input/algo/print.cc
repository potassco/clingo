#include <sstream>

#include <util/algorithm.hh>
#include <util/print.hh>

#include <input/algo/check_type.hh>
#include <input/algo/print.hh>

namespace Gringo::Input {

namespace {

//! Enumeration of term positions.
enum class Position : int {
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
            return Position::left;
        }
        case BinaryOperator::pow: {
            break;
        }
    }
    return Position::right;
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

auto operator<<(std::ostream &out, Sign op) -> std::ostream & {
    switch (op) {
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

auto operator<<(std::ostream &out, ScriptType type) -> std::ostream & {
    switch (type) {
        case ScriptType::lua: {
            out << "lua";
            break;
        }
        case ScriptType::python: {
            out << "python";
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
        if (no_leading_op && has_sign(term.value)) {
            lp = "(";
            rp = "(";
        }
        out << lp << term.value << rp;
    }

    void operator()(TermVariable const &term) const { out << term.name; }

    void operator()(TupleElem const &elem) const {
        Util::visit_variant(
            elem, [this](std::monostate) { out << "*"; }, [&](Term const &term) { std::visit(*this, term); });
    }

    void operator()(TermTuple const &term) const {
        auto const &pool = term.pool;
        if (pool.size() == 1 && std::holds_alternative<Term>(pool.front())) {
            std::visit(*this, std::get<Term>(term.pool.front()));
        } else {
            out << "(";
            Print{out}.apply_to_range_with(pool, ";", [this](auto const &term_or_tuple) {
                Util::visit_variant(
                    term_or_tuple, [this](Term const &term) { std::visit(*this, term); },
                    [this](TupleVec const &tuple) {
                        apply_to_range(tuple);
                        if (tuple.size() == 1) {
                            out << ",";
                        }
                    });
            });
            out << ")";
        }
    }

    void operator()(TermFunction const &term) const {
        if (term.external) {
            out << "@";
        }
        out << term.name;
        auto const &pool = term.pool;
        if (pool.size() != 1 || !pool.front().empty()) {
            out << "(";
            Print{out}.apply_to_range_with(pool, ";", [this](auto const &tuple) { apply_to_range(tuple); });
            out << ")";
        }
    }

    void operator()(TermAbs const &term) const {
        out << "|";
        Print{out}.visit_range(term.pool, ";");
        out << "|";
    }

    void operator()(TermUnary const &term) const {
        char const *lp = "";
        char const *rp = "";
        auto op = term.op;
        // No need to consider associativity/position because the unary priority is
        // different from all binary ones.
        if (no_leading_op || (priority(op) < prio)) {
            lp = "(";
            rp = ")";
        }
        out << lp << op;
        std::visit(Print{out, Position::none, priority(op), true}, *term.rhs);
        out << rp;
    }

    void operator()(TermBinary const &term) const {
        char const *lp = "";
        char const *rp = "";
        auto op = term.op;
        bool lhs_no_leading_op = no_leading_op;
        // We assume that operators with the same priority have the same associativity.
        if (priority(op) < prio || (prio == priority(op) && associativity(op) != pos)) {
            lp = "(";
            rp = ")";
            lhs_no_leading_op = false;
        }
        out << lp;
        std::visit(Print{out, Position::left, priority(op), lhs_no_leading_op}, *term.lhs);
        out << op;
        std::visit(Print{out, Position::right, priority(op), true}, *term.rhs);
        out << rp;
    }

    // theory terms

    void operator()(TheoryTerm const &term) const { std::visit(*this, term); }

    void operator()(TheoryTermSymbol const &term) const { out << term.value; }

    void operator()(TheoryTermVariable const &term) const { out << term.name; }

    void operator()(TheoryTermTuple const &term) const {
        out << left_bracket(term.type);
        visit_range(term.elems);
        if (term.type == TheoryTermTupleType::tuple && term.elems.size() == 1) {
            out << ",";
        }
        out << right_bracket(term.type);
    }

    void operator()(TheoryTermFunction const &term) const {
        out << term.name;
        if (!term.args.empty()) {
            out << "(";
            visit_range(term.args);
            out << ")";
        }
    }

    void operator()(TheoryTermUnparsed const &term) const {
        auto elems = term.elems;
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

    void operator()(Literal const &lit) const { std::visit(*this, lit); }

    void operator()(LiteralBoolean const &lit) const { out << lit.sign << (lit.value ? "#true" : "#false"); }

    void operator()(LiteralRelation const &lit) const {
        out << lit.sign << lit.lhs;
        for (auto const &[rel, term] : lit.rhs) {
            out << rel << term;
        }
    }

    void operator()(LiteralSymbolic const &lit) const { out << lit.sign << lit.term; }

    // conditional literal

    void operator()(ConditionalLiteralVec const &elems, char const *kw, bool simple_empty) const {
        auto is_simple = elems.empty() ? simple_empty : std::all_of(elems.begin(), elems.end(), [&](auto const &elem) {
            return elem.lits.size() == 1;
        });
        if (is_simple) {
            apply_to_range_with(elems, "; ", [this](auto const &elem) {
                auto cs = elem.cond.empty() ? "" : ": ";
                operator()(elem.lits.front());
                out << cs;
                visit_range(elem.cond, ", ");
            });
        } else {
            char const *sp = elems.empty() ? "" : " ";
            out << kw << " { ";
            apply_to_range_with(elems, "; ", [this](auto const &elem) {
                char const *cs = !elem.cond.empty() ? ": " : elem.lits.empty() ? ":" : "";
                visit_range(elem.lits, ", ");
                out << cs;
                visit_range(elem.cond, ", ");
            });
            out << sp << "}";
        }
    }

    void operator()(SetAggregate const &aggr) const {
        if (aggr.lhs.has_value()) {
            out << aggr.lhs->first << " " << aggr.lhs->second << " ";
        }
        out << "{ ";
        apply_to_range_with(aggr.elems, "; ", [this](auto const &elem) {
            operator()(elem.lit);
            if (!elem.cond.empty()) {
                out << ": ";
                visit_range(elem.cond, ", ");
            }
        });
        out << (aggr.elems.empty() ? "}" : " }");
        if (aggr.rhs.has_value()) {
            out << " " << aggr.rhs->first << " " << aggr.rhs->second;
        }
    }

    void operator()(TheoryAtom const &atom) const {
        out << "&" << atom.name;
        auto const &elems = atom.elems;
        if (!elems.empty() || atom.rhs.has_value()) {
            out << " { ";
            apply_to_range_with(elems, "; ", [this](TheoryAtom::Element const &elem) {
                visit_range(elem.first);
                if (!elem.second.empty() || elem.first.empty()) {
                    out << ": ";
                    visit_range(elem.second, ", ");
                }
            });
            out << (elems.empty() ? "}" : " }");
        }
        if (atom.rhs.has_value()) {
            out << " " << atom.rhs.value().first << " ";
            operator()(atom.rhs.value().second);
        }
    }

    // head literals

    void operator()(HeadLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(Disjunction const &lit) const { operator()(lit.elems, "#or", true); }

    void operator()(HeadSetAggregate const &lit) const { operator()(lit.aggr); }

    void operator()(HeadAggregate const &lit) const {
        auto const &lhs = lit.lhs;
        auto const &rhs = lit.rhs;
        if (lhs.has_value()) {
            out << lhs->first << " " << lhs->second << " ";
        }
        out << lit.fun << " { ";
        apply_to_range_with(lit.elems, "; ", [this](auto const &elem) {
            visit_range(elem.tuple);
            out << ": ";
            operator()(elem.lit);
            if (!elem.cond.empty()) {
                out << ": ";
                visit_range(elem.cond, ", ");
            }
        });
        out << (lit.elems.empty() ? "}" : " }");
        if (rhs.has_value()) {
            out << " " << rhs->first << " " << rhs->second;
        }
    }

    void operator()(HeadTheoryAtom const &lit) const { operator()(lit.atom); }

    // body literals

    void operator()(BodyLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(Conjunction const &lit) const { operator()(lit.elems, "#and", false); }

    void operator()(BodySetAggregate const &lit) const {
        out << lit.sign;
        operator()(lit.aggr);
    }

    void operator()(BodyAggregate const &lit) const {
        out << lit.sign;
        if (lit.lhs.has_value()) {
            out << lit.lhs->first << " " << lit.lhs->second << " ";
        }
        out << lit.fun << " { ";
        apply_to_range_with(lit.elems, "; ", [this](auto const &elem) {
            visit_range(elem.tuple);
            if (!elem.cond.empty()) {
                out << ": ";
                visit_range(elem.cond, ", ");
            }
        });
        out << (lit.elems.empty() ? "}" : " }");
        if (lit.rhs.has_value()) {
            out << " " << lit.rhs->first << " " << lit.rhs->second;
        }
    }

    void operator()(BodyTheoryAtom const &lit) const {
        out << lit.sign;
        operator()(lit.atom);
    }

    std::ostream &out;
    Position pos = Position::none;
    unsigned int prio = 0;
    bool no_leading_op = false;
};

class PrintVisitor : public StatementVisitor {
  public:
    PrintVisitor(std::ostream &out) : out_{out} {}

    // generic functions

    void apply_to_range_with(auto const &rng, char const *sep, auto const &fun) const {
        bool comma = false;
        for (auto const &x : rng) {
            if (comma) {
                out_ << sep;
            } else {
                comma = true;
            }
            fun(x);
        }
    }

    void visit_range(auto const &rng, char const *sep = ",") const {
        apply_to_range_with(rng, sep, [this](auto const &x) { x->accept(*this); });
    }

    void apply_to_range(auto const &rng, char const *sep = ",") const {
        apply_to_range_with(rng, sep, [this](auto const &x) { this->visit(x); });
    }

    void print_range(auto const &rng, char const *sep = ",") const {
        apply_to_range_with(rng, sep, [this](auto const &x) { out_ << x; });
    }

    // auxiliary functions

    void visit(TheoryOpDefinition const &def) const {
        out_ << def.theory_operator() << " : " << def.priority() << ", ";
        switch (def.type()) {
            case TheoryOpType::unary: {
                out_ << "unary";
                break;
            }
            case TheoryOpType::binary_left: {
                out_ << "binary, left";
                break;
            }
            case TheoryOpType::binary_right: {
                out_ << "binary, right";
                break;
            }
        }
    }

    void visit(TheoryTermDefinition const &def) const {
        out_ << "  " << def.name() << " {";
        if (def.operator_definitions().empty()) {
            out_ << " }";
        } else if (def.operator_definitions().size() == 1) {
            out_ << " ";
            visit(def.operator_definitions().front());
            out_ << " }";
        } else {
            out_ << "\n";
            apply_to_range_with(def.operator_definitions(), ";\n", [this](auto &op_def) {
                out_ << "    ";
                visit(op_def);
            });
            out_ << "\n  }";
        }
    }

    void visit(TheoryAtomDefinition const &def) const {
        out_ << "  &" << def.name() << "/" << def.arity() << ": " << def.term() << ", ";
        if (def.rhs().has_value()) {
            out_ << "{";
            print_range(def.rhs()->first);
            out_ << "}, " << def.rhs()->second << ", ";
        }
        out_ << def.type();
    }

    // visit head literals

    // visit body literals

    // visit statements

    void visit(Rule const &stm) const override {
        auto const *disj = std::get_if<Disjunction>(&stm.head());
        bool empty_head = disj != nullptr && disj->elems.empty();
        if (!empty_head) {
            out_ << stm.head();
        }
        if (empty_head || !stm.body().empty()) {
            out_ << " :- ";
            Print{out_}.visit_range(stm.body(), "; ");
        }
        out_ << ".";
    }

    void visit(TheoryDefinition const &stm) const override {
        out_ << "#theory " << stm.name() << (stm.term_defs().empty() && stm.atom_defs().empty() ? " { " : " {\n");
        apply_to_range(stm.term_defs(), ";\n");
        if (!stm.term_defs().empty()) {
            if (!stm.atom_defs().empty()) {
                out_ << ";";
            }
            out_ << "\n";
        }
        apply_to_range(stm.atom_defs(), ";\n");
        if (!stm.atom_defs().empty()) {
            out_ << "\n";
        }
        out_ << "}.";
    }

    void visit(StatementOptimize const &stm) const override {
        out_ << stm.type() << " { ";
        apply_to_range_with(stm.elements(), "; ", [this](auto const &elem) {
            auto const &[tuple, cond] = elem;
            auto const &[weight, prio, terms] = tuple;
            out_ << weight;
            if (prio) {
                out_ << "@" << prio.value();
            }
            if (!terms.empty()) {
                out_ << ",";
                Print{out_}.visit_range(terms);
            }
            if (!cond.empty()) {
                out_ << ": ";
                Print{out_}.visit_range(cond, ", ");
            }
        });
        out_ << (stm.elements().empty() ? "}" : " }") << ".";
    }

    void visit(StatementWeakConstraint const &stm) const override {
        auto const &[weight, prio, terms] = stm.tuple();
        out_ << " :~ ";
        Print{out_}.visit_range(stm.body(), "; ");
        out_ << ". [" << weight;
        if (prio) {
            out_ << "@" << prio.value();
        }
        if (!terms.empty()) {
            out_ << ",";
            Print{out_}.visit_range(terms);
        }
        out_ << "]";
    }

    void visit(StatementShow const &stm) const override {
        char const *lp = "";
        char const *rp = "";
        if (check_type(stm.term(), TermCheckType::sig, nullptr)) {
            lp = "(";
            rp = ")";
        }
        out_ << "#show " << lp << stm.term() << rp << ": ";
        Print{out_}.visit_range(stm.body(), "; ");
        out_ << ".";
    }

    void visit(StatementShowSig const &stm) const override {
        out_ << "#show " << (stm.has_sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void visit(StatementProject const &stm) const override {
        out_ << "#project " << stm.term() << (stm.body().empty() ? "" : ": ");
        Print{out_}.visit_range(stm.body(), "; ");
        out_ << ".";
    }

    void visit(StatementProjectSig const &stm) const override {
        out_ << "#project " << (stm.has_sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void visit(StatementDefined const &stm) const override {
        out_ << "#defined " << (stm.has_sign() ? "-" : "") << stm.name() << "/" << stm.arity() << ".";
    }

    void visit(StatementExternal const &stm) const override {
        out_ << "#external " << stm.term() << (stm.body().empty() ? "" : ": ");
        Print{out_}.visit_range(stm.body(), "; ");
        out_ << ".";
        if (stm.type().has_value()) {
            out_ << " [" << stm.type().value() << "]";
        }
    }

    void visit(StatementEdge const &stm) const override {
        out_ << "#edge (";
        apply_to_range_with(stm.edges(), ";", [this](auto const &edge) { out_ << edge.first << "," << edge.second; });
        out_ << ")" << (stm.body().empty() ? "" : ": ");
        Print{out_}.visit_range(stm.body(), "; ");
        out_ << ".";
    }

    void visit(StatementHeuristic const &stm) const override {
        out_ << "#heuristic " << stm.atom() << (stm.body().empty() ? "" : ": ");
        Print{out_}.visit_range(stm.body(), "; ");
        out_ << ". [" << stm.type();
        if (stm.priority().has_value()) {
            out_ << "@" << stm.priority().value();
        }
        out_ << "," << stm.modifier() << "]";
    }

    void visit(StatementScript const &stm) const override {
        out_ << "#script (" << stm.type() << ")" << stm.content() << "#end.";
    }

    void visit(StatementInclude const &stm) const override {
        if (stm.type() == IncludeType::inbuild) {
            out_ << "#include <" << stm.path() << ">.";
        } else {
            out_ << "#include ";
            Util::print_quoted(out_, stm.path());
            out_ << ".";
        }
    }

    void visit(StatementProgram const &stm) const override {
        out_ << "#program " << stm.name();
        if (!stm.arguments().empty()) {
            out_ << "(";
            print_range(stm.arguments());
            out_ << ")";
        }
        out_ << ".";
    }

    void visit(StatementConst const &stm) const override {
        out_ << "#const " << stm.name() << "=" << stm.value() << ". [" << stm.type() << "]";
    }

  private:
    std::ostream &out_;
};

} // namespace

auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream & {
    Print{out}(term);
    return out;
}

auto operator<<(std::ostream &out, Literal const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, HeadLiteral const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, BodyLiteral const &lit) -> std::ostream & {
    Print{out}(lit);
    return out;
}

auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream & {
    stm.accept(PrintVisitor{out});
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

auto to_string(Literal const &lit) -> std::string {
    std::ostringstream oss;
    oss << lit;
    return oss.str();
}

auto to_string(HeadLiteral const &lit) -> std::string {
    std::ostringstream out;
    out << lit;
    return out.str();
}

auto to_string(BodyLiteral const &lit) -> std::string {
    std::ostringstream out;
    out << lit;
    return out.str();
}

auto to_string(Statement const &stm) -> std::string {
    std::ostringstream out;
    out << stm;
    return out.str();
}

} // namespace Gringo::Input
