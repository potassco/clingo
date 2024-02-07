#include <gringo/util/algorithm.hh>
#include <gringo/util/optional.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/unpool_relations.hh>
#include <gringo/input/algo/visit_variables.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct NegateLiteral {
    auto operator()(Literal const &lit) const -> Literal { return std::visit(*this, lit); }
    auto operator()(LiteralBoolean const &lit) const -> Literal {
        return LiteralBoolean{lit.loc_, lit.sign_, !lit.value_};
    }
    auto operator()(LiteralRelation const &lit) const -> Literal {
        if (lit.rhs_.size() == 1) {
            auto const &[rel, rhs] = lit.rhs_.front();
            return LiteralRelation{lit.loc_, lit.sign_, lit.lhs_, Util::make_vec<Guard>(Guard{complement(rel), rhs})};
        }
        return LiteralRelation{lit.loc_, lit.sign_ + Sign::once, lit.lhs_, lit.rhs_};
    }
    auto operator()(LiteralSymbolic const &lit) const -> Literal {
        return LiteralSymbolic{lit.loc_, lit.sign_ + Sign::once, lit.term_};
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
    if (auto const *sym = std::get_if<LiteralSymbolic>(&lit); sym != nullptr && sym->sign_ != Sign::none) {
        append_conjunctive(lits, negate ? NegateLiteral{}(*sym) : Literal{*sym});
        return LiteralBoolean{location(lit), Sign::none, !negate};
    }
    return std::nullopt;
}

