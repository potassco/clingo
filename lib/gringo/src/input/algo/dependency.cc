#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/dependency.hh>

#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

namespace Gringo::Input {

using Signature = std::tuple<String, size_t, bool>;
using Dependency = std::tuple<Stm const *, Term const *, bool>;

using DependencyMap = Util::unordered_map<Signature, std::vector<Dependency>>;

enum class DependencyType : uint32_t {
    positive = 1,
    negative = 2,
};
GRINGO_ENUM_FLAGS(DependencyType)

//! Builder for the dependencies between statements.
struct AddDepend {

    // TODO:
    // - too verbose, unnecessary information for grounding
    // - restrict to essential dependencies for grounding
    // - tag statements as normal/not normal to detect to positive program (parts)

    void operator()(LitSymbolic const &lit, DependencyType type) const {
        auto const &term = lit.term();
        auto sig = signature(term).value();
        if (test(type, DependencyType::positive) && lit.sign() == Sign::none) {
            map[sig].emplace_back(&stm, &term, false);
        }
        if (test(type, DependencyType::negative) || lit.sign() != Sign::none) {
            map[sig].emplace_back(&stm, &term, true);
        }
    }

    template <class T>
        requires Util::is_among_v<T, LitComparison, LitSymbolic>
    void operator()([[maybe_unused]] T const &lit, [[maybe_unused]] DependencyType type) const {}

    void operator()(Lit const &lit, DependencyType type) const {
        std::visit([type, this](auto const &lit) { operator()(lit, type); }, lit);
    }

    void operator()(LitArray const &lits, DependencyType type) const {
        for (auto const &lit : lits) {
            operator()(lit, type);
        }
    }

    void operator()([[maybe_unused]] HdLitSimple const &lit) const {
        // Note: we can ignore the literal here because all negative literals
        // have been shifted to the body and positive literals do not induce a
        // dependency.
    }

    void operator()(HdLitDisjunction const &lit) const {
        // TODO: think about self dependency or tag
        for (auto const &elem : lit.elems()) {
            std::visit(
                [this]<class T>(T const &lit) {
                    if constexpr (Util::matches<T, Lit>) {
                        operator()(lit, DependencyType::negative);
                    } else {
                        operator()(lit.lit(), DependencyType::negative);
                        operator()(lit.cond(), DependencyType::positive | DependencyType::negative);
                    }
                },
                elem);
        }
    }

    template <class T>
        requires Util::is_among_v<T, HdLitSetAggregate, HdLitAggregate, HdLitTheoryAtom>
    void operator()(HdLitSetAggregate const &lit) const {
        // TODO: think about self dependency or tag
        for (auto const &elem : lit.elems()) {
            if constexpr (Util::is_among_v<T, HdLitSetAggregate, HdLitArray>) {
                operator()(elem.lit(), DependencyType::negative);
            }
            operator()(elem.cond(), DependencyType::positive | DependencyType::negative);
        }
    }

    void operator()(HdLit const &lit) const { std::visit(*this, lit); }

    void operator()(BdLitSimple const &lit) const { operator()(lit.lit(), DependencyType::positive); }

    void operator()(BdLitConjunction const &lit) const {
        operator()(lit.lit().lit(), DependencyType::positive);
        operator()(lit.lit().cond(), DependencyType::positive | DependencyType::negative);
    }

    void operator()(BdLitSetAggregate const &lit) const {
        auto type = DependencyType::positive;
        if (!reduct_is_monotone(lit.lhs(), AggregateFunction::count, lit.rhs())) {
            type |= DependencyType::negative;
        }
        for (auto const &elem : lit.elems()) {
            operator()(elem.lit(), type);
            operator()(elem.cond(), type);
        }
    }

    void operator()(BdLitAggregate const &lit) const {
        auto type = DependencyType::positive;
        if (!reduct_is_monotone(lit.lhs(), lit.fun(), lit.rhs())) {
            type |= DependencyType::negative;
        }
        for (auto const &elem : lit.elems()) {
            operator()(elem.cond(), type);
        }
    }

    void operator()(BdLitTheoryAtom const &lit) const {
        // TODO: think about self dependency
        for (auto const &elem : lit.elems()) {
            operator()(elem.cond(), DependencyType::positive | DependencyType::negative);
        }
    }

    void operator()(BdLit const &lit) const {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    template <class T>
        requires Util::is_among_v<T, StmTheory, StmOptimize, StmWeakConstraint, StmShow, StmShowSig, StmProject,
                                  StmProjectSig, StmDefined, StmExternal, StmEdge, StmHeuristic, StmScript, StmInclude,
                                  StmProgram, StmConst, StmComment>
    void operator()(T const &stm) const {
        static_cast<void>(stm);
        throw std::runtime_error("implement me!!!");
    }

    void operator()(StmRule const &stm) const {
        operator()(stm.head());
        std::for_each(stm.body().begin(), stm.body().end(), *this);
    }

    Stm const &stm;
    DependencyMap &map;
};

struct DependencyGraph {
    void add(std::vector<Stm> const &stms) {
        // add dependencies to the dependency map
        for (auto const &stm : stms) {
            std::visit(AddDepend{stm, map_}, stm);
        }
        // build the dependency graph
        for (auto const &stm : stms) {
            static_cast<void>(stm);
            throw std::logic_error("build dependencies using provided atoms");
        }
    }
    DependencyMap map_;
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
    gph.add(stms);
    throw std::runtime_error("implement me!!!");
}

} // namespace Gringo::Input
