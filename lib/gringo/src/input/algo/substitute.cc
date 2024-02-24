#include <gringo/input/program.hh>

#include "transform.hh"
#include "visit.hh"

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

struct MapParams : Transformer<MapParams> {

    MapParams(RewriteContext &ctx) : ctx{ctx} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(Location const &loc, SymbolSpan args) const
        -> std::optional<std::variant<SymbolVec, ArgumentTuple>> {
        std::optional<std::vector<std::variant<Term, Symbol>>> res_args;
        bool constant = true;
        {
            size_t i = 0;
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
            auto tuple = SymbolVec{};
            tuple.reserve(res_args->size());
            for (auto &&arg : *res_args) {
                tuple.emplace_back(std::get<Symbol>(arg));
            }
            return tuple;
        }
        auto tuple = std::vector<Argument>{};
        tuple.reserve(res_args->size());
        for (auto &&arg : *res_args) {
            tuple.emplace_back(std::visit(
                [&loc](auto &&x) -> Term {
                    GRINGO_MATCH(x, Symbol) { return TermSymbol{loc, x}; }
                    GRINGO_MATCH(x, Term) { return x; }
                },
                std::move(arg)));
        }
        return ArgumentTuple{std::move(tuple)};
    }

    [[nodiscard]] auto accept(Location const &loc, Symbol const &sym) const
        -> std::optional<std::variant<Term, Symbol>> {
        switch (sym.type()) {
            case SymbolType::function: {
                if (sym.args().empty()) {
                    if (auto param = ctx.is_param(sym.name()); param) {
                        return variable_for_param(ctx, loc, param.value());
                    }
                    if (auto value = ctx.is_const(sym.name()); value) {
                        if (sym.has_sign()) {
                            switch (sym.type()) {
                                case SymbolType::function: {
                                    return value->flip_classical_sign();
                                }
                                case SymbolType::number: {
                                    return ctx.store().num(-*value->num());
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
                        [this, &loc, &sym](auto &&tuple) -> std::variant<Term, Symbol> {
                            GRINGO_MATCH(tuple, SymbolVec) {
                                return ctx.store().fun(sym.name(), std::move(tuple), sym.has_sign());
                            }
                            GRINGO_MATCH(tuple, ArgumentTuple) {
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
                        [this, &loc](auto &&tuple) -> std::variant<Term, Symbol> {
                            GRINGO_MATCH(tuple, SymbolVec) { return ctx.store().tup(std::move(tuple)); }
                            GRINGO_MATCH(tuple, ArgumentTuple) {
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
                [&term](auto &&x) -> Term {
                    GRINGO_MATCH(x, Symbol) { return TermSymbol{term.loc(), x}; }
                    GRINGO_MATCH(x, Term) { return x; }
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
            return transform_construct2(term, a_pool);
        }
        if (auto param = ctx.is_param(term.name()); param) {
            return TermVariable{term.loc(), ctx.store().string("$" + std::to_string(param.value()))};
        }
        if (auto value = ctx.is_const(term.name()); value) {
            return TermSymbol{term.loc(), value.value()};
        }
        return std::nullopt;
    }

    // theory

    [[nodiscard]] static auto accept(TheoryTerm const &term) -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // literal

    [[nodiscard]] auto accept(LitSymbolic const &lit) const -> std::optional<Lit> {
        if (!is_identifier(lit.term())) {
            return transform_construct2(lit, a_term);
        }
        return std::nullopt;
    }

    // statement

    [[nodiscard]] auto accept(StmProject const &stm) const -> std::optional<Stm> {
        if (!is_identifier(stm.term())) {
            return transform_construct2(stm, a_term, a_body);
        }
        return transform_construct2(stm, a_body);
    }

    [[nodiscard]] auto accept(StmExternal const &stm) const -> std::optional<Stm> {
        if (!is_identifier(stm.term())) {
            return transform_construct2(stm, a_term, a_body, a_type);
        }
        return transform_construct2(stm, a_body, a_type);
    }

    [[nodiscard]] auto accept(StmHeuristic const &stm) const -> std::optional<Stm> {
        if (!is_identifier(stm.atom())) {
            return transform_construct2(stm, a_atom, a_body, a_weight, a_prio, a_type);
        }
        return transform_construct2(stm, a_body, a_weight, a_prio, a_type);
    }

    RewriteContext &ctx;
};

struct UnmapParams : Transformer<UnmapParams> {

    UnmapParams(SymbolStore &store, Util::ordered_map<String, String> const &map) : store{store}, map{map} {}

    // protect ourselves -> no unintended overloads

    // NOLINTNEXTLINE
    template <class T> [[nodiscard]] auto accept(T const &x) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(TermVariable const &term) const -> std::optional<Term> {
        if (auto it = map.find(term.name()); it != map.end()) {
            return TermSymbol{term.loc(), store.fun(it.value(), {}, false)};
        }
        return std::nullopt;
    }

    // theory

    [[nodiscard]] static auto accept(TheoryTerm const &term) -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    SymbolStore &store;
    Util::ordered_map<String, String> const &map;
};

struct Collect : Visitor<Collect> {

    Collect(StringSet &ids) : ids{ids} {}

    // protect ourselves -> no unintended overloads

    template <class T> void accept(T const &x) const = delete;

    // term

    void accept(Symbol const &sym) const {
        switch (sym.type()) {
            case SymbolType::function: {
                if (sym.args().empty()) {
                    ids.emplace(sym.name());
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
            ids.emplace(term.name());
        } else {
            visit(term.pool());
        }
    }

    // theory

    static void accept(TheoryTerm const &term) { static_cast<void>(term); }

    // literal

    void accept(LitSymbolic const &lit) const {
        if (!is_identifier(lit.term())) {
            visit(lit.term());
        }
    }

    // statement

    void accept(StmProject const &stm) const {
        if (!is_identifier(stm.term())) {
            visit(stm.term());
        }
        visit(stm.body());
    }

    void accept(StmExternal const &stm) const {
        if (!is_identifier(stm.term())) {
            visit(stm.term());
        }
        visit(stm.body(), stm.type());
    }

    void accept(StmHeuristic const &stm) const {
        if (!is_identifier(stm.atom())) {
            visit(stm.atom());
        }
        visit(stm.body(), stm.weight(), stm.prio(), stm.type());
    }

    StringSet &ids;
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

[[nodiscard]] auto map_params(RewriteContext &ctx, Location const &loc, Symbol const &sym)
    -> std::variant<Symbol, Stm> {
    if (!ctx.has_params() || (sym.type() == SymbolType::function && sym.args().empty())) {
        return sym;
    }
    if (auto res_sym = MapParams{ctx}.accept(loc, sym); res_sym) {
        return std::visit(
            [&loc](auto &&x) -> std::variant<Symbol, Stm> {
                GRINGO_MATCH(x, Symbol) { return x; }
                GRINGO_MATCH(x, Term) {
                    return StmRule{loc, HdLitSimple{LitSymbolic{loc, Sign::none, std::move(x)}},
                                   Util::make_vec<BdLit>()};
                }
            },
            std::move(res_sym).value());
    }
    return sym;
}

[[nodiscard]] auto unmap_params(SymbolStore &store, Util::ordered_map<String, String> const &map, Stm const &stm)
    -> std::optional<Stm> {
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

void collect_ids(Stm const &stm, StringSet &ids) { Collect{ids}(stm); }

} // namespace Gringo::Input
