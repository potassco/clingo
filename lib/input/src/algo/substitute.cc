#include "transform.hh"
#include "visit.hh"

#include <gringo/input/algo/substitute.hh>

namespace Gringo::Input {

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

[[nodiscard]] auto variable_for_param(RewriteContext &ctx, Location const &loc, size_t param) {
    return TermVariable{loc, ctx.store().string("$" + std::to_string(param))};
}

class MapParams : public Transformer<MapParams> {
  public:
    MapParams(RewriteContext &ctx) : ctx_{&ctx} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(Location const &loc,
                              SymbolSpan args) const -> std::optional<std::variant<SymbolRefVec, ArgumentTuple>> {
        std::optional<std::vector<std::variant<Term, SymbolRef>>> res_args;
        bool constant = true;
        {
            ssize_t i = 0;
            for (auto arg : args) {
                auto res_arg = accept(loc, arg);
                if (res_arg.has_value() || res_args.has_value()) {
                    if (!res_args.has_value()) {
                        res_args.emplace();
                        res_args->reserve(args.size());
                        res_args->insert(res_args->end(), args.begin(), args.begin() + i);
                    }
                    res_args->emplace_back(std::move(res_arg).value_or(arg));
                    if (constant && std::holds_alternative<Term>(res_args->back())) {
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
            auto tuple = SymbolRefVec{};
            tuple.reserve(res_args->size());
            for (auto &&arg : *res_args) {
                tuple.emplace_back(std::get<SymbolRef>(arg));
            }
            return tuple;
        }
        auto tuple = std::vector<Argument>{};
        tuple.reserve(res_args->size());
        for (auto &&arg : *res_args) {
            tuple.emplace_back(std::visit(
                [&loc]<class T>(T x) -> Term {
                    if constexpr (std::is_same_v<T, SymbolRef>) {
                        return TermSymbol{loc, x};
                    }
                    if constexpr (std::is_same_v<T, Term>) {
                        return x;
                    }
                },
                std::move(arg)));
        }
        return ArgumentTuple{std::move(tuple)};
    }

    [[nodiscard]] auto accept(Location const &loc,
                              SymbolRef const &sym) const -> std::optional<std::variant<Term, SymbolRef>> {
        switch (sym.type()) {
            case SymbolType::function: {
                if (sym.args().empty()) {
                    if (auto param = ctx_->is_param(sym.name()); param) {
                        return variable_for_param(*ctx_, loc, param.value());
                    }
                    if (auto value = ctx_->is_const(sym.name()); value) {
                        if (sym.has_sign()) {
                            switch (sym.type()) {
                                case SymbolType::function: {
                                    return value->flip_classical_sign();
                                }
                                case SymbolType::number: {
                                    return ctx_->store().num(-*value->num());
                                }
                                case SymbolType::inf:
                                case SymbolType::sup:
                                case SymbolType::string:
                                case SymbolType::tuple: {
                                    break;
                                }
                            }
                            // Note: this will evaluated as empty pool later
                            return TermUnary{loc, UnaryOperator::negate, TermSymbol{loc, *value}};
                        }
                        return value;
                    }
                    break;
                }
                if (auto res_args = accept(loc, sym.args()); res_args) {
                    return std::visit(
                        [this, &loc, &sym]<class T>(T tuple) -> std::variant<Term, SymbolRef> {
                            if constexpr (std::is_same_v<T, SymbolRefVec>) {
                                return ctx_->store().fun(sym.name(), std::move(tuple), sym.has_sign());
                            }
                            if constexpr (std::is_same_v<T, ArgumentTuple>) {
                                auto ret = Term{TermFunction{loc, sym.name(), {std::move(tuple)}, false}};
                                if (sym.has_sign()) {
                                    ret = TermUnary{loc, UnaryOperator::negate, std::move(ret)};
                                }
                                return ret;
                            }
                        },
                        std::move(res_args).value());
                }
                break;
            }
            case SymbolType::tuple: {
                if (auto res_args = accept(loc, sym.args()); res_args) {
                    return std::visit(
                        [this, &loc]<class T>(T tuple) -> std::variant<Term, SymbolRef> {
                            if constexpr (std::is_same_v<T, SymbolRefVec>) {
                                return ctx_->store().tup(std::move(tuple));
                            }
                            if constexpr (std::is_same_v<T, ArgumentTuple>) {
                                return TermTuple{loc, Util::make_vec<TupleElement>(std::move(tuple))};
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
        auto sym = accept(term.loc(), term.value());
        if (sym.has_value()) {
            return std::visit(
                [&term]<class T>(T const &x) -> Term {
                    if constexpr (std::is_same_v<T, SymbolRef>) {
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
            return TermVariable{term.loc(), ctx_->store().string("$" + std::to_string(param.value()))};
        }
        if (auto value = ctx_->is_const(term.name()); value) {
            return TermSymbol{term.loc(), value.value()};
        }
        return std::nullopt;
    }

    // theory

    [[nodiscard]] static auto accept([[maybe_unused]] TheoryTerm const &term) -> std::optional<TheoryTerm> {
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
    UnmapParams(SymbolStore &store, Util::ordered_map<StringRef, StringRef> const &map) : store_{&store}, map_{&map} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const = delete;

    // term

    [[nodiscard]] auto accept(TermVariable const &term) const -> std::optional<Term> {
        if (auto it = map_->find(term.name()); it != map_->end()) {
            return TermSymbol{term.loc(), store_->fun(it.value(), {}, false)};
        }
        return std::nullopt;
    }

    // theory

    [[nodiscard]] static auto accept([[maybe_unused]] TheoryTerm const &term) -> std::optional<TheoryTerm> {
        return std::nullopt;
    }

  private:
    SymbolStore *store_;
    Util::ordered_map<StringRef, StringRef> const *map_;
};

struct Collect : public Visitor<Collect> {
  public:
    Collect(StringRefSet &ids) : ids_{&ids} {}

    // protect ourselves -> no unintended overloads

    template <class T> void accept(T const &x) const = delete;

    // term

    void accept(SymbolRef const &sym) const {
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
    StringRefSet *ids_;
};

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

[[nodiscard]] auto map_params(RewriteContext &ctx, Location const &loc,
                              SymbolRef const &sym) -> std::variant<SymbolRef, Stm> {
    if (!ctx.has_params() || (sym.type() == SymbolType::function && sym.args().empty())) {
        return sym;
    }
    if (auto res_sym = MapParams{ctx}.accept(loc, sym); res_sym) {
        return std::visit(
            [&loc]<class T>(T x) -> std::variant<SymbolRef, Stm> {
                if constexpr (std::is_same_v<T, SymbolRef>) {
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

[[nodiscard]] auto unmap_params(SymbolStore &store, Util::ordered_map<StringRef, StringRef> const &map,
                                Stm const &stm) -> std::optional<Stm> {
    if (!map.empty()) {
        return UnmapParams{store, map}.transform(stm);
    }
    return std::nullopt;
}

void collect_ids(SymbolRef const &sym, StringRefSet &ids) {
    if (sym.type() != SymbolType::function || !sym.args().empty()) {
        Collect{ids}(sym);
    }
}

void collect_ids(Stm const &stm, StringRefSet &ids) { Collect{ids}(stm); }

} // namespace Gringo::Input
