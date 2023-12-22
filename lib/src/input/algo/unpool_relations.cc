#include <util/algorithm.hh>
#include <util/optional.hh>

#include <input/algo/analyze.hh>
#include <input/algo/unpool_relations.hh>
#include <input/algo/visit_variables.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct NegateLiteral {
    auto operator()(Literal const &lit) const -> Literal { return std::visit(*this, lit); }
    auto operator()(LiteralBoolean const &lit) const -> Literal {
        return LiteralBoolean{lit.loc, lit.sign, !lit.value};
    }
    auto operator()(LiteralRelation const &lit) const -> Literal {
        if (lit.rhs.size() == 1) {
            auto const &[rel, rhs] = lit.rhs.front();
            return LiteralRelation{lit.loc, lit.sign, lit.lhs, Util::make_vec<Guard>(Guard{complement(rel), rhs})};
        }
        return LiteralRelation{lit.loc, lit.sign + Sign::once, lit.lhs, lit.rhs};
    }
    auto operator()(LiteralSymbolic const &lit) const -> Literal {
        return LiteralSymbolic{lit.loc, lit.sign + Sign::once, lit.term};
    }
};

auto unpool_conjunctive(LiteralVec const &lits) -> Util::ResultVec<Literal> {
    auto res_lits = Util::ResultVec{lits};
    for (auto const &lit : lits) {
        if (auto res = unpool_relations(lit, true); res) {
            res_lits.remove();
            res_lits.extend(std::make_move_iterator(res->begin()), std::make_move_iterator(res->end()));
        } else {
            res_lits.keep();
        }
    }
    return res_lits;
}

void append_conjunctive(auto &lits, Literal lit) {
    if (auto res = unpool_relations(lit, true); res) {
        lits.extend(std::make_move_iterator(res->begin()), std::make_move_iterator(res->end()));
    } else {
        lits.append(std::move(lit));
    }
}

auto shift(auto const &lit, auto &lits, bool negate) -> std::optional<Literal> {
    // TODO: there should be something in analyze
    if (auto const *rel = std::get_if<LiteralRelation>(&lit); rel != nullptr) {
        append_conjunctive(lits, negate ? NegateLiteral{}(*rel) : Literal{*rel});
        return LiteralBoolean{location(lit), Sign::none, !negate};
    }
    if (auto const *sym = std::get_if<LiteralSymbolic>(&lit); sym != nullptr && sym->sign != Sign::none) {
        append_conjunctive(lits, negate ? NegateLiteral{}(*sym) : Literal{*sym});
        return LiteralBoolean{location(lit), Sign::none, !negate};
    }
    return std::nullopt;
}

auto shift(TheoryElementVec const &elems) -> Util::ResultVec<TheoryElement> {
    auto res_elems = Util::ResultVec{elems};
    for (auto const &elem : elems) {
        if (auto res_cond = unpool_conjunctive(elem.second); res_cond) {
            res_elems.replace(TheoryElement{elem.first, std::move(res_cond).value()});
        } else {
            res_elems.keep();
        }
    }
    return res_elems;
}

struct ShiftHead {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteral> {
        return shift(lit.lit, body, true);
    }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteral> {
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto const &elem : lit.elems) {
            std::visit(
                [this, &res_elems](auto const &x) {
                    GRINGO_MATCH(x, Literal) {
                        if (auto res_lit = shift(x, body, true); res_lit) {
                            res_elems.remove();
                        } else {
                            res_elems.keep();
                        }
                    }
                    GRINGO_MATCH(x, ConditionalLiteral) {
                        auto res_cond = unpool_conjunctive(x.cond);
                        auto res_lit = shift(x.lit, res_cond, false);
                        if (res_lit || res_cond) {
                            res_elems.replace(ConditionalLiteral{x.loc, std::move(res_lit).value_or(x.lit),
                                                                 std::move(res_cond).value()});
                        } else {
                            res_elems.keep();
                        }
                    }
                },
                elem);
        }
        if (res_elems) {
            return Disjunction{lit.loc, std::move(res_elems).value()};
        }
        return std::nullopt;
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto const &elem : lit.elems) {
            auto res_cond = unpool_conjunctive(elem.cond);
            auto res_lit = shift(elem.lit, res_cond, false);
            if (res_lit || res_cond) {
                res_elems.replace(HeadAggregate::Element{elem.loc, elem.tuple, std::move(res_lit).value_or(elem.lit),
                                                         std::move(res_cond).value()});
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return HeadAggregate{lit.loc, lit.lhs, lit.fun, *std::move(res_elems), lit.rhs};
        }
        return std::nullopt;
    }

    auto operator()(HeadTheoryAtom const &atom) const -> std::optional<HeadLiteral> {
        if (auto res_elems = shift(atom.elems); res_elems) {
            return HeadTheoryAtom{atom.loc, atom.name, *std::move(res_elems), atom.rhs};
        }
        return std::nullopt;
    }

    Util::ResultVec<BodyLiteral> &body;
};

