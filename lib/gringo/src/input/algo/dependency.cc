#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/dependency.hh>

#include <gringo/util/unordered_map.hh>

namespace Gringo::Input {

struct DependencyGraph {
    void add(Stm const &stm, std::vector<Term> provide, std::vector<std::pair<Term, bool>> depend) {
        for (auto &lit : depend) {
            depend_[signature(lit.first).value()].emplace_back(lit, stm);
        }
        // Note: the head dependency should be computed on the fly and only the statement be stored here.
        // This way, the vector of statements can be reused.
        // Furthermore, references to terms can be used here because the statements are not touched in any way.
        stms_.emplace_back(stm, std::move(provide));
    }
    Util::unordered_map<std::tuple<String, size_t, bool>, std::vector<std::pair<std::pair<Term, bool>, Stm const &>>>
        depend_;
    std::vector<std::pair<Stm const &, std::vector<Term>>> stms_;
};

//! Builder for the dependencies between statements.
struct DependencyBuilder {

    auto operator()(HdLit const &lit) -> std::vector<Term> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    auto operator()(BdLitArray const &lit) -> std::vector<std::pair<Term, bool>> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    //! Add dependencies for the given rule.
    void operator()(Stm const &stm) {
        if (auto const *rule = std::get_if<StmRule>(&stm); rule != nullptr) {
            graph.add(stm, operator()(rule->head()), operator()(rule->body()));
        } else {
            throw std::runtime_error("implement me!!!");
        }
    }

    DependencyGraph &graph;
};

/*
class DependencyBuilder:
    """
    Builder for the dependencies between predicates given by rules.
    """

    def __init__(self):
        self.graph = PredicateGraph()

    def _get_pred(self, lit: Literal):
        if isinstance(lit, ast.LiteralSymbolic):
            atom = lit.atom
            if isinstance(atom, ast.TermSymbolic):
                symbol = atom.symbol
                return [(symbol.name, symbol.arity, symbol.sign)]

            sign = False
            if isinstance(atom, ast.TermUnaryOperation):
                sign = True
                atom = atom.right
            assert isinstance(atom, ast.TermFunction)

            return [(atom.name, len(atom.pool[0].arguments), sign)]
        return []

    def _get_body_pred(self, lit: Literal, force_negative=False):
        res = [(pred, lit.sign != ast.Sign.NoSign) for pred in self._get_pred(lit)]
        if force_negative:
            res += [(pred, True) for pred, sign in res if not sign]
        return res

    @singledispatchmethod
    def _head(self, lit) -> list[Predicate]:
        _ = lit
        return []

    @_head.register
    def _(self, lit: ast.HeadSimpleLiteral) -> list[Predicate]:
        if isinstance(lit.literal, ast.LiteralSymbolic):
            return self._get_pred(lit.literal)
        return []

    @_head.register(ast.HeadDisjunction)
    @_head.register(ast.HeadAggregate)
    def _(self, lit: Union[ast.HeadDisjunction, ast.HeadAggregate]) -> list[Predicate]:
        # Note: that here edges are added that only involve the conditionals in
        # the head. It would also be possible to add the predicates to the
        # result. Then, one could even change the interface to get the
        # predicates a rule provides/depends.
        res = []
        for elem in lit.elements:
            if isinstance(elem, (ast.HeadConditionalLiteral, ast.HeadAggregateElement)):
                head_preds = self._get_pred(elem.literal)
                for slit in elem.condition:
                    for body_pred, sign in self._body(slit):
                        for head_pred in head_preds:
                            self.graph.add_edge(head_pred, body_pred, sign)
            else:
                head_preds = self._get_pred(elem)
            for head_pred in head_preds:
                self.graph.add_edge(head_pred, head_pred, True)
            res.extend(head_preds)
        return res

    @singledispatchmethod
    def _body(self, lit: BodyLiteral) -> list[tuple[Predicate, bool]]:
        _ = lit
        return []

    @_body.register
    def _(self, lit: ast.BodySimpleLiteral) -> list[tuple[Predicate, bool]]:
        return self._get_body_pred(lit.literal)

    @_body.register
    def _(self, lit: ast.BodyTheoryAtom) -> list[tuple[Predicate, bool]]:
        res = []
        for elem in lit.elements:
            for slit in elem.condition:
                res.extend(self._get_body_pred(slit, True))
        return res

    @_body.register
    def _(self, lit: ast.BodyConditionalLiteral) -> list[tuple[Predicate, bool]]:
        res = self._get_body_pred(lit.literal)
        for slit in lit.condition:
            res.extend(self._get_body_pred(slit, True))
        return res

    def _is_monotone(
        self,
        left: Optional[ast.LeftGuard],
        fun: ast.AggregateFunction,
        right: Optional[ast.RightGuard],
    ) -> bool:
        if fun != ast.AggregateFunction.Sum:
            rel_left = (ast.Relation.Less, ast.Relation.LessEqual)
            rel_right = (ast.Relation.Greater, ast.Relation.GreaterEqual)
            if fun == ast.AggregateFunction.Min:
                rel_left, rel_right = rel_right, rel_left
            return (not left or left.relation in rel_left) and (
                not right or right.relation in rel_right
            )
        return False

    @_body.register
    def _(self, lit: ast.BodyAggregate) -> list[tuple[Predicate, bool]]:
        res = []
        force_negative = not self._is_monotone(lit.left, lit.function, lit.right)
        for elem in lit.elements:
            for slit in elem.condition:
                res.extend(self._get_body_pred(slit, force_negative))
        return res

    def add(self, stm: ast.StatementRule):
        """
        Add dependencies for the given rule.
        """
        head_preds = self._head(stm.head)
        for lit in stm.body:
            for head_pred in head_preds:
                for body_pred, sign in self._body(lit):
                    self.graph.add_edge(head_pred, body_pred, sign)
*/

auto analyze(std::vector<Stm> const &stms) -> Components {
    auto gph = DependencyGraph{};
    auto bld = DependencyBuilder{gph};
    std::for_each(stms.begin(), stms.end(), bld);
    throw std::runtime_error("implement me!!!");
}

} // namespace Gringo::Input
