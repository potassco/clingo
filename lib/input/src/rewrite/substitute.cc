#include "transform.hh"
#include "visit.hh"

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/simplify.hh>
#include <clingo/input/rewrite/substitute.hh>
#include <clingo/input/rewrite/visit_variables.hh>

namespace CppClingo::Input {

namespace {

[[nodiscard]] auto is_identifier(Term const &term) -> bool {
    if (auto const *fun = std::get_if<TermFunction>(&term); fun != nullptr) {
        return !fun->external() && fun->pool().size() == 1 && fun->pool().front().elems().empty();
    }
    if (auto const *sym = std::get_if<TermSymbol>(&term); sym != nullptr) {
        return sym->value().type() == SymbolType::function && sym->value().args().empty();
    }
    return false;
}

class MapParams : public Transformer<MapParams> {
  public:
    MapParams(RewriteContext &ctx) : ctx_{&ctx} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto var(size_t param) const { return ctx_->store().string_ref("$" + std::to_string(param)); }

    //! Transform a symbol span into a term tuple/theory term array.
    template <bool plain_term>
    [[nodiscard]] auto accept(Location const &loc, SymbolSpan args) const
        -> std::optional<std::variant<SymbolVec, std::conditional_t<plain_term, ArgumentTuple, TheoryTermArray>>> {
        using TermArrayValueType = std::conditional_t<plain_term, Argument, TheoryTerm>;
        using TermSymbolType = std::conditional_t<plain_term, TermSymbol, TheoryTermSymbol>;
        using TermType = std::conditional_t<plain_term, Term, TheoryTerm>;
        using TermArrayType = std::conditional_t<plain_term, ArgumentTuple, TheoryTermArray>;

        std::optional<std::vector<std::variant<TermType, Symbol>>> res_args;
        bool constant = true;
        {
            std::ptrdiff_t i = 0;
            for (auto arg : args) {
                auto res_arg = accept<plain_term>(loc, arg);
                if (res_arg.has_value() || res_args.has_value()) {
                    if (!res_args.has_value()) {
                        res_args.emplace();
                        res_args->reserve(args.size());
                        res_args->insert(res_args->end(), args.begin(), args.begin() + i);
                    }
                    res_args->emplace_back(std::move(res_arg).value_or(arg));
                    if (constant && std::holds_alternative<TermType>(res_args->back())) {
                        constant = false;
                    }
                }
                ++i;
            }
        }
        if (!res_args) {
            return std::nullopt;
        }
        if (constant) {
            auto tuple = SymbolVec{};
            tuple.reserve(res_args->size());
            for (auto &&arg : *res_args) {
                tuple.emplace_back(std::get<Symbol>(arg));
            }
            return tuple;
        }
        auto tuple = std::vector<TermArrayValueType>{};
        tuple.reserve(res_args->size());
        for (auto &&arg : *res_args) {
            tuple.emplace_back(std::visit(
                [&loc]<class T>(T x) -> TermType {
                    if constexpr (std::is_same_v<T, Symbol>) {
                        return TermSymbolType{loc, x};
                    }
                    if constexpr (std::is_same_v<T, TermType>) {
                        return x;
                    }
                },
                std::move(arg)));
        }
        return TermArrayType{std::move(tuple)};
    }