auto shift(TheoryElementVec const &elems) -> Util::ResultVec<TheoryElement> {
    auto res_elems = Util::ResultVec{elems};
    for (auto const &elem : elems) {
        if (auto res_cond = unpool_conjunctive(elem.cond_); res_cond) {
            res_elems.replace(TheoryElement{elem.loc_, elem.tuple_, std::move(res_cond).value()});
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
        return shift(lit.lit_, body, true);
    }

    //! This also shifts disjunctions with non-atomic literals into the rule body.
    //!
    //! For example, the rule
    //!
    //!   #true : p(X) :- q(X).
    //!
    //! is equivalent to
    //!
    //!   #false :- q(X); #false: p(X).
    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteral> {
        auto res_elems = Util::ResultVec{lit.elems_};
        for (auto const &elem : lit.elems_) {
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
                        auto res_cond = unpool_conjunctive(x.cond_);
                        auto res_lit = shift(x.lit_, res_cond, false);
                        if (res_lit || res_cond) {
                            auto clit = ConditionalLiteral{x.loc_, std::move(res_lit).value_or(x.lit_),
                                                           std::move(res_cond).value()};
                            if (auto *blit = std::get_if<LiteralBoolean>(&clit.lit_); blit != nullptr) {
                                res_elems.remove();
                                assert(blit->sign_ == Sign::none);
                                if (blit->value_) {
                                    blit->value_ = false;
                                    body.append(Conjunction{std::move(clit)});
                                }
                            } else {
                                res_elems.replace(std::move(clit));
                            }

                        } else if (auto *blit = std::get_if<LiteralBoolean>(&x.lit_); blit != nullptr) {
                            res_elems.remove();
                            assert(blit->sign_ == Sign::none);
                            if (blit->value_) {
                                body.append(Conjunction{ConditionalLiteral{x.loc_, NegateLiteral{}(*blit), x.cond_}});
                            }
                        } else {
                            res_elems.keep();
                        }
                    }
                },
                elem);
        }
        if (res_elems.value().empty()) {
            return SimpleHeadLiteral{LiteralBoolean{lit.loc_, Sign::none, false}};
        }
        if (res_elems) {
            return Disjunction{lit.loc_, std::move(res_elems).value()};
        }
        return std::nullopt;
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        auto res_elems = Util::ResultVec{lit.elems_};
        for (auto const &elem : lit.elems_) {
            auto res_cond = unpool_conjunctive(elem.cond_);
            auto res_lit = shift(elem.lit_, res_cond, false);
            if (res_lit || res_cond) {
                res_elems.replace(HeadAggregate::Element{elem.loc_, elem.tuple_, std::move(res_lit).value_or(elem.lit_),
                                                         std::move(res_cond).value()});
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            return HeadAggregate{lit.loc_, lit.lhs_, lit.fun_, *std::move(res_elems), lit.rhs_};
        }
        return std::nullopt;
    }

    auto operator()(HeadTheoryAtom const &atom) const -> std::optional<HeadLiteral> {
        if (auto res_elems = shift(atom.elems_); res_elems) {
            return HeadTheoryAtom{atom.loc_, atom.name_, *std::move(res_elems), atom.rhs_};
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
        if (auto res = shift(lit.lit_, body, false); res) {
            body.remove();
        } else {
            body.keep();
        }
    }

    void operator()(Conjunction const &lit) const {
        auto res_cond = unpool_conjunctive(lit.lit_.cond_);
        auto res_lit = shift(lit.lit_.lit_, res_cond, true);
        if (res_lit || res_cond) {
            body.replace(ConditionalLiteral{lit.lit_.loc_, std::move(res_lit).value_or(lit.lit_.lit_),
                                            std::move(res_cond).value()});
        } else {
            body.keep();
        }
    }

    void operator()(BodySetAggregate const &lit) const {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    void operator()(BodyAggregate const &lit) const {
        auto res_elems = Util::ResultVec{lit.elems_};
        for (auto const &elem : lit.elems_) {
            if (auto res_cond = unpool_conjunctive(elem.cond_); res_cond) {
                res_elems.replace(BodyAggregate::Element{elem.loc_, elem.tuple_, *std::move(res_cond)});
            } else {
                res_elems.keep();
            }
        }
        bool assign_lhs = lit.lhs_ && lit.lhs_->second == Relation::equal;
        bool assign_rhs = lit.rhs_ && lit.rhs_->first == Relation::equal;
        bool has_assign = assign_lhs || assign_rhs;
        if (lit.sign_ == Sign::none && has_assign && lit.rhs_) {
            body.remove();
            if (lit.lhs_) {
                body.append(BodyAggregate{lit.loc_, lit.sign_, lit.lhs_, lit.fun_, *res_elems, std::nullopt});
            }
            body.append(BodyAggregate{lit.loc_, lit.sign_, std::make_pair(lit.rhs_->second, lit.rhs_->first), lit.fun_,
                                      *std::move(res_elems), std::nullopt});
        } else if (res_elems) {
            body.replace(BodyAggregate{lit.loc_, lit.sign_, lit.lhs_, lit.fun_, *std::move(res_elems), lit.rhs_});
        } else {
            body.keep();
        }
    }

    void operator()(BodyTheoryAtom const &atom) const {
        if (auto res_elems = shift(atom.elems_); res_elems) {
            body.replace(BodyTheoryAtom{atom.loc_, atom.sign_, atom.name_, *std::move(res_elems), atom.rhs_});
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
    return Util::transform_vec(unpool_crossproduct(lits, unpool), [](auto vec) { return LiteralVec{std::move(vec)}; });
}

struct UnpoolHeadBody {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<std::vector<T>> = delete;

    // aggregate

    template <bool head> using HBLitVecVec = std::conditional_t<head, HeadLiteralVec, std::vector<BodyLiteral>>;

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &lit) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(lit);
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    // theory

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const -> std::optional<HBLitVecVec<!HasSign>> {
        auto unpool_elem = [](TheoryElement const &elem) {
            auto build = [&elem](auto lits) -> TheoryElement { return {elem.loc_, elem.tuple_, std::move(lits)}; };
            return unpool_crossproducts(build, unpool_disjunctive, elem.cond_);
        };
        auto res_elems = unpool_union(atom.elems_, unpool_elem);
        if (res_elems) {
            if constexpr (HasSign) {
                return Util::make_vec<BodyLiteral>(
                    TheoryAtom<HasSign>{atom.loc_, atom.sign_, atom.name_, *std::move(res_elems), atom.rhs_});
            } else {
                return Util::make_vec<HeadLiteral>(
                    TheoryAtom<HasSign>{atom.loc_, atom.name_, *std::move(res_elems), atom.rhs_});
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
                            return ConditionalLiteral{elem.loc_, elem.lit_, std::move(lits)};
                        };
                        return unpool_crossproducts(build, unpool_disjunctive, elem.cond_);
                    }
                },
                elem);
        };
        auto res_elems = unpool_union(lit.elems_, unpool_elem);
        if (res_elems) {
            return Util::make_vec<HeadLiteral>(Disjunction{lit.loc_, std::move(res_elems).value()});
        }
        return std::nullopt;
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        auto unpool_elem = [](HeadAggregate::Element const &elem) {
            auto build = [&elem](auto cond) {
                return HeadAggregate::Element{elem.loc_, elem.tuple_, elem.lit_, std::move(cond)};
            };
            return unpool_crossproducts(build, unpool_disjunctive, elem.cond_);
        };
        auto res_elems = unpool_union(lit.elems_, unpool_elem);
        if (res_elems) {
            return Util::make_vec<HeadLiteral>(
                HeadAggregate{lit.loc_, lit.lhs_, lit.fun_, *std::move(res_elems), lit.rhs_});
        }
        return std::nullopt;
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<std::vector<BodyLiteral>> {
        return std::visit(*this, lit);
    }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<std::vector<BodyLiteral>> {
        auto build = [](auto lit) -> BodyLiteral { return SimpleBodyLiteral{std::move(lit)}; };
        auto unpool = [](auto const &lit) { return unpool_relations(lit, false); };
        return unpool_crossproducts(build, unpool, lit.lit_);
    }

    auto operator()(Conjunction const &lit) const -> std::optional<std::vector<BodyLiteral>> {
        auto build = [&lit](auto cond) -> BodyLiteral {
            return Conjunction{ConditionalLiteral{lit.lit_.loc_, lit.lit_.lit_, std::move(cond)}};
        };
        return unpool_crossproducts(build, unpool_disjunctive, lit.lit_.cond_);
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<std::vector<BodyLiteral>> {
        auto unpool_elem = [](BodyAggregate::Element const &elem) {
            auto build = [&elem](auto cond) { return BodyAggregate::Element{elem.loc_, elem.tuple_, std::move(cond)}; };
            return unpool_crossproducts(build, unpool_disjunctive, elem.cond_);
        };
        auto res_elems = unpool_union(lit.elems_, unpool_elem);
        if (res_elems) {
            return Util::make_vec<BodyLiteral>(
                BodyAggregate{lit.loc_, lit.sign_, lit.lhs_, lit.fun_, *std::move(res_elems), lit.rhs_});
        }
        return std::nullopt;
    }

    auto operator()(std::vector<BodyLiteral> const &body) const -> std::optional<std::vector<BodyLiteralVec>> {
        return Util::transform_vec(unpool_crossproduct(body, *this),
                                   [](auto vec) { return BodyLiteralVec{std::move(vec)}; });
    }

    auto operator()(BodyLiteralVec const &body) const -> std::optional<std::vector<BodyLiteralVec>> {
        return operator()(body.vector());
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
        auto body = shift_body(stm.body_);
        auto head = ShiftHead{body}(stm.head_);
        auto build = [&stm](auto head, auto body) -> Statement {
            return Rule{stm.loc_, std::move(head), std::move(body)};
        };
        auto unpool = UnpoolHeadBody{ctx};
        if (head || body) {
            auto shifted = Rule{stm.loc_, std::move(head).value_or(stm.head_), std::move(body).value()};
            if (auto res = unpool_crossproducts(build, unpool, shifted.head_, shifted.body_); res) {
                return res;
            }
            return Util::make_vec<Statement>(std::move(shifted));
        }
        return unpool_crossproducts(build, unpool, stm.head_, stm.body_);
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
            return StatementWeakConstraint{stm.loc_, std::move(body), stm.tuple_};
        };
        return rewrite_with_body(build, stm.body_);
    }

    auto operator()(StatementShow const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementShow{stm.loc_, stm.term_, std::move(body)}; };
        return rewrite_with_body(build, stm.body_);
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementProject{stm.loc_, stm.term_, std::move(body)}; };
        return rewrite_with_body(build, stm.body_);
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
            return StatementExternal{stm.loc_, stm.term_, std::move(body), stm.type_};
        };
        return rewrite_with_body(build, stm.body_);
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement { return StatementEdge{stm.loc_, stm.edges_, std::move(body)}; };
        return rewrite_with_body(build, stm.body_);
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<StatementVec> {
        auto build = [&stm](auto body) -> Statement {
            return StatementHeuristic{stm.loc_, stm.atom_, std::move(body), stm.type_, stm.prio_, stm.mod_};
        };
        return rewrite_with_body(build, stm.body_);
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
    if (rel != nullptr && rel->rhs_.size() > 1 && conjunctive == (rel->sign_ != Sign::once)) {
        auto const *lhs = &rel->lhs_;
        std::vector<Literal> res;
        for (auto const &rhs : rel->rhs_) {
            auto cmp = rel->sign_ != Sign::once ? rhs.first : complement(rhs.first);
            res.emplace_back(
                LiteralRelation{rel->loc_, Sign::none, *lhs, Util::make_vec<Guard>(Guard{cmp, rhs.second})});
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
            if (!check_global(ctx.logger(), global, unpooled)) {
                return StatementVec{};
            }
        }
    }
    return stms;
}

} // namespace Gringo::Input
