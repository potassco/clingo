#include "unpool.hh"

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/unpool_relations.hh>
#include <clingo/input/rewrite/visit_variables.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/optional.hh>
#include <clingo/util/type_traits.hh>

namespace CppClingo::Input {

namespace {

struct NegateLiteral {
    auto operator()(Lit const &lit) const -> Lit { return std::visit(*this, lit); }
    auto operator()(LitBool const &lit) const -> Lit { return lit.update(a_value = !lit.value()); }
    auto operator()(LitComparison const &lit) const -> Lit {
        if (lit.rhs().size() == 1) {
            auto const &[rel, rhs] = lit.rhs().front();
            return lit.update(a_rhs = Util::make_vec<Guard>(Guard{complement(rel), rhs}));
        }
        return lit.update(a_sign = lit.sign() + Sign::once);
    }
    auto operator()(LitSymbolic const &lit) const -> Lit { return lit.update(a_sign = lit.sign() + Sign::once); }
};

auto unpool_conjunctive(LitArray const &lits) {
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

enum class ShiftMode : uint8_t { keep, negate, force };

auto append_conjunctive(auto &lits, Lit const &lit, ShiftMode mode) -> std::optional<Lit> {
    auto res_lit = std::optional<Lit>{};
    if (mode == ShiftMode::force) {
        res_lit = lit;
    } else if (mode == ShiftMode::negate) {
        res_lit = negate(lit);
    }
    if (auto res = unpool_relations(res_lit ? res_lit.value() : lit, true); res) {
        lits.extend(std::make_move_iterator(res->begin()), std::make_move_iterator(res->end()));
    } else if (res_lit) {
        lits.append(*std::move(res_lit));
    } else {
        return std::nullopt;
    }
    return LitBool{location(lit), Sign::none, mode != ShiftMode::negate};
}

auto shift(auto const &lit, auto &lits, ShiftMode mode) -> std::optional<Lit> {
    if (auto const *rel = std::get_if<LitComparison>(&lit); rel != nullptr) {
        return append_conjunctive(lits, lit, mode);
    }
    if (auto const *sym = std::get_if<LitSymbolic>(&lit); sym != nullptr && sym->sign() != Sign::none) {
        return append_conjunctive(lits, lit, mode);
    }
    return std::nullopt;
}

auto shift(TheoryElementArray const &elems) {
    auto res_elems = Util::ResultVec{elems};
    for (auto const &elem : elems) {
        res_elems.update(elem.rewrite(a_cond = unpool_conjunctive(elem.cond())));
    }
    return res_elems;
}

class ShiftHead {
  public:
    explicit ShiftHead(Util::ResultVec<BdLit, false> &body) : body_{&body} {}
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // head literal

    auto operator()(HdLit const &lit) const -> std::optional<HdLit> { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> std::optional<HdLit> {
        return shift(lit.lit(), *body_, ShiftMode::negate);
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
    auto operator()(HdLitDisjunction const &lit) const -> std::optional<HdLit> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            std::visit(
                [this, &res_elems]<class T>(T const &x) {
                    if constexpr (std::is_same_v<T, Lit>) {
                        if (shift(x, *body_, ShiftMode::negate)) {
                            res_elems.remove();
                        } else {
                            res_elems.keep();
                        }
                    }
                    if constexpr (std::is_same_v<T, CondLit>) {
                        auto res_cond = unpool_conjunctive(x.cond());
                        auto res_lit = shift(x.lit(), res_cond, ShiftMode::force);
                        if (const auto *blit = std::get_if<LitBool>(res_lit ? &*res_lit : &x.lit()); blit != nullptr) {
                            res_elems.remove();
                            body_->append(BdLitConjunction{
                                x.update(a_lit = NegateLiteral{}(*blit), a_cond = *std::move(res_cond))});
                        } else {
                            res_elems.update(x.rewrite(a_lit = std::move(res_lit), a_cond = std::move(res_cond)));
                        }
                    }
                },
                elem);
        }
        if (res_elems.value().empty()) {
            return HdLitSimple{LitBool{lit.loc(), Sign::none, false}};
        }
        return lit.rewrite(a_elems = std::move(res_elems));
    }

    auto operator()([[maybe_unused]] HdLitSetAggregate const &lit) const -> std::optional<HdLit> {
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    auto operator()(HdLitAggregate const &lit) const -> std::optional<HdLit> {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            auto res_cond = unpool_conjunctive(elem.cond());
            auto res_lit = shift(elem.lit(), res_cond, ShiftMode::force);
            res_elems.update(elem.rewrite(a_lit = std::move(res_lit), a_cond = std::move(res_cond)));
        }
        return lit.rewrite(a_elems = std::move(res_elems));
    }

    auto operator()(HdLitTheoryAtom const &atom) const -> std::optional<HdLit> {
        return atom.rewrite(a_elems = shift(atom.elems()));
    }

  private:
    Util::ResultVec<BdLit, false> *body_;
};

class ShiftBody {
  public:
    explicit ShiftBody(Util::ResultVec<BdLit, false> &body) : body_{&body} {}

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // head literal

    void operator()(BdLit const &lit) const { std::visit(*this, lit); }

    void operator()(BdLitSimple const &lit) const {
        if (shift(lit.lit(), *body_, ShiftMode::keep)) {
            body_->remove();
        } else {
            body_->keep();
        }
    }

    void operator()(BdLitConjunction const &lit) const {
        auto res_cond = unpool_conjunctive(lit.lit().cond());
        auto res_lit = shift(lit.lit().lit(), res_cond, ShiftMode::negate);
        body_->update(lit.lit().rewrite(a_lit = std::move(res_lit), a_cond = std::move(res_cond)));
    }

    void operator()([[maybe_unused]] BdLitSetAggregate const &lit) const {
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    void operator()(BdLitAggregate const &lit) const {
        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            res_elems.update(elem.rewrite(a_cond = unpool_conjunctive(elem.cond())));
        }
        auto const &lhs = lit.lhs();
        auto const &rhs = lit.rhs();
        bool assign_lhs = lhs && lhs->second == Relation::equal;
        bool assign_rhs = rhs && rhs->first == Relation::equal;
        bool has_assign = assign_lhs || assign_rhs;
        if (lit.sign() == Sign::none && has_assign && rhs) {
            body_->remove();
            if (lit.lhs()) {
                body_->append(lit.update(a_elems = *res_elems, a_rhs = std::nullopt));
            }
            body_->append(lit.update(a_lhs = std::make_pair(rhs->second, flip(rhs->first)),
                                     a_elems = *std::move(res_elems), a_rhs = std::nullopt));
        } else {
            body_->update(lit.rewrite(a_elems = std::move(res_elems)));
        }
    }

    void operator()(BdLitTheoryAtom const &atom) const { body_->update(atom.rewrite(a_elems = shift(atom.elems()))); }

  private:
    Util::ResultVec<BdLit, false> *body_;
};

auto shift_body(BdLitArray const &body) {
    auto res_body = Util::ResultVec{body};
    for (auto const &lit : body) {
        ShiftBody{res_body}(lit);
    }
    return res_body;
}

auto unpool_disjunctive(LitArray const &lits) -> std::optional<std::vector<LitArray>> {
    auto unpool = [](auto const &lit) { return unpool_relations(lit, false); };
    return Util::transform_vec(unpool_crossproduct(lits, unpool), [](auto vec) { return LitArray{std::move(vec)}; });
}

struct UnpoolHeadBody {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<std::vector<T>> = delete;

    // aggregate

    template <bool head> using HBLitVecVec = std::conditional_t<head, HdLitArray, std::vector<BdLit>>;

    template <bool HasSign>
    auto operator()([[maybe_unused]] SetAggregate<HasSign> const &lit) const -> std::optional<HBLitVecVec<!HasSign>> {
        throw std::runtime_error("simplify must be called before unpooling of relations");
    }

    // theory

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const -> std::optional<HBLitVecVec<!HasSign>> {
        auto unpool_elem = [](TheoryElement const &elem) {
            return unpool_rewrite<TheoryElement>(elem, unpool_disjunctive, a_cond);
        };
        if (auto res = atom.rewrite(a_elems = unpool_union(atom.elems(), unpool_elem)); res) {
            return Util::make_vec<std::conditional_t<HasSign, BdLit, HdLit>>(*std::move(res));
        }
        return std::nullopt;
    }

    // head literal

    auto operator()(HdLit const &lit) const -> std::optional<HdLitArray> { return std::visit(*this, lit); }

    auto operator()([[maybe_unused]] HdLitSimple const &lit) const -> std::optional<HdLitArray> {
        // Note: anything that could be unpooled has been shifted
        return std::nullopt;
    }

    auto operator()(HdLitDisjunction const &lit) const -> std::optional<HdLitArray> {
        auto unpool_elem = [](HdLitDisjunctionElement const &elem) {
            return std::visit(
                []<class T>(T const &elem) -> std::optional<HdLitDisjunctionElementArray> {
                    if constexpr (std::is_same_v<T, Lit>) {
                        return std::nullopt;
                    }
                    if constexpr (std::is_same_v<T, CondLit>) {
                        return unpool_rewrite<HdLitDisjunctionElement>(elem, unpool_disjunctive, a_cond);
                    }
                },
                elem);
        };
        if (auto res_elems = unpool_union(lit.elems(), unpool_elem); res_elems) {
            return Util::make_vec<HdLit>(HdLitDisjunction{lit.loc(), std::move(res_elems).value()});
        }
        return std::nullopt;
    }

    auto operator()(HdLitAggregate const &lit) const -> std::optional<HdLitArray> {
        auto unpool_elem = [](HdLitAggregateElement const &elem) {
            return unpool_rewrite<HdLitAggregateElement>(elem, unpool_disjunctive, a_cond);
        };
        if (auto res_elems = unpool_union(lit.elems(), unpool_elem); res_elems) {
            return Util::make_vec<HdLit>(lit.update(a_elems = *std::move(res_elems)));
        }
        return std::nullopt;
    }

    // body literal

    auto operator()(BdLit const &lit) const -> std::optional<std::vector<BdLit>> { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> std::optional<std::vector<BdLit>> {
        auto unpool = [](auto const &lit) { return unpool_relations(lit, false); };
        return unpool_rewrite<BdLit>(lit, unpool, a_lit);
    }

    auto operator()(BdLitConjunction const &lit) const -> std::optional<std::vector<BdLit>> {
        return unpool_rewrite<BdLit>(lit.lit(), unpool_disjunctive, a_cond);
    }

    auto operator()(BdLitAggregate const &lit) const -> std::optional<std::vector<BdLit>> {
        auto unpool_elem = [](BdLitAggregateElement const &elem) {
            return unpool_rewrite<BdLitAggregateElement>(elem, unpool_disjunctive, a_cond);
        };
        if (auto res_elems = unpool_union(lit.elems(), unpool_elem); res_elems) {
            return Util::make_vec<BdLit>(lit.update(a_elems = *std::move(res_elems)));
        }
        return std::nullopt;
    }

    auto operator()(std::span<BdLit const> body) const -> std::optional<std::vector<BdLitArray>> {
        return Util::transform_vec(unpool_crossproduct(body, *this),
                                   [](auto vec) { return BdLitArray{std::move(vec)}; });
    }

    auto operator()(BdLitArray const &body) const -> std::optional<std::vector<BdLitArray>> {
        return operator()(std::span{body});
    }
};

struct UnpoolStatement {
    template <class S> [[nodiscard]] auto rewrite_with_body(S const &stm) const -> std::optional<StmVec> {
        auto build = [&stm](auto body) -> Stm { return stm.update(a_body = std::move(body)); };
        auto unpool = [&build](auto const &body) { return Util::transform_vec(UnpoolHeadBody{}(body), build); };
        if (auto res_body = shift_body(stm.body()); res_body) {
            if (auto res = unpool(*res_body); res) {
                return res;
            }
            return Util::make_vec<Stm>(build(*std::move(res_body)));
        }
        return unpool(stm.body());
    }

    auto operator()(StmRule const &stm) const -> std::optional<StmVec> {
        auto res_body = shift_body(stm.body());
        auto res_head = ShiftHead{res_body}(stm.head());
        auto unpool = [](auto const &stm) { return unpool_rewrite<Stm>(stm, UnpoolHeadBody{}, a_head, a_body); };
        if (auto res_shifted = stm.rewrite(a_head = std::move(res_head), a_body = std::move(res_body)); res_shifted) {
            if (auto res = unpool(*res_shifted); res) {
                return res;
            }
            return Util::make_vec<Stm>(*std::move(res_shifted));
        }
        return unpool(stm);
    }

    auto operator()([[maybe_unused]] StmOptimize const &stm) const -> std::optional<StmVec> {
        throw std::runtime_error("unpool must be called before unpooling relations");
    }

    template <class S> auto operator()([[maybe_unused]] S const &stm) const -> std::optional<StmVec> {
        if constexpr (Util::is_among_v<S, StmWeakConstraint, StmShow, StmProject, StmExternal, StmEdge, StmHeuristic>) {
            return rewrite_with_body(stm);
        } else {
            static_assert(Util::is_among_v<S, StmTheory, StmShowNothing, StmShowSig, StmProjectSig, StmDefined,
                                           StmScript, StmInclude, StmProgram, StmConst, StmParts, StmComment>);
            return std::nullopt;
        }
    }

    auto operator()(Stm const &stm) const -> std::optional<StmVec> { return std::visit(*this, stm); }
};

} // namespace

[[nodiscard]] auto negate(Lit const &lit) -> Lit {
    return NegateLiteral{}(lit);
}

[[nodiscard]] auto unpool_relations(Lit const &lit, bool conjunctive) -> std::optional<LitArray> {
    auto const *rel = std::get_if<LitComparison>(&lit);
    if (rel != nullptr && rel->rhs().size() > 1 && conjunctive == (rel->sign() != Sign::once)) {
        auto const *lhs = &rel->lhs();
        std::vector<Lit> res;
        for (auto const &rhs : rel->rhs()) {
            auto cmp = rel->sign() != Sign::once ? rhs.first : complement(rhs.first);
            res.emplace_back(
                rel->update(a_sign = Sign::none, a_lhs = *lhs, a_rhs = Util::make_vec<Guard>(Guard{cmp, rhs.second})));
            lhs = &rhs.second;
        }
        return res;
    }
    return std::nullopt;
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Stm const &stm) -> std::optional<StmVec> {
    auto stms = UnpoolStatement{}(stm);
    if (stms) {
        VariableSet global = select_variables(stm, VariableContext::global);
        for (auto const &unpooled : stms.value()) {
            if (!check_global(ctx.logger(), global, unpooled)) {
                ctx.set_error();
                return StmVec{};
            }
        }
    }
    return stms;
}

} // namespace CppClingo::Input