struct ShiftBody {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // head literal

    void operator()(BodyLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(SimpleBodyLiteral const &lit) const {
        if (auto res = shift(lit.lit, body, false); res) {
            body.remove();
        } else {
            body.keep();
        }
    }

    void operator()(Conjunction const &lit) const {
        auto res_cond = unpool_conjunctive(lit.lit.cond);
        auto res_lit = shift(lit.lit.lit, res_cond, true);
        if (res_lit || res_cond) {
            body.replace(
                ConditionalLiteral{lit.lit.loc, std::move(res_lit).value_or(lit.lit.lit), std::move(res_cond).value()});
        } else {
            body.keep();
        }
    }

    void operator()(BodySetAggregate const &lit) const {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    void operator()(BodyAggregate const &lit) const {
        auto res_elems = Util::ResultVec{lit.elems};
        for (auto const &elem : lit.elems) {
            if (auto res_cond = unpool_conjunctive(elem.cond); res_cond) {
                res_elems.replace(BodyAggregate::Element{elem.loc, elem.tuple, *std::move(res_cond)});
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            body.replace(BodyAggregate{lit.loc, lit.sign, lit.lhs, lit.fun, *std::move(res_elems), lit.rhs});
        } else {
            body.keep();
        }
    }

    void operator()(BodyTheoryAtom const &atom) const {
        if (auto res_elems = shift(atom.elems); res_elems) {
            body.replace(BodyTheoryAtom{atom.loc, atom.sign, atom.name, *std::move(res_elems), atom.rhs});
        } else {
            body.keep();
        }
    }

    Util::ResultVec<BodyLiteral> &body;
};

auto shift_body(BodyLiteralVec const &body) -> Util::ResultVec<BodyLiteral> {
    auto res_body = Util::ResultVec{body};
    for (auto const &lit : body) {
        ShiftBody{res_body}(lit);
    }
    return res_body;
}

auto unpool_disjunctive(LiteralVec const &lits) -> std::optional<std::vector<LiteralVec>> {
    auto unpool = [](auto const &lit) { return unpool_relations(lit, false); };
    return unpool_crossproduct(lits, unpool);
}

struct UnpoolHeadBody {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<std::vector<T>> = delete;

    // aggregate

    template <bool head> using HBLitVecVec = std::conditional_t<head, HeadLiteralVec, BodyLiteralVec>;

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &lit) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    // theory

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const -> std::optional<HBLitVecVec<!HasSign>> {
        auto unpool_elem = [](TheoryElement const &elem) {
            auto build = [&elem](auto lits) -> TheoryElement { return {elem.first, std::move(lits)}; };
            return unpool_crossproducts(build, unpool_disjunctive, elem.second);
        };
        auto res_elems = unpool_union(atom.elems, unpool_elem);
        if (res_elems) {
            if constexpr (HasSign) {
                return Util::make_vec<BodyLiteral>(
                    TheoryAtom<HasSign>{atom.loc, atom.sign, atom.name, *std::move(res_elems), atom.rhs});
            } else {
                return Util::make_vec<HeadLiteral>(
                    TheoryAtom<HasSign>{atom.loc, atom.name, *std::move(res_elems), atom.rhs});
            }
        }
        return std::nullopt;
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteralVec> {
        // Note: anything that could be unpooled has been shifted
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteralVec> {
        auto unpool_elem = [](Disjunction::Element const &elem) {
            return std::visit(
                [](auto const &elem) -> std::optional<Disjunction::ElementVec> {
                    GRINGO_MATCH(elem, Literal) { return std::nullopt; }
                    GRINGO_MATCH(elem, ConditionalLiteral) {
                        auto build = [&elem](auto lits) -> Disjunction::Element {
                            return ConditionalLiteral{elem.loc, elem.lit, std::move(lits)};
                        };
                        return unpool_crossproducts(build, unpool_disjunctive, elem.cond);
                    }
                },
                elem);
        };
        auto res_elems = unpool_union(lit.elems, unpool_elem);
        if (res_elems) {
            return Util::make_vec<HeadLiteral>(Disjunction{lit.loc, std::move(res_elems).value()});
        }
        return std::nullopt;
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        auto unpool_elem = [](HeadAggregate::Element const &elem) {
            auto build = [&elem](auto cond) -> HeadAggregate::Element {
                return {elem.loc, elem.tuple, elem.lit, std::move(cond)};
            };
            return unpool_crossproducts(build, unpool_disjunctive, elem.cond);
        };
        auto res_elems = unpool_union(lit.elems, unpool_elem);
        if (res_elems) {
            return Util::make_vec<HeadLiteral>(
                HeadAggregate{lit.loc, lit.lhs, lit.fun, *std::move(res_elems), lit.rhs});
        }
        return std::nullopt;
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<BodyLiteralVec> {
        auto build = [](auto lit) -> BodyLiteral { return SimpleBodyLiteral{std::move(lit)}; };
        auto unpool = [](auto const &lit) { return unpool_relations(lit, false); };
        return unpool_crossproducts(build, unpool, lit.lit);
    }

    auto operator()(Conjunction const &lit) const -> std::optional<BodyLiteralVec> {
        auto build = [&lit](auto cond) -> BodyLiteral {
            return ConditionalLiteral{lit.lit.loc, lit.lit.lit, std::move(cond)};
        };
        return unpool_crossproducts(build, unpool_disjunctive, lit.lit.cond);
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteralVec> {
        auto unpool_elem = [](BodyAggregate::Element const &elem) {
            auto build = [&elem](auto cond) -> BodyAggregate::Element {
                return {elem.loc, elem.tuple, std::move(cond)};
            };
            return unpool_crossproducts(build, unpool_disjunctive, elem.cond);
        };
        auto res_elems = unpool_union(lit.elems, unpool_elem);
        if (res_elems) {
            return Util::make_vec<BodyLiteral>(
                BodyAggregate{lit.loc, lit.sign, lit.lhs, lit.fun, *std::move(res_elems), lit.rhs});
        }
        return std::nullopt;
    }

    auto operator()(BodyLiteralVec const &body) const -> std::optional<std::vector<BodyLiteralVec>> {
        return unpool_crossproduct(body, *this);
    }

    RewriteContext const &ctx;
};

struct UnpoolStatement {
    template <class F>
    auto rewrite_with_body(F &&build, BodyLiteralVec const &body) const -> std::optional<StatementVec> {
        auto unpool = UnpoolHeadBody{ctx};
        if (auto res_body = shift_body(body); res_body) {
            if (auto res = unpool_crossproducts(build, unpool, res_body.value()); res) {
                return res;
            }
            return Util::make_vec<Statement>(build(std::move(res_body).value()));
        }
        return unpool_crossproducts(build, unpool, body);
    }

    auto operator()(Statement const &stm) const -> std::optional<StatementVec> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> std::optional<StatementVec> {
        auto body = shift_body(stm.body);
        auto head = ShiftHead{body}(stm.head);
        auto build = [&stm](auto head, auto body) -> Statement {
            return Rule{stm.loc, std::move(head), std::move(body)};
        };
        auto unpool = UnpoolHeadBody{ctx};
        if (head || body) {
            auto shifted = Rule{stm.loc, std::move(head).value_or(stm.head), std::move(body).value()};
            if (auto res = unpool_crossproducts(build, unpool, shifted.head, shifted.body); res) {
                return res;
            }
            return Util::make_vec<Statement>(std::move(shifted));
        }
        return unpool_crossproducts(build, unpool, stm.head, stm.body);
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::runtime_error("unpool must be called before unpooling relations");
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement {
            return StatementWeakConstraint{stm.loc, std::move(body), stm.tuple};
        };
        return rewrite_with_body(build, stm.body);
    }

    auto operator()(StatementShow const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementShow{stm.loc, stm.term, std::move(body)}; };
        return rewrite_with_body(build, stm.body);
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementProject{stm.loc, stm.term, std::move(body)}; };
        return rewrite_with_body(build, stm.body);
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement {
            return StatementExternal{stm.loc, stm.term, std::move(body), stm.type};
        };
        return rewrite_with_body(build, stm.body);
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementEdge{stm.loc, stm.edges, std::move(body)}; };
        return rewrite_with_body(build, stm.body);
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement {
            return StatementHeuristic{stm.loc, stm.atom, std::move(body), stm.type, stm.prio, stm.mod};
        };
        return rewrite_with_body(build, stm.body);
    }

    auto operator()(StatementScript const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementConst const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(Comment const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    RewriteContext const &ctx;
};

} // namespace

[[nodiscard]] auto unpool_relations(Literal const &lit, bool conjunctive) -> std::optional<LiteralVec> {
    auto const *rel = std::get_if<LiteralRelation>(&lit);
    if (rel != nullptr && rel->rhs.size() > 1 && conjunctive == (rel->sign != Sign::once)) {
        auto const *lhs = &rel->lhs;
        LiteralVec res;
        for (auto const &rhs : rel->rhs) {
            auto cmp = rel->sign != Sign::once ? rhs.first : complement(rhs.first);
            res.emplace_back(
                LiteralRelation{rel->loc, Sign::none, *lhs, Util::make_vec<Guard>(Guard{cmp, rhs.second})});
            lhs = &rhs.second;
        }
        return res;
    }
    return std::nullopt;
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec> {
    auto stms = UnpoolStatement{ctx}(stm);
    if (stms) {
        VariableSet global = select_variables(stm, VariableContext::global);
        for (auto const &unpooled : stms.value()) {
            check_global(ctx.logger(), global, unpooled);
        }
    }
    return stms;
}

} // namespace Gringo::Input
