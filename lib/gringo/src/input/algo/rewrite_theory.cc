#include "transform.hh"

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/rewrite_theory.hh>

namespace Gringo::Input {

enum class Arity {
    unary = 0,
    binary = 1,
};

enum class Associativity {
    left = 0,
    right = 1,
    non_assoc = 2,
};

class UnparsedTermParser {
  public:
    using Table = Util::unordered_map<std::pair<String, Arity>, std::pair<int, Associativity>>;
    using Stack = std::vector<std::pair<String, Arity>>;
    using Terms = std::vector<TheoryTerm>;

    void add(TheoryOpDefinition const &def) {
        auto arity = Arity::binary;
        auto assoc = Associativity::non_assoc;
        if (def.type() == TheoryOpType::binary_left) {
            assoc = Associativity::left;
        } else if (def.type() == TheoryOpType::binary_right) {
            assoc = Associativity::right;
        } else {
            arity = Arity::unary;
        }
        table_.try_emplace(std::make_pair(def.op(), arity), def.prio(), assoc);
    }

    //! Check if the given operator is in the parse table raising a runtime error if absent.
    void check_operator(Logger &log, String op, Arity arity, Location loc) const {
        if (!table_.contains(std::make_pair(op, arity))) {
            GRINGO_REPORT_LOC(log, error, loc) << "cannot parse operator `" << op << "`";
        }
    }

    //! Parses the given unparsed term, replacing it by nested theory functions.
    auto parse(Logger &log, TheoryTermUnparsed const &term) const -> TheoryTerm {
        stack_.clear();
        terms_.clear();

        auto arity = Arity::unary;
        for (auto const &elem : term.elems()) {
            for (auto const &op : elem.first) {
                check_operator(log, op, arity, term.loc());

                while (arity == Arity::binary && check_(op)) {
                    reduce_();
                }

                stack_.emplace_back(std::make_pair(op, arity));
                arity = Arity::unary;
            }

            terms_.emplace_back(elem.second);
            arity = Arity::binary;
        }

        while (!stack_.empty()) {
            reduce_();
        }

        return std::move(terms_.back());
    }

  private:
    //! Get priority and associativity of the given binary operator.
    auto priority_and_associativity_(String op) const -> std::pair<int, Associativity> {
        if (auto it = table_.find(std::make_pair(op, Arity::binary)); it != table_.end()) {
            return it->second;
        }
        return {0, Associativity::left};
    }

    //! Get priority of the given unary or binary operator.
    auto priority_(String op, Arity arity) const -> int {
        if (auto it = table_.find(std::make_pair(op, arity)); it != table_.end()) {
            return it->second.first;
        }
        return 0;
    }

    //! Returns true if the stack has to be reduced.
    //!
    //! Returns true if the priority of the given binary operator is lower than the preceeding operator on the stack.
    auto check_(String op) const -> bool {
        if (stack_.empty()) {
            return false;
        }
        auto [priority, associativity] = priority_and_associativity_(op);
        auto previous_priority = priority_(stack_.back().first, stack_.back().second);
        return previous_priority > priority || (previous_priority == priority && associativity == Associativity::left);
    }

    //! Combines the last unary or binary term on the stack.
    void reduce_() const {
        auto b = std::move(terms_.back());
        auto loc = location(b);
        terms_.pop_back();
        auto [op, arity] = std::move(stack_.back());
        stack_.pop_back();
        if (arity == Arity::unary) {
            terms_.emplace_back(TheoryTermFunction{loc, op, Util::make_immutable_array<TheoryTerm>(std::move(b))});
        } else {
            auto a = std::move(terms_.back());
            loc = location(a) + loc;
            terms_.pop_back();
            terms_.emplace_back(
                TheoryTermFunction{loc, op, Util::make_immutable_array<TheoryTerm>(std::move(a), std::move(b))});
        }
    }

    Table table_;
    mutable Terms terms_;
    mutable Stack stack_;
};

//! Parser for theory terms.
class ParseTheoryTerm : public Transformer<ParseTheoryTerm> {
  public:
    ParseTheoryTerm(Logger &log, UnparsedTermParser &parser) : log_{log}, parser_{parser} {}

    [[nodiscard]] auto accept(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        auto arity = std::optional<Arity>{};
        if (term.args().size() == 1) {
            arity = Arity::unary;
        } else if (term.args().size() == 2) {
            arity = Arity::binary;
        }
        if (arity && is_theory_operator(term.name().view())) {
            parser_.check_operator(log_, term.name(), *arity, term.loc());
        }

        return rewrite(term, a_args);
    }

    [[nodiscard]] auto accept(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return parser_.parse(log_, term);
    }