    //! Transform a symbol into a term/theory term.
    template <bool plain_term>
    [[nodiscard]] auto accept(Location const &loc, Symbol const &sym) const
        -> std::optional<std::variant<std::conditional_t<plain_term, Term, TheoryTerm>, Symbol>> {
        using TermType = std::conditional_t<plain_term, Term, TheoryTerm>;
        using TermVariableType = std::conditional_t<plain_term, TermVariable, TheoryTermVariable>;
        switch (sym.type()) {
            case SymbolType::function: {
                if (sym.args().empty()) {
                    if (auto param = ctx_->is_param(sym.name()); param) {
                        return TermVariableType{loc, var(param.value())};
                    }
                    if (auto value = ctx_->is_const(sym.name()); value) {
                        if (sym.has_sign()) {
                            switch (sym.type()) {
                                case SymbolType::function: {
                                    return value->flip_classical_sign();
                                }
                                case SymbolType::number: {
                                    return ctx_->store().num_ref(-value->num());
                                }
                                case SymbolType::inf:
                                case SymbolType::sup:
                                case SymbolType::string:
                                case SymbolType::tuple: {
                                    break;
                                }
                            }
                            if constexpr (plain_term) {
                                // NOTE: this will evaluated as empty pool later
                                return TermUnary{loc, UnaryOperator::minus, TermSymbol{loc, *value}};
                            } else {
                                return TheoryTermFunction{loc, ctx_->store().string_ref("-"),
                                                          Util::make_vec<TheoryTerm>(TheoryTermSymbol{loc, *value})};
                            }
                        }
                        return *value;
                    }
                    break;
                }
                if (auto res_args = accept<plain_term>(loc, sym.args()); res_args) {
                    return std::visit(
                        [this, &loc, &sym]<class T>(T tuple) -> std::variant<TermType, Symbol> {
                            if constexpr (std::is_same_v<T, SymbolVec>) {
                                return ctx_->store().fun_ref(sym.name(), std::move(tuple), sym.has_sign());
                            }
                            if constexpr (plain_term) {
                                if constexpr (std::is_same_v<T, ArgumentTuple>) {
                                    auto ret = Term{TermFunction{
                                        loc, sym.name(), Util::make_vec<ArgumentTuple>(std::move(tuple)), false}};
                                    if (sym.has_sign()) {
                                        ret = TermUnary{loc, UnaryOperator::minus, std::move(ret)};
                                    }
                                    return ret;
                                }
                            } else {
                                if constexpr (std::is_same_v<T, TheoryTermArray>) {
                                    auto ret = TheoryTerm{TheoryTermFunction{loc, sym.name(), {std::move(tuple)}}};
                                    if (sym.has_sign()) {
                                        ret = TheoryTermFunction{loc, ctx_->store().string_ref("-"),
                                                                 Util::make_vec<TheoryTerm>(std::move(ret))};
                                    }
                                    return ret;
                                }
                            }
                        },
                        std::move(res_args).value());
                }
                break;
            }
            case SymbolType::tuple: {
                if (auto res_args = accept<plain_term>(loc, sym.args()); res_args) {
                    return std::visit(
                        [this, &loc]<class T>(T tuple) -> std::variant<TermType, Symbol> {
                            if constexpr (std::is_same_v<T, SymbolVec>) {
                                return ctx_->store().tup_ref(std::move(tuple));
                            }
                            if constexpr (plain_term) {
                                if constexpr (std::is_same_v<T, ArgumentTuple>) {
                                    return TermTuple{loc, Util::make_vec<TupleElement>(std::move(tuple))};
                                }
                            } else {
                                if constexpr (std::is_same_v<T, TheoryTermArray>) {
                                    return TheoryTermTuple{loc, TheoryTermTupleType::tuple, std::move(tuple)};
                                }
                            }
                        },
                        std::move(res_args).value());
                }
                break;
            }
            case SymbolType::inf:
            case SymbolType::sup:
            case SymbolType::number:
            case SymbolType::string: {
                break;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] auto accept(TermSymbol const &term) const -> std::optional<Term> {
        auto sym = accept<true>(term.loc(), term.value());
        if (sym.has_value()) {
            return std::visit(
                [&term]<class T>(T const &x) -> Term {
                    if constexpr (std::is_same_v<T, Symbol>) {
                        return term.update(a_value = x);
                    }
                    if constexpr (std::is_same_v<T, Term>) {
                        return x;
                    }
                },
                sym.value());
        }
        return std::nullopt;
    }

    [[nodiscard]] auto accept(TermFunction const &term) const -> std::optional<Term> {
        if (term.pool().size() != 1) {
            throw std::runtime_error("unpool has to be called before substituting parameters");
        }
        if (!term.pool().front().elems().empty() || term.external()) {
            return rewrite(term, a_pool);
        }
        if (auto param = ctx_->is_param(term.name()); param) {
            return TermVariable{term.loc(), var(param.value())};
        }
        if (auto value = ctx_->is_const(term.name()); value) {
            return TermSymbol{term.loc(), *value};
        }
        return std::nullopt;
    }

    // theory

    template <bool HasSign>
    [[nodiscard]] auto accept(TheoryAtom<HasSign> const &atom) const
        -> std::optional<std::conditional_t<HasSign, BdLit, HdLit>> {
        if (!is_identifier(atom.name())) {
            return rewrite(atom, a_name, a_elems, a_rhs);
        }
        return rewrite(atom, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        if (term.args().empty()) {
            if (auto param = ctx_->is_param(term.name()); param) {
                return TheoryTermVariable{term.loc(), var(param.value())};
            }
            if (auto value = ctx_->is_const(term.name()); value) {
                return TheoryTermSymbol{term.loc(), *value};
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] auto accept(TheoryTermSymbol const &term) const -> std::optional<TheoryTerm> {
        auto sym = accept<false>(term.loc(), term.value());
        if (sym.has_value()) {
            return std::visit(
                [&term]<class T>(T const &x) -> TheoryTerm {
                    if constexpr (std::is_same_v<T, Symbol>) {
                        return term.update(a_value = x);
                    }
                    if constexpr (std::is_same_v<T, TheoryTerm>) {
                        return x;
                    }
                },
                sym.value());
        }
        return std::nullopt;
    }

    // literal

    [[nodiscard]] auto accept(LitSymbolic const &lit) const -> std::optional<Lit> {
        if (!is_identifier(lit.term())) {
            return rewrite(lit, a_term);
        }
        return std::nullopt;
    }

    // statement

    [[nodiscard]] auto accept(StmProject const &stm) const -> std::optional<Stm> {
        if (!is_identifier(stm.atom())) {
            return rewrite(stm, a_atom, a_body);
        }
        return rewrite(stm, a_body);
    }

    [[nodiscard]] auto accept(StmExternal const &stm) const -> std::optional<Stm> {
        if (!is_identifier(stm.atom())) {
            return rewrite(stm, a_atom, a_body, a_type);
        }
        return rewrite(stm, a_body, a_type);
    }

    [[nodiscard]] auto accept(StmHeuristic const &stm) const -> std::optional<Stm> {
        if (!is_identifier(stm.atom())) {
            return rewrite(stm, a_atom, a_body, a_weight, a_prio, a_type);
        }
        return rewrite(stm, a_body, a_weight, a_prio, a_type);
    }

  private:
    RewriteContext *ctx_;
};

class UnmapParams : public Transformer<UnmapParams> {
  public:
    UnmapParams(SymbolStore &store, ParamUnmap const &map) : store_{&store}, map_{&map} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const = delete;

    // term

    [[nodiscard]] auto accept(TermVariable const &term) const -> std::optional<Term> {
        if (auto it = map_->find(term.name()); it != map_->end()) {
            return TermSymbol{term.loc(), store_->fun_ref(*it.value(), {}, false)};
        }
        return std::nullopt;
    }

    // theory

    [[nodiscard]] static auto accept([[maybe_unused]] TheoryTerm const &term) -> std::optional<TheoryTerm> {
        return std::nullopt;
    }

  private:
    SymbolStore *store_;
    ParamUnmap const *map_;
};

struct Collect : public Visitor<Collect> {
  public:
    Collect(StringSet &ids) : ids_{&ids} {}

    // protect ourselves -> no unintended overloads

    template <class T> void accept(T const &x) const = delete;

    // term

    void accept(Symbol const &sym) const {
        switch (sym.type()) {
            case SymbolType::function: {
                if (sym.args().empty()) {
                    ids_->emplace(sym.name());
                } else {
                    std::for_each(sym.args().begin(), sym.args().end(), *this);
                }
                break;
            }
            case SymbolType::tuple: {
                std::for_each(sym.args().begin(), sym.args().end(), *this);
                break;
            }
            case SymbolType::inf:
            case SymbolType::sup:
            case SymbolType::number:
            case SymbolType::string: {
                break;
            }
        }
    }

    void accept(TermSymbol const &term) const { visit(term.value()); }

    void accept(TermFunction const &term) const {
        if (term.pool().size() != 1) {
            throw std::runtime_error("unpool has to be called before substituting parameters");
        }
        if (term.pool().front().elems().empty() && !term.external()) {
            ids_->emplace(term.name());
        } else {
            visit(term.pool());
        }
    }

    // theory

    static void accept([[maybe_unused]] TheoryTerm const &term) {}

    // literal

    void accept(LitSymbolic const &lit) const {
        if (!is_identifier(lit.term())) {
            visit(lit.term());
        }
    }

    // statement

    void accept(StmProject const &stm) const {
        if (!is_identifier(stm.atom())) {
            visit(stm.atom());
        }
        visit(stm.body());
    }

    void accept(StmExternal const &stm) const {
        if (!is_identifier(stm.atom())) {
            visit(stm.atom());
        }
        visit(stm.body(), stm.type());
    }

    void accept(StmHeuristic const &stm) const {
        if (!is_identifier(stm.atom())) {
            visit(stm.atom());
        }
        visit(stm.body(), stm.weight(), stm.prio(), stm.type());
    }

  private:
    StringSet *ids_;
};

class AssignmentSubstituter : public Transformer<AssignmentSubstituter> {
  public:
    AssignmentSubstituter(Util::unordered_map<String, Term> const &rep) : rep_{&rep} {}
    [[nodiscard]] auto accept(TermVariable const &term) const -> std::optional<Term> {
        if (auto it = rep_->find(term.name()); it != rep_->end()) {
            return it->second;
        }
        return std::nullopt;
    }

  private:
    Util::unordered_map<String, Term> const *rep_;
};

class AssignmentRemover : public Transformer<AssignmentRemover> {
  public:
    AssignmentRemover(Util::unordered_map<String, Term> &rep, StringSet *blocked = nullptr)
        : blocked_{blocked}, rep_{&rep} {}

    template <class T>
        requires Util::matches<T, HdLitTheoryAtom, HdLitSetAggregate, HdLitAggregate, HdLitDisjunction>
    [[nodiscard]] static auto accept([[maybe_unused]] T const &lit) -> std::optional<HdLit> {
        return std::nullopt;
    }

    template <class T>
        requires Util::matches<T, BdLitTheoryAtom, BdLitSetAggregate, BdLitAggregate, BdLitConjunction>
    [[nodiscard]] static auto accept([[maybe_unused]] T const &lit) -> std::optional<BdLit> {
        return std::nullopt;
    }

    [[nodiscard]] auto accept(LitComparison const &lit) const -> std::optional<Lit> {
        assert(lit.sign() == Sign::none && lit.rhs().size() == 1);
        auto const &[rel, rhs] = lit.rhs().front();
        if (auto const *var = get_if<TermVariable>(&lit.lhs());
            var != nullptr && rel == Relation::equal && is_matchable(rhs) && check(var->name(), rhs)) {
            rep_->emplace(var->name(), rhs);
            return LitBool{lit.loc(), Sign::none, true};
        }
        return std::nullopt;
    }

  private:
    [[nodiscard]] auto check(String name, Term const &rhs) const -> bool {
        bool status = !rep_->contains(name) && (blocked_ == nullptr || !blocked_->contains(name));
        auto visit = [&, this](auto const &term) {
            if (status) {
                visit_variables(term, [&, this]([[maybe_unused]] auto const &loc, String var) {
                    if (name == var || rep_->contains(var)) {
                        status = false;
                    }
                });
            }
        };
        visit(rhs);
        for (auto const &[var, term] : *rep_) {
            visit(term);
        }
        return status;
    }
    StringSet *blocked_;
    Util::unordered_map<String, Term> *rep_;
};

class BlockedTheoryVars : public Visitor<BlockedTheoryVars> {
  public:
    BlockedTheoryVars(StringSet &vars) : vars_{&vars} {}
    template <bool HasSign> void accept(TheoryAtom<HasSign> const &atom) const {
        if (auto const &rhs = atom.rhs()) {
            visit_variables(rhs->term(),
                            [&]([[maybe_unused]] Location const &loc, String const &var) { vars_->emplace(var); });
        }
    }

  private:
    StringSet *vars_;
};

class LocalRemover : public Transformer<LocalRemover> {
  public:
    LocalRemover(StringSet &blocked) : blocked_{&blocked} {}

    template <class T>
        requires Util::matches<T, HdLitDisjunctionElement, HdLitAggregateElement, BdLitAggregateElement,
                               SetAggregateElement, CondLit>
    [[nodiscard]] auto accept(T const &elem) const -> std::optional<T> {
        Util::unordered_map<String, Term> rep;
        if (auto elems_rem = AssignmentRemover{rep, blocked_}.transform(elem)) {
            if (auto elems_sub = AssignmentSubstituter{rep}.transform(*elems_rem)) {
                return elems_sub;
            }
            return elems_rem;
        }
        return std::nullopt;
    }

    template <class T>
        requires Util::matches<T, TheoryElement>
    [[nodiscard]] auto accept(T const &elem) const -> std::optional<T> {
        Util::unordered_map<String, Term> rep;
        // NOTE: protect theory term variables because they cannot be
        // substituted by arbitrary terms
        auto blocked = *blocked_;
        for (auto const &term : elem.tuple()) {
            visit_variables(term,
                            [&]([[maybe_unused]] Location const &loc, String const &var) { blocked.emplace(var); });
        }
        if (auto elems_rem = AssignmentRemover{rep, &blocked}.transform(elem)) {
            if (auto elems_sub = AssignmentSubstituter{rep}.transform(*elems_rem)) {
                return elems_sub;
            }
            return elems_rem;
        }
        return std::nullopt;
    }

    StringSet *blocked_;
};

auto substitute_one(RewriteContext &ctx, Stm const &stm) -> SimplifyResult<Stm> {
    auto res_sub = std::optional<Stm>{};
    Util::unordered_map<String, Term> rep;
    // NOTE: protect theory term variables because they cannot be
    // substituted by arbitrary terms
    StringSet blocked;
    BlockedTheoryVars{blocked}.visit(stm);
    if (auto rem = AssignmentRemover{rep, &blocked}.transform(stm)) {
        if (auto sub = AssignmentSubstituter{rep}.transform(*rem)) {
            res_sub = std::move(sub);
        } else {
            res_sub = std::move(rem);
        }
        // Note that it might happen in (constructed) statements that a global
        // variable becomes local during rewriting. Such statements are discarded
        // here.
        if (res_sub && !check_global(ctx.logger(), select_variables(stm, VariableContext::global), *res_sub)) {
            ctx.set_error();
        }
    } else {
        VariableSet global = select_variables(stm, VariableContext::global);
        if (auto rem = LocalRemover{global}.transform(stm)) {
            if (auto sub = AssignmentSubstituter{rep}.transform(*rem)) {
                res_sub = std::move(sub);
            } else {
                res_sub = std::move(rem);
            }
        }
    }
    auto res_smp = SimplifyResult<Stm>{TruthValue::unknown};
    if (res_sub) {
        res_smp = simplify(ctx, *res_sub);
        if (!res_smp.value) {
            res_smp.value = *std::move(res_sub);
        }
    }
    return res_smp;
}

} // namespace

[[nodiscard]] auto map_params(RewriteContext &ctx, Term const &term) -> std::optional<Term> {
    if (ctx.has_params()) {
        return MapParams{ctx}.transform(term);
    }
    return std::nullopt;
}

[[nodiscard]] auto map_params(RewriteContext &ctx, Lit const &lit) -> std::optional<Lit> {
    if (ctx.has_params()) {
        return MapParams{ctx}.transform(lit);
    }
    return std::nullopt;
}

[[nodiscard]] auto map_params(RewriteContext &ctx, HdLit const &lit) -> std::optional<HdLit> {
    if (ctx.has_params()) {
        return MapParams{ctx}.transform(lit);
    }
    return std::nullopt;
}

[[nodiscard]] auto map_params(RewriteContext &ctx, BdLit const &lit) -> std::optional<BdLit> {
    if (ctx.has_params()) {
        return MapParams{ctx}.transform(lit);
    }
    return std::nullopt;
}

[[nodiscard]] auto map_params(RewriteContext &ctx, Stm const &stm) -> std::optional<Stm> {
    if (ctx.has_params()) {
        return MapParams{ctx}.transform(stm);
    }
    return std::nullopt;
}

[[nodiscard]] auto map_params(RewriteContext &ctx, Location const &loc, Symbol const &sym)
    -> std::variant<Symbol, Stm> {
    if (!ctx.has_params() || (sym.type() == SymbolType::function && sym.args().empty())) {
        return sym;
    }
    if (auto res_sym = MapParams{ctx}.accept<true>(loc, sym); res_sym) {
        return std::visit(
            [&loc]<class T>(T x) -> std::variant<Symbol, Stm> {
                if constexpr (std::is_same_v<T, Symbol>) {
                    return x;
                }
                if constexpr (std::is_same_v<T, Term>) {
                    return StmRule{loc, HdLitSimple{LitSymbolic{loc, Sign::none, std::move(x)}},
                                   Util::make_vec<BdLit>()};
                }
            },
            std::move(res_sym).value());
    }
    return sym;
}

[[nodiscard]] auto unmap_params(SymbolStore &store, ParamUnmap const &map, Stm const &stm) -> std::optional<Stm> {
    if (!map.empty()) {
        return UnmapParams{store, map}.transform(stm);
    }
    return std::nullopt;
}

void collect_ids(Symbol const &sym, StringSet &ids) {
    if (sym.type() != SymbolType::function || !sym.args().empty()) {
        Collect{ids}(sym);
    }
}

void collect_ids(Stm const &stm, StringSet &ids) {
    Collect{ids}(stm);
}

auto substitute(RewriteContext &ctx, Stm const &stm) -> Util::ResultState<Stm, TruthValue> {
    auto res = substitute_one(ctx, stm);
    while (res.value) {
        if (auto next = substitute_one(ctx, *res.value); next.value) {
            res = std::move(next);
        } else {
            break;
        }
    }
    return res;
}

} // namespace CppClingo::Input