  private:
    Logger &log_;
    UnparsedTermParser &parser_;
};

//! Parser for theory atoms.
class ParseTheory : public Transformer<ParseTheory> {
  public:
    using Table = Util::unordered_map<
        std::pair<String, int>,
        std::tuple<TheoryAtomType, UnparsedTermParser, std::optional<std::pair<StringSet, UnparsedTermParser>>>>;
    /*
    _table: Mapping[
        Tuple[str, int],
        Tuple[
            TheoryAtomType,
            TheoryTermParser,
            Optional[Tuple[Set[str], TheoryTermParser]],
        ],
    ]
    _in_body: bool
    _in_head: bool
    _is_directive: bool

    def __init__(
        self,
        terms: Mapping[str, Union[OperatorTable, TheoryTermParser]],
        atoms: AtomTable,
    ):
        self._reset()

        term_parsers = {}
        for term_key, parser in terms.items():
            if isinstance(parser, TheoryTermParser):
                term_parsers[term_key] = parser
            else:
                term_parsers[term_key] = TheoryTermParser(parser)

        self._table = {}
        for atom_key, (atom_type, term_key, guard) in atoms.items():
            guard_table = None
            if guard is not None:
                guard_table = (set(guard[0]), term_parsers[guard[1]])
            self._table[atom_key] = (atom_type, term_parsers[term_key], guard_table)

    def _reset(self, in_head=True, in_body=True, is_directive=True):
        """
        Set state information about active scope.
        """
        self._in_head = in_head
        self._in_body = in_body
        self._is_directive = is_directive

    def _visit_body(self, x: AST) -> AST:
        try:
            self._reset(False, True, False)
            old = x.body
            new = self.visit_sequence(old)
            return x if new is old else x.update(body=new)
        finally:
            self._reset()

    def visit_Rule(self, x: AST) -> AST:
        """
        Parse theory atoms in body and head.

        Parameters
        ----------
        x
            The AST to rewrite.

        Returns
        -------
        The rewritten AST.
        """
        ret = self._visit_body(x)
        try:
            self._reset(True, False, not x.body)
            head = self(x.head)
            if head is not x.head:
                if ret is x:
                    ret = copy(ret)
                ret.head = head
        finally:
            self._reset()

        return ret

    def visit_ShowTerm(self, x: AST) -> AST:
        """
        Parse theory atoms in body.

        Parameters
        ----------
        x
            The AST to rewrite.

        Returns
        -------
        The rewritten AST.
        """
        return self._visit_body(x)

    def visit_Minimize(self, x: AST) -> AST:
        """
        Parse theory atoms in body.

        Parameters
        ----------
        x
            The AST to rewrite.

        Returns
        -------
        The rewritten AST.
        """
        return self._visit_body(x)

    def visit_Edge(self, x: AST) -> AST:
        """
        Parse theory atoms in body.

        Parameters
        ----------
        x
            The AST to rewrite.

        Returns
        -------
        The rewritten AST.
        """
        return self._visit_body(x)

    def visit_Heuristic(self, x: AST) -> AST:
        """
        Parse theory atoms in body.

        Parameters
        ----------
        x
            The AST to rewrite.

        Returns
        -------
        The rewritten AST.
        """
        return self._visit_body(x)

    def visit_ProjectAtom(self, x: AST) -> AST:
        """
        Parse theory atoms in body.

        Parameters
        ----------
        x
            The AST to rewrite.

        Returns
        -------
        The rewritten AST.
        """
        return self._visit_body(x)

    def visit_TheoryAtom(self, x: AST) -> AST:
        """
        Parse the given theory atom.

        Parameters
        ----------
        x
            The AST to rewrite.

        Returns
        -------
        The rewritten AST.
        """
        name = x.term.name
        arity = len(x.term.arguments)
        if (name, arity) not in self._table:
            raise RuntimeError(
                f"theory atom definiton not found: {location_to_str(x.location)}"
            )

        type_, element_parser, guard_table = self._table[(name, arity)]
        if type_ == TheoryAtomType.Head and not self._in_head:
            raise RuntimeError(
                f"theory atom only accepted in head: {location_to_str(x.location)}"
            )
        if type_ == TheoryAtomType.Body and not self._in_body:
            raise RuntimeError(
                f"theory atom only accepted in body: {location_to_str(x.location)}"
            )
        if type_ == TheoryAtomType.Directive and not (
            self._in_head and self._is_directive
        ):
            raise RuntimeError(
                f"theory atom must be a directive: {location_to_str(x.location)}"
            )

        x = copy(x)
        x.term = element_parser(x.term)
        x.elements = element_parser.visit_sequence(x.elements)

        if x.guard is not None:
            if guard_table is None:
                raise RuntimeError(
                    f"unexpected guard in theory atom: {location_to_str(x.location)}"
                )

            guards, guard_parser = guard_table
            if x.guard.operator_name not in guards:
                raise RuntimeError(
                    f"unexpected guard in theory atom: {location_to_str(x.location)}"
                )

            x.guard = copy(x.guard)
            x.guard.term = guard_parser(x.guard.term)

        return x
        */
};

class TheoryParser {
  public:
    void add_theory(StmTheory const &stm) {
        static_cast<void>(this);
        for (auto const &term_def : stm.term_defs()) {
            static_cast<void>(term_def);
        }
        for (auto const &atom_def : stm.atom_defs()) {
            static_cast<void>(atom_def);
        }
    }
};

// auto rewrite_theory(Theory const &thy, Stm const &stm) -> std::optional<Stm>;

} // namespace Gringo::Input
