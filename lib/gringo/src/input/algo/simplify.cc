#include <algorithm>
#include <ctime>

#include <gringo/util/algorithm.hh>
#include <gringo/util/checked_math.hh>
#include <gringo/util/optional.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/evaluate.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/simplify.hh>
#include <gringo/input/algo/visit_variables.hh>

namespace Gringo::Input {

namespace {

//! Extend the contained vector with the given assignements.
template <class T> void extend(Util::ResultVec<T> &res, AuxTermVec &aux, bool conjunctive = true) {
    for (auto &[lhs, rhs] : aux) {
        auto loc = location(lhs);
        auto rel = conjunctive ? Relation::equal : Relation::inequal;
        auto lit = LiteralRelation{loc, Sign::none, std::move(lhs), Util::make_vec<Guard>(Guard{rel, std::move(rhs)})};
        if constexpr (std::is_same_v<T, Literal>) {
            res.append(std::move(lit));
        } else {
            res.append(SimpleBodyLiteral{std::move(lit)});
        }
    }
}

template <class O, class T> auto opt_vector_or(O &&vec, T span) -> std::decay_t<O>::value_type {
    if (vec) {
        return *std::forward<O>(vec);
    }
    return {span.begin(), span.end()};
}

//! Return a Boolean literal with the given location and truth value.
[[nodiscard]] auto make_constant(Location loc, bool truth) -> Literal { return LiteralBoolean{loc, Sign::none, truth}; }

//! Ensure that the term only matches numbers.
[[nodiscard]] auto as_linear_term(SymbolStore &store, Term term) -> Term {
    auto loc = location(term);
    term = TermBinary(loc, TermSymbol{loc, store.num(1)}, BinaryOperator::times, std::move(term));
    return TermBinary(loc, std::move(term), BinaryOperator::plus, TermSymbol{loc, store.num(0)});
}

//! Introduce a fresh variable for the given term.
//!
//! If linear is true, the term is assumed to be a number
//! and a linear term instead of a variable is returned.
[[nodiscard]] auto map_term(RewriteContext &ctx, Term term, bool linear = false) -> Term {
    auto loc = location(term);
    ctx.aux().emplace_back(TermVariable{std::move(loc), ctx.gen().new_name()}, std::move(term));
    return linear ? as_linear_term(ctx.store(), ctx.aux().back().first) : ctx.aux().back().first;
}

//! Simplify a term vector.
[[nodiscard]] auto simplify_termvec(RewriteContext &ctx, TermVec const &terms) -> Util::ResultState<TermVec> {
    auto state_terms = true;
    auto res_terms = Util::ResultVec{terms};
    for (auto const &term : terms) {
        auto [state_term, res_term] = simplify(SimplifyTermFlags::none, ctx, term);
        state_terms = state_terms && state_term;
        if (state_terms) {
            res_terms.update(res_term);
        }
    }
    if (!state_terms) {
        return {false};
    }
    return {true, std::move(res_terms).as_optional()};
}

[[nodiscard]] auto all_symbol(TermVec const &terms) -> bool {
    return std::all_of(terms.begin(), terms.end(), is_symbol);
}

//! The detected type of a term.
enum class TermType {
    numeric,  //!< Term evaluates to a number.
    symbolic, //!< Term evaluates to a function.
    tuple,    //!< Term evaluates to a tuple.
    any,      //!< Term evaluates to anything.
};
//! The evaluation of the term failed.
struct TermResultFail {};
//! The evaluation resulted in a term of the given type.
struct TermResultChanged {
    TermType type;
    Term term;
};
//! The evaluation resulted in a linear term.
struct TermResultLinear {
    Term x;
    Number m;
    Number n;
};
//! The evaluation resulted in a symbol.
using TermResultSymbol = Symbol;
//! The evaluation did not change the term.
using TermResultUnchanged = TermType;
//! Variant for the different evaluation results.
using TermResult =
    std::variant<TermResultFail, TermResultSymbol, TermResultUnchanged, TermResultChanged, TermResultLinear>;

//! Construct an unchanged or changed term result depending on whether the old equals the new term or not.
[[nodiscard]] auto check_change(TermType type, Term const &old, Term new_) -> TermResult {
    if (old != new_) {
        return TermResultChanged{type, std::move(new_)};
    }
    return TermResultUnchanged{type};
}

//! Convert a linear result into a term.
[[nodiscard]] auto linear_as_term(RewriteContext &ctx, TermResultLinear res, bool simplify = true) -> Term {
    auto mxn = std::move(res.x);
    auto loc = location(mxn);
    if (!simplify || res.m != 1) {
        mxn =
            TermBinary(loc, TermSymbol{loc, ctx.store().num(std::move(res.m))}, BinaryOperator::times, std::move(mxn));
    }
    if (!simplify || res.n != 0) {
        mxn = TermBinary(loc, std::move(mxn), BinaryOperator::plus, TermSymbol{loc, ctx.store().num(std::move(res.n))});
    }
    return mxn;
}

//! Convert term results to terms reusing the old term if possible.
struct ResultAsTerm {
    //! Convert a linear result.
    [[nodiscard]] auto operator()(TermResultLinear res) -> Term {
        auto ret = linear_as_term(ctx, std::move(res));
        return ret != term ? std::move(ret) : term;
    }

    //! Convert a failed result.
    [[nodiscard]] auto operator()(TermResultFail res) const -> Term {
        static_cast<void>(res);
        throw std::logic_error("cannot happen");
    }

    //! Convert an unchanged result.
    [[nodiscard]] auto operator()(TermResultUnchanged res) const -> Term {
        static_cast<void>(res);
        return term;
    }

    //! Convert a changed result.
    [[nodiscard]] auto operator()(TermResultChanged res) const -> Term { return std::move(res.term); }

    //! Convert a symbol result.
    [[nodiscard]] auto operator()(TermResultSymbol res) const -> Term {
        Term ret = TermSymbol{location(term), res};
        return ret != term ? std::move(ret) : term;
    }
    //! The rewrite context.
    RewriteContext &ctx;
    //! The original term.
    Term const &term;
};

//! Convert terms of form V and -V where V is a variable to linear terms.
struct VarToLinear {
    //! Handle remaining term results.
    auto operator()(auto res) const -> TermResult { return res; }

    //! Handle unchanged results.
    auto operator()(TermResultUnchanged res) const -> TermResult {
        if (std::holds_alternative<TermVariable>(term)) {
            return TermResultLinear{term, Number(1), Number(0)};
        }
        auto const *term_unary = std::get_if<TermUnary>(&term);
        if (term_unary != nullptr && std::holds_alternative<TermVariable>(*term_unary->rhs())) {
            return TermResultLinear{term_unary->rhs(), Number(-1), Number(0)};
        }
        return res;
    }

    //! Handle changed results.
    auto operator()(TermResultChanged res) const -> TermResult {
        if (std::holds_alternative<TermVariable>(res.term)) {
            return TermResultLinear{std::move(res.term), Number(1), Number(0)};
        }
        auto const *term_unary = std::get_if<TermUnary>(&res.term);
        if (term_unary != nullptr && std::holds_alternative<TermVariable>(*term_unary->rhs())) {
            return TermResultLinear{term_unary->rhs(), Number(-1), Number(0)};
        }
        return res;
    }

    //! The original term.
    Term const &term;
};

//! Result indicating a changed tuple.
using TupleResultChanged = std::vector<std::variant<Projection, Symbol, Term>>;
//! Result indicating an changed tuple.
struct TupleResultUnhanged {};
//! Result indicating a tuples that failed to simplify.
struct TupleResultFail {};
//! Variant for the different tuple evaluation results.
using TupleResult = std::variant<TupleResultFail, TupleResultUnhanged, TupleResultChanged>;

//! Convert the given simplified arguments to a symbol vector.
//!
//! The result vector must only store symbols.
auto result_as_symbol_vec(TupleResultChanged args_tuple) -> std::vector<Symbol> {
    std::vector<Symbol> args;
    args.reserve(args_tuple.size());
    for (auto const &arg : args_tuple) {
        args.emplace_back(std::get<Symbol>(arg));
    }
    return args;
}

//! Convert the given simplified arguments to term tuple.
auto result_as_tuple(ArgumentTuple const &tuple, TupleResultChanged args_tuple) -> ArgumentTuple {
    std::vector<ArgumentTuple::Element> args;
    auto it = tuple.elems().begin();
    args.reserve(tuple.elems().size());
    for (auto &arg : args_tuple) {
        std::visit(
            [&](auto &&val) {
                GRINGO_MATCH(val, Symbol) { args.emplace_back(TermSymbol{location(std::get<Term>(*it)), val}); }
                else {
                    args.emplace_back(std::move(val));
                }
            },
            std::move(arg));
        ++it;
    }
    return ArgumentTuple{std::move(args)};
}

//! Simplify terms.
struct SimplifyTerm {
    //! Helper to simplify the arguments of the tuple.
    //!
    //! The resulting vector is nullopt if there were no simplifications.
    //! Otherwise, each element is either a projection, a symbol if it could
    //! evaluated right away, or a term in case of some other simplification.
    auto handle_tuple(SimplifyTermFlags flags, ArgumentTuple const &tuple, bool &constant) const -> TupleResult {
        size_t n = 0;

        TupleResult res_tuple = TupleResultUnhanged{};

        // helper to initialize the optional result vector
        auto init = [&]() -> TupleResultChanged & {
            auto *res_changed = std::get_if<TupleResultChanged>(&res_tuple);
            if (res_changed == nullptr) {
                res_changed = &res_tuple.emplace<TupleResultChanged>();
                res_changed->reserve(tuple.elems().size());
                for (auto it = tuple.elems().begin(), ie = it + n; it != ie; ++it) {
                    std::visit([res_changed](auto &&res) { res_changed->emplace_back(res); }, *it);
                }
            }
            return *res_changed;
        };

        auto simplify = [&, this](auto &&arg) -> bool {
            // projected argument
            GRINGO_MATCH(arg, Projection) {
                constant = false;
                init().emplace_back(arg);
                return true;
            }
            // term argument
            GRINGO_MATCH(arg, Term) {
                auto simplify = [&](auto &&res) -> bool {
                    // evaluation of argument failed
                    GRINGO_MATCH(res, TermResultFail) { return false; }
                    // argument evaluated to symbol
                    GRINGO_MATCH(res, TermResultSymbol) {
                        // see note at check_change for function/tuple visitor
                        init().emplace_back(res);
                    }
                    else {
                        constant = false;
                    }
                    GRINGO_MATCH(res, TermResultLinear) {
                        init().emplace_back(linear_as_term(ctx, std::move(res), false));
                    }
                    // argument did not change
                    GRINGO_MATCH(res, TermResultUnchanged) {
                        if (auto *res_changed = std::get_if<TupleResultChanged>(&res_tuple); res_changed != nullptr) {
                            res_changed->emplace_back(arg);
                        }
                    }
                    // argument changed
                    GRINGO_MATCH(res, TermResultChanged) { init().emplace_back(std::move(res.term)); }
                    return true;
                };
                return std::visit(simplify, operator()(arg, flags));
            }
        };

        // evaluate arguments
        auto res = true;
        for (auto const &arg : tuple.elems()) {
            res = std::visit(simplify, arg) && res;
            ++n;
        }
        if (!res) {
            return TupleResultFail{};
        }
        return res_tuple;
    }

    //! Simplify the given term.
    auto operator()(Term const &term, SimplifyTermFlags flags) const -> TermResult {
        return std::visit(*this, term, std::variant<SimplifyTermFlags>{flags});
    }

    //! Protect from calling unindented overloads.
    auto operator()(auto const &term, SimplifyTermFlags flags) const -> TermResult = delete;

    //! Simplify the given symbolic term.
    auto operator()(TermSymbol const &term, SimplifyTermFlags flags) const -> TermResult {
        static_cast<void>(flags);
        return term.value();
    }

    //! Simplify the given variable.
    auto operator()(TermVariable const &term, SimplifyTermFlags flags) const -> TermResult {
        static_cast<void>(term);
        static_cast<void>(flags);
        // a variable can represent any term
        return TermType::any;
    }

    //! Simplify the given function term.
    auto operator()(TermFunction const &term, SimplifyTermFlags flags) const -> TermResult {
        if (term.pool().size() != 1) {
            throw std::runtime_error("functions must be unpooled before simplifying");
        }

        bool preserve = test(flags, SimplifyTermFlags::preserve_toplevel);

        flags &= ~SimplifyTermFlags::preserve_toplevel;

        bool constant = !term.external();
        auto type = term.external() ? TermType::any : TermType::symbolic;
        auto const &tuple = term.pool().front();

        // simplify arguments
        return std::visit(
            [&, this](auto &&res) -> TermResult {
                GRINGO_MATCH(res, TupleResultFail) { return TermResultFail{}; }
                GRINGO_MATCH(res, TupleResultUnhanged) {
                    if (term.external() && !preserve) {
                        return TermResultChanged{type, map_term(ctx, term)};
                    }
                    if (!constant) {
                        return type;
                    }
                    return ctx.store().fun(term.name(), {}, false);
                }
                GRINGO_MATCH(res, TupleResultChanged) {
                    if (!constant) {
                        auto fun = TermFunction{
                            term.loc(), term.name(), {result_as_tuple(tuple, std::move(res))}, term.external()};
                        if (term.external() && !preserve) {
                            return TermResultChanged{type, map_term(ctx, std::move(fun))};
                        }
                        // Note: this is somewhat inefficient because the
                        // equality comparision recurses into the structure
                        return check_change(type, term, std::move(fun));
                    }
                    return ctx.store().fun(term.name(), result_as_symbol_vec(std::move(res)), false);
                }
            },
            handle_tuple(flags, tuple, constant));
    }

    //! Simplify the given term tuple.
    auto operator()(TermTuple const &term, SimplifyTermFlags flags) const -> TermResult {
        if (term.pool().size() != 1 || !std::holds_alternative<ArgumentTuple>(term.pool().front())) {
            throw std::runtime_error("tuples must be unpooled before simplifying");
        }

        flags &= ~SimplifyTermFlags::preserve_toplevel;

        bool constant = true;
        auto type = TermType::tuple;
        auto const &tuple = std::get<ArgumentTuple>(term.pool().front());

        // simplify arguments
        return std::visit(
            [&, this](auto &&res) -> TermResult {
                GRINGO_MATCH(res, TupleResultFail) { return TermResultFail{}; }
                GRINGO_MATCH(res, TupleResultUnhanged) {
                    // unchanged term that did not evaluate to a symbol
                    if (!constant) {
                        return type;
                    }
                    return ctx.store().tup(result_as_symbol_vec({}));
                }
                GRINGO_MATCH(res, TupleResultChanged) {
                    // changed term that did not evaluate to a symbol
                    if (!constant) {
                        // Note: this is somewhat inefficient because the
                        // equality comparision recurses into the structure
                        return check_change(type, term,
                                            TermTuple{term.loc(), Util::make_vec<TermTuple::Element>(
                                                                      result_as_tuple(tuple, std::move(res)))});
                    }
                    return ctx.store().tup(result_as_symbol_vec(std::move(res)));
                }
                // the term evaluated to a symbol
            },
            handle_tuple(flags, tuple, constant));
    }

    //! Simplify the given absolute term.
    auto operator()(TermAbs const &term, SimplifyTermFlags flags) const -> TermResult {
        if (term.pool().size() != 1) {
            throw std::runtime_error("absolute terms must be unpooled before simplifying");
        }

        flags &= ~SimplifyTermFlags::preserve_toplevel;

        auto simplify = [&term, this](auto &&res) -> TermResult {
            // evaluation of argument failed
            GRINGO_MATCH(res, TermResultFail) { return {}; }
            // the argument evaluated to a symbol
            GRINGO_MATCH(res, TermResultSymbol) {
                if (res.type() != SymbolType::number) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                          << "  " << term << "\n";
                    return TermResultFail{};
                }
                return ctx.store().num(abs(*res.num()));
            }
            GRINGO_MATCH(res, TermResultLinear) {
                std::vector<Term> pool;
                pool.emplace_back(linear_as_term(ctx, std::move(res)));
                return check_change(TermType::numeric, term, TermAbs(term.loc(), std::move(pool)));
            }
            // the argument did not change
            GRINGO_MATCH(res, TermResultUnchanged) {
                if (res == TermType::symbolic || res == TermType::tuple) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                          << "  " << term << "\n";
                    return TermResultFail{};
                }
                return TermResultUnchanged{TermType::numeric};
            }
            // the argument changed
            GRINGO_MATCH(res, TermResultChanged) {
                // handle invalid terms
                if (res.type == TermType::symbolic || res.type == TermType::tuple) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                          << "  " << term << "\n";
                    return TermResultFail{};
                }
                // construct a new term
                std::vector<Term> pool;
                pool.emplace_back(std::move(res.term));
                return TermResultChanged{TermType::numeric, TermAbs{term.loc(), std::move(pool)}};
            }
        };

        return std::visit(simplify, operator()(term.pool().front(), flags));
    }

    //! Simplify the given unary term.
    auto operator()(TermUnary const &term, SimplifyTermFlags flags) const -> TermResult {
        flags &= ~SimplifyTermFlags::preserve_toplevel;

        auto simplify = [&term, this](auto &&res) -> TermResult {
            // evaluation of argument failed
            GRINGO_MATCH(res, TermResultFail) { return TermResultFail{}; }
            // the argument evaluated to a symbol
            GRINGO_MATCH(res, TermResultSymbol) {
                // we can always evaluate constants
                auto opt_sym = evaluate(ctx.store(), term.op(), res);
                if (!opt_sym.has_value()) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                          << "  " << term << "\n";
                    return TermResultFail{};
                }
                return TermResultSymbol{opt_sym.value()};
            }
            GRINGO_MATCH(res, TermResultLinear) {
                if (term.op() == UnaryOperator::negate) {
                    res.m = -std::move(res.m);
                    res.n = -std::move(res.n);
                    return std::move(res);
                }
                return check_change(TermType::numeric, term,
                                    TermUnary(term.loc(), term.op(), linear_as_term(ctx, std::move(res))));
            }
            // get type of term based on the given type of its argument
            auto check_type = [this, &term](TermType type) -> std::optional<TermType> {
                if (type == TermType::tuple || (term.op() == UnaryOperator::invert && type == TermType::symbolic)) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                          << "  " << term << "\n";
                    return std::nullopt;
                }
                // ~term is always numeric
                return term.op() == UnaryOperator::invert ? TermType::numeric : type;
            };
            // simplify --symbolic to symbolic and ---any to -any
            auto fold = [&term](TermType type, Term const &rhs) -> std::optional<TermResultChanged> {
                if (term.op() != UnaryOperator::negate) {
                    return std::nullopt;
                }
                auto const *rhs_unary = std::get_if<TermUnary>(&rhs);
                if (rhs_unary == nullptr || rhs_unary->op() != UnaryOperator::negate) {
                    return std::nullopt;
                }
                // --symbolic
                if (type == TermType::symbolic) {
                    return TermResultChanged{type, rhs_unary->rhs()};
                }
                auto const *rhs_rhs_unary = std::get_if<TermUnary>(rhs_unary->rhs().get());
                if (rhs_rhs_unary == nullptr || rhs_rhs_unary->op() != UnaryOperator::negate) {
                    return std::nullopt;
                }
                // --any
                return TermResultChanged{type, rhs_unary->rhs()};
            };
            // the argument did not change
            GRINGO_MATCH(res, TermType) {
                auto type = check_type(res);
                if (!type.has_value()) {
                    return TermResultFail{};
                }
                // fold if possible
                if (auto opt_res = fold(type.value(), term.rhs()); opt_res.has_value()) {
                    return std::move(opt_res).value();
                }
                return TermResultUnchanged{type.value()};
            }
            // the argument changed
            GRINGO_MATCH(res, TermResultChanged) {
                auto type = check_type(res.type);
                if (!type.has_value()) {
                    return TermResultFail{};
                }
                // fold if possible
                if (auto opt_res = fold(type.value(), res.term); opt_res.has_value()) {
                    return std::move(opt_res).value();
                }
                return TermResultChanged{type.value(), TermUnary{term.loc(), term.op(), std::move(res.term)}};
            }
        };
        return std::visit(simplify, operator()(*term.rhs(), flags));
    }

    //! Simplify the given binary term.
    auto operator()(TermBinary const &term, SimplifyTermFlags flags) const -> TermResult {
        // check if the result can evaluate to a number
        auto is_numeric = [](auto const &res) -> bool {
            GRINGO_MATCH(res, TermResultFail) { return false; }
            GRINGO_MATCH(res, TermResultLinear) { return true; }
            GRINGO_MATCH(res, TermResultUnchanged) { return res == TermType::any || res == TermType::numeric; }
            GRINGO_MATCH(res, TermResultChanged) { return res.type == TermType::any || res.type == TermType::numeric; }
            GRINGO_MATCH(res, TermResultSymbol) { return res.type() == SymbolType::number; }
        };

        if (term.op() == BinaryOperator::dots) {
            auto simplify = [&, this](auto &&res_lhs, auto &&res_rhs) -> TermResult {
                // check arguments
                if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                          << "  " << term << "\n";
                    return {};
                }
                if (test(flags, SimplifyTermFlags::preserve_toplevel)) {
                    return check_change(TermType::numeric, term,
                                        TermBinary{term.loc(), ResultAsTerm{ctx, term.lhs()}(std::move(res_lhs)),
                                                   BinaryOperator::dots,
                                                   ResultAsTerm{ctx, term.rhs()}(std::move(res_rhs))});
                }
                // Note: If the surrounding term does not have to be matchable,
                // then the variable can be returned as is.
                auto var =
                    map_term(ctx, TermBinary{term.loc(), ResultAsTerm{ctx, term.lhs()}(std::move(res_lhs)),
                                             BinaryOperator::dots, ResultAsTerm{ctx, term.rhs()}(std::move(res_rhs))});
                if (test(flags, SimplifyTermFlags::matchable)) {
                    return TermResultLinear{std::move(var), Number{1}, Number{0}};
                }
                return TermResultChanged{TermType::numeric, std::move(var)};
            };
            return std::visit(simplify, operator()(*term.lhs(), flags), operator()(*term.rhs(), flags));
        }
        flags &= ~SimplifyTermFlags::preserve_toplevel;

        auto simplify = [&, this](auto &&res_lhs, auto &&res_rhs) -> TermResult {
            // check arguments
            if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                      << "  " << term << "\n";
                return {};
            }

            // evaluate to symbol
            GRINGO_MATCH2(res_lhs, Symbol, res_rhs, Symbol) {
                auto res = evaluate(ctx.store(), res_lhs, term.op(), res_rhs);
                if (!res.has_value()) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                          << "  " << term << "\n";
                    return TermResultFail{};
                }
                return res.value();
            }
            GRINGO_MATCH2(res_lhs, Symbol, res_rhs, TermResultLinear) {
                if (term.op() == BinaryOperator::plus) {
                    res_rhs.n += res_lhs.num();
                    return std::move(res_rhs);
                }
                if (term.op() == BinaryOperator::minus) {
                    res_rhs.m = -std::move(res_rhs.m);
                    res_rhs.n = res_lhs.num() - std::move(res_rhs.n);
                    return std::move(res_rhs);
                }
                if (term.op() == BinaryOperator::times && *res_lhs.num() != 0) {
                    res_rhs.m *= res_lhs.num();
                    res_rhs.n *= res_lhs.num();
                    return std::move(res_rhs);
                }
                return check_change(TermType::numeric, term,
                                    TermBinary(term.loc(), ResultAsTerm{ctx, term.lhs()}(res_lhs), term.op(),
                                               ResultAsTerm{ctx, term.rhs()}(std::move(res_rhs))));
            }
            GRINGO_MATCH2(res_lhs, TermResultLinear, res_rhs, Symbol) {
                if (term.op() == BinaryOperator::plus) {
                    res_lhs.n += res_rhs.num();
                    return std::move(res_lhs);
                }
                if (term.op() == BinaryOperator::minus) {
                    res_lhs.n -= res_rhs.num();
                    return std::move(res_lhs);
                }
                if (term.op() == BinaryOperator::times && *res_rhs.num() != 0) {
                    res_lhs.m *= res_rhs.num();
                    res_lhs.n *= res_rhs.num();
                    return std::move(res_lhs);
                }
                return check_change(TermType::numeric, term,
                                    TermBinary(term.loc(), ResultAsTerm{ctx, term.lhs()}(std::move(res_lhs)), term.op(),
                                               ResultAsTerm{ctx, term.rhs()}(res_rhs)));
            }
            GRINGO_MATCH2(res_lhs, TermResultLinear, res_rhs, TermResultLinear) {
                if (term.op() == BinaryOperator::plus) {
                    if (res_lhs.x == res_rhs.x) {
                        res_lhs.n += res_rhs.n;
                        res_lhs.m += res_rhs.m;
                        return std::move(res_lhs);
                    }
                    res_rhs.n += res_lhs.n;
                    res_lhs.n = Number(0);
                }
                if (term.op() == BinaryOperator::minus) {
                    if (res_lhs.x == res_rhs.x) {
                        res_lhs.n -= res_rhs.n;
                        res_lhs.m -= res_rhs.m;
                        return std::move(res_lhs);
                    }
                    res_rhs.n -= res_lhs.n;
                    res_lhs.n = Number(0);
                }
                return check_change(TermType::numeric, term,
                                    TermBinary(term.loc(), linear_as_term(ctx, std::move(res_lhs)), term.op(),
                                               linear_as_term(ctx, std::move(res_rhs))));
            }

            // none of the arguments changed
            GRINGO_MATCH2(res_lhs, TermType, res_rhs, TermType) { return TermType::numeric; }

            // at least one of the arguments changed
            return check_change(TermType::numeric, term,
                                TermBinary(term.loc(), ResultAsTerm{ctx, term.lhs()}(std::move(res_lhs)), term.op(),
                                           ResultAsTerm{ctx, term.rhs()}(std::move(res_rhs))));
        };

        // construct result
        return std::visit(simplify, std::visit(VarToLinear{term.lhs()}, operator()(*term.lhs(), flags)),
                          std::visit(VarToLinear{term.rhs()}, operator()(*term.rhs(), flags)));
    }

    RewriteContext &ctx; //!< Context used during simplification.
};

//! Make a term matchable by removing terms that cannot be matched.
//!
//! If the unfailable flag is set, all terms that can evaluate to undefined are
//! removed as well. Only produces a result if one of the arguments changed;
//! there are no failures to handle.
struct MakeMatchableTerm {
    using Result = std::optional<Term>;
    using ResultTuple = std::optional<std::vector<ArgumentTuple::Element>>;

    //! Make the arguments of the given tuple matchable.
    [[nodiscard]] auto handle_tuple(SimplifyTermFlags flags, ArgumentTuple const &tuple) const -> ResultTuple {
        size_t n = 0;

        ResultTuple res_tuple;

        // helper to initialize the optional result vector
        auto init = [&]() -> std::vector<ArgumentTuple::Element> & {
            if (!res_tuple.has_value()) {
                res_tuple = Util::copy_n(tuple.elems(), n);
            }
            return *res_tuple;
        };

        auto handle_argument = [&, this](auto &&arg) -> void {
            // projected argument
            GRINGO_MATCH(arg, Projection) {
                if (res_tuple.has_value()) {
                    init().emplace_back(arg);
                }
            }
            // term argument
            GRINGO_MATCH(arg, Term) {
                if (auto res_arg = operator()(arg, flags & ~SimplifyTermFlags::nested_matchable);
                    res_arg.has_value() || res_tuple.has_value()) {
                    init().emplace_back(std::move(res_arg).value_or(arg));
                }
            }
        };

        // evaluate arguments
        for (auto const &arg : tuple.elems()) {
            std::visit(handle_argument, arg);
            ++n;
        }
        return res_tuple;
    }

    //! Make the given term matchable.
    auto operator()(Term const &term, SimplifyTermFlags flags) const -> Result {
        return std::visit(*this, term, std::variant<SimplifyTermFlags>{flags});
    }

    //! Make the given symbolic term matchable.
    auto operator()(TermSymbol const &term, SimplifyTermFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        return std::nullopt;
    }

    //! Make the given variable term matchable.
    auto operator()(TermVariable const &term, SimplifyTermFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        return std::nullopt;
    }

    //! Make the given function term matchable.
    auto operator()(TermFunction const &term, SimplifyTermFlags flags) const -> Result {
        assert(term.pool().size() == 1);
        return Util::transform(handle_tuple(flags, term.pool().front()), [&term](auto &&args) {
            return TermFunction{term.loc(), term.name(), {ArgumentTuple{std::move(args)}}, term.external()};
        });
    }

    //! Make the given tuple term matchable.
    auto operator()(TermTuple const &term, SimplifyTermFlags flags) const -> Result {
        assert(term.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(term.pool().front()));
        return Util::transform(handle_tuple(flags, std::get<ArgumentTuple>(term.pool().front())), [&term](auto &&args) {
            return TermTuple{term.loc(), {ArgumentTuple{std::move(args)}}};
        });
    }

    //! Make the given absolute term matchable.
    auto operator()(TermAbs const &term, SimplifyTermFlags flags) const -> Result {
        if (!test(flags, SimplifyTermFlags::unfailable) && test(flags, SimplifyTermFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(ctx, term, !test(flags, SimplifyTermFlags::unfailable));
    }

    //! Make the given unary term matchable.
    auto operator()(TermUnary const &term, SimplifyTermFlags flags) const -> Result {
        if (!test(flags, SimplifyTermFlags::unfailable) && term.op() == UnaryOperator::negate) {
            return Util::transform(operator()(term.rhs(), flags), [&term](auto &&arg) -> Term {
                return TermUnary{term.loc(), term.op(), GRINGO_FWD(arg)};
            });
        }
        if (!test(flags, SimplifyTermFlags::unfailable) && test(flags, SimplifyTermFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(ctx, term, !test(flags, SimplifyTermFlags::unfailable) && always_numeric(term));
    }

    //! Make the given binary term matchable.
    auto operator()(TermBinary const &term, SimplifyTermFlags flags) const -> Result {
        if (is_linear(term)) {
            // The goal here is to avoid adding additional assignments for auxiliary variables
            // that correspond to variables having a numeric value.
            if (test(flags, SimplifyTermFlags::unfailable)) {
                const auto &n = std::get<TermSymbol>(*term.rhs());
                const auto &mx = std::get<TermBinary>(*term.lhs());
                const auto &m = std::get<TermSymbol>(*mx.lhs());
                if (*n.value().num() == 0 && *m.value().num() == 1) {
                    for (auto &[lhs, rhs] : ctx.aux()) {
                        if (*mx.rhs() == lhs) {
                            if (always_numeric(rhs)) {
                                return mx.rhs();
                            }
                            break;
                        }
                    }
                }
            } else {
                return std::nullopt;
            }
        }
        if (!test(flags, SimplifyTermFlags::unfailable) && test(flags, SimplifyTermFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(ctx, term, !test(flags, SimplifyTermFlags::unfailable));
    }

    RewriteContext &ctx; //!< Context used during simplification.
};

//! Simplify literals.
//!
//! Does not return a value if the literal did not change.
struct SimplifyLiteral {
    //! Simplify literals dispatching based on type stored in variant.
    auto operator()(Literal const &lit, SimplifyLiteralFlags flags) const -> SimplifyResult<Literal> {
        return std::visit(*this, lit, std::variant<SimplifyLiteralFlags>{flags});
    }

    //! Simplify Boolean literals.
    //!
    //! Ensures that the literal is either true or false.
    auto operator()(LiteralBoolean const &lit, SimplifyLiteralFlags flags) const -> SimplifyResult<Literal> {
        static_cast<void>(flags);
        auto value = (lit.sign_ != Sign::once) == lit.value_;
        auto state = value ? TruthValue::top : TruthValue::bot;

        if (lit.sign_ != Sign::none) {
            return {state, make_constant(lit.loc(), value)};
        }
        return {state};
    }

    //! Simplify relation literals.
    //!
    //! The function ensures the following properties:
    //! (1) both sides of assignments are matchable (if parts of the terms are matchable),
    //! (2) terms in disjunctive non-binary relations cannot evaluate to empty pools.
    //! The letter is important to ensure that relations can be split into multiple rules
    //! without unintuitive side-effects.
    //!
    //! Consider the program
    //!   p(a).
    //!   2 <= 1 <= X+5 :- p(X).
    //! This is not equilavent to
    //!   p(a).
    //!   2 <= 1 :- p(X).
    //!   1 <= X+5 :- p(X).
    //! because first program is satisfiable while the second is unsatisfiable.
    //!
    //! A correct translation is
    //!   p(a).
    //!   2 <= 1 <= A | A!=X+5 :- p(X).
    //! equivalent to
    //!   p(a).
    //!   2 <= 1 :- p(X), A=X+5.
    //!   1 <= A :- p(X), A=X+5.
    //! The seemingly unnecessary assignment ensures that pools are handled correctly.
    //!
    //! A similar effect can be observed with negated relation literals in the body.
    //!
    //! Applied Simplifications:
    //!
    //!   (1) X=2<1 => false
    //!   (2) X=2>1 => X=2>1
    //!   (3) not X!=2<1 => true
    //!   (4) not X!=2>1 => not X=2>1
    //!
    //! Note that cases (1) and (2) should be fine regarding safety.
    //! Further, simplifications in cases (2) and (3) are delayed until the comparisons are unpooled.
    //!
    //! Terms of form t..u are preserved if preserve_toplevel_dots is set.
    //! (This does not apply to nested terms.)
    //!
    //! Assignments of form t=X are replaced by X=t if t is not a variable.
    auto operator()(LiteralRelation const &lit, SimplifyLiteralFlags flags) const -> SimplifyResult<Literal> {
        // whether pools are treated disjunctively or conjunctively
        bool head = test(flags, SimplifyLiteralFlags::head);
        // whether the elements of the relation are disjunctive or conjunctive
        // (after applying the sign)
        bool disjunctive = head != (lit.sign_ == Sign::once);

        auto fixed_flags = SimplifyTermFlags::none;
        if (lit.rhs_.size() > 1 && disjunctive) {
            // ensure that unpooling preserves terms that can fail
            fixed_flags = SimplifyTermFlags::matchable | SimplifyTermFlags::unfailable;
        }
        // the relation symbol that corresponds to assignment
        // (in the head it is inequality)
        auto assign = disjunctive ? Relation::inequal : Relation::equal;

        auto get_constant = [](Term const &orig, std::optional<Term> const &res) -> std::optional<Symbol> {
            if (res.has_value()) {
                if (auto const *sym = std::get_if<TermSymbol>(&res.value()); sym != nullptr) {
                    return sym->value();
                }
            } else {
                if (auto const *sym = std::get_if<TermSymbol>(&orig); sym != nullptr) {
                    return sym->value();
                }
            }
            return std::nullopt;
        };

        // the truth value of the relation literal if all (signed) comparisions are true
        auto state = lit.sign_ != Sign::once ? TruthValue::top : TruthValue::bot;
        // the truth value of the literal fixed by one of the  comparisons
        auto state_fixed = lit.sign_ != Sign::once ? TruthValue::bot : TruthValue::top;
        // the truth value if evaluation of a term fails
        auto state_fail = head ? TruthValue::top : TruthValue::bot;

        // simplify lhs
        auto match_flags = SimplifyTermFlags::none;
        if (lit.rhs_.front().first == assign) {
            match_flags = SimplifyTermFlags::matchable | SimplifyTermFlags::nested_matchable;
        }

        // binary assignment
        if (lit.rhs_.size() == 1 && lit.rhs_.front().first == assign) {
            // Note: in theory the left hand side could even be a more complex term that
            // is made machable (but not nested matchable).
            if (!is_variable(lit.lhs_) && is_variable(lit.rhs_.front().second)) {
                auto inv = LiteralRelation{lit.loc(), lit.sign_, lit.rhs_.front().second,
                                           Util::make_vec<Guard>(Guard{assign, lit.lhs_})};
                auto res = operator()(inv, flags);
                if (!res.value.has_value()) {
                    res.value = std::move(inv);
                }
                return res;
            }

            if (lit.rhs_.size() == 1 && lit.rhs_.front().first == assign && is_variable(lit.lhs_)) {
                fixed_flags |= SimplifyTermFlags::preserve_toplevel;
            }
        }

        auto [succeeded, res_lhs] = simplify(fixed_flags | match_flags, ctx, lit.lhs_);

        // simplify rhs
        auto res_rhs = Util::ResultVec{lit.rhs_};
        auto prev_symbol = get_constant(lit.lhs_, res_lhs);
        size_t n = 0;
        for (auto const &[rel, term] : lit.rhs_) {
            ++n;
            match_flags = SimplifyTermFlags::none;
            if (rel == assign || (n < lit.rhs_.size() && lit.rhs_[n].first == assign)) {
                match_flags = SimplifyTermFlags::matchable | SimplifyTermFlags::nested_matchable;
            }
            auto [state_term, res_term] = simplify(fixed_flags | match_flags, ctx, term);
            succeeded = succeeded && state_term;
            if (!succeeded || state == state_fixed) {
                continue;
            }
            res_rhs.update(Util::transform(std::move(res_term), [&rel](auto term) {
                return Guard{rel, std::move(term)};
            }));
            auto cur_symbol = get_constant(term, res_term);
            if (prev_symbol.has_value() && cur_symbol.has_value()) {
                // the truth value of the relation literal is fixed if the comparison is false
                if (!evaluate(prev_symbol.value(), rel, cur_symbol.value())) {
                    state = state_fixed;
                }
            } else {
                state = TruthValue::unknown;
            }

            prev_symbol = cur_symbol;
        }

        // construct result
        if (!succeeded) {
            state = state_fail;
        }
        if (state != TruthValue::unknown) {
            return {state, make_constant(lit.loc(), state == TruthValue::top)};
        }
        if (res_lhs.has_value() || res_rhs.has_value() || lit.sign_ == Sign::twice) {
            auto sign = lit.sign_ == Sign::twice ? Sign::none : lit.sign_;
            return {TruthValue::unknown, LiteralRelation{lit.loc(), sign, std::move(res_lhs).value_or(lit.lhs_),
                                                         std::move(res_rhs).value()}};
        }
        return {TruthValue::unknown};
    }

    //! Simplify symbolic literals.
    //!
    //! The function ensures the following properties:
    //! (1) the literal is matchable if the corresponding flag has been set,
    //! (2) projection is accepted if the corresponding flag has been set.
    auto operator()(LiteralSymbolic const &lit, SimplifyLiteralFlags flags) const -> SimplifyResult<Literal> {
        bool head = test(flags, SimplifyLiteralFlags::head);
        auto sub_flags = SimplifyTermFlags::none;

        if (test(flags, SimplifyLiteralFlags::unfailable)) {
            sub_flags |= SimplifyTermFlags::matchable | SimplifyTermFlags::unfailable;
        } else if (test(flags, SimplifyLiteralFlags::matchable) && (!head && lit.sign_ == Sign::none)) {
            sub_flags |= SimplifyTermFlags::matchable;
        }
        auto [state, res] = simplify(sub_flags, ctx, lit.term_);
        if (!state) {
            return {head ? TruthValue::top : TruthValue::bot, make_constant(lit.loc(), head)};
        }
        return {TruthValue::unknown, Util::transform(std::move(res), [&](auto term) {
                    return LiteralSymbolic{lit.loc(), lit.sign_, std::move(term)};
                })};
    }

    RewriteContext &ctx; //!< Context used during simplification.
};

struct LiteralToTuple {
    auto operator()(Literal const &lit) -> TermVec { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) -> TermVec {
        ++n;
        return Util::make_vec<Term>(TermSymbol{lit.loc(), store.num(n)});
    }

    auto operator()(LiteralRelation const &lit) -> TermVec {
        ++n;
        auto var_set = select_variables(lit);
        auto var_vec = VariableVec(var_set.begin(), var_set.end());
        std::sort(var_vec.begin(), var_vec.end());
        std::vector<Term> res;
        res.reserve(var_vec.size() + 1);
        res.emplace_back(TermSymbol{lit.loc(), store.num(n)});
        for (auto const &var : var_vec) {
            res.emplace_back(TermVariable{lit.loc(), var});
        }
        return res;
    }

    auto operator()(LiteralSymbolic const &lit) -> TermVec {
        std::vector<Term> res;
        res.reserve(2);
        int i = 0;
        switch (lit.sign_) {
            case Sign::none: {
                i = 0;
                break;
            }
            case Sign::once: {
                i = 1;
                break;
            }
            case Sign::twice: {
                i = 2;
                break;
            }
        }
        res.emplace_back(TermSymbol{lit.loc(), store.num(i)});
        res.emplace_back(lit.term_);
        return res;
    }

    SymbolStore &store;
    int n = 2;
};

//! Simplify a conjunction of literals.
//!
//! In the conjunctive case empty pools evaluate disjunctively, and the result is bot.
//! In the disjunctive case empty pools evaluate conjunctively, and the result is top.
//!
//! @note The automatic extension with the aux elements makes for a somewhat awkward interface.
[[nodiscard]] auto simplify_litvec(RewriteContext &ctx, LiteralVec const &lits, bool conjunctive = true)
    -> SimplifyResult<std::vector<Literal>> {
    auto state_fixed = conjunctive ? TruthValue::bot : TruthValue::top;
    auto state_empty = conjunctive ? TruthValue::top : TruthValue::bot;
    auto state_lits = state_empty;
    auto res_lits = Util::ResultVec{lits};
    for (auto const &lit : lits) {
        auto [state, value] =
            simplify(conjunctive ? SimplifyLiteralFlags::matchable : SimplifyLiteralFlags::head, ctx, lit);
        if (state_lits == state_fixed) {
            continue;
        }
        if (state == state_fixed) {
            if (lits.size() != 1 || value.has_value()) {
                res_lits.as_optional() = Util::make_vec<Literal>(std::move(value).value_or(lit));
            }
            state_lits = state_fixed;
        } else if (state == state_empty) {
            res_lits.remove();
        } else {
            state_lits = TruthValue::unknown;
            res_lits.update(std::move(value));
        }
    }
    if (state_lits != state_fixed) {
        extend(res_lits, ctx.aux(), conjunctive);
    }
    return {state_lits, std::move(res_lits).as_optional()};
}

//! Simplify a conditional literal.
//!
//! In case the truth value of the conditional literal is fixed,
//! the resulting literal has an empty condtion and its literal is a Boolean literal.
//!
//!
//! Example for the head (not conjunctive):
//! - pools have to be evaluated disjunctively in conclusions
//!   to see this, we have a look at the example below
//! - p((X;A+B),Z): q(Z) :- r.                      % 1
//!   - (p(X,Z) | p(A+B,Z)): q(Z) :- r.             % 1.1
//!     - p(X,Z) : q(Z); p(Y,Z): q(Z), Y=A+B :- r.  % 1.1.1
//! - case: A+B is undefined
//!   - p(X,Z) : q(Z) :- r.
//!   - this is the same as obtained from 1.1.1
//!
//! Example for the body (conjunctive):
//! - pools have to be evaluated conjunctively in conclusions
//!   to see this, we have a look at the example below
//! - p :- q((X;A+B),Z): r.                        % 1
//!   - p :- (q(X,Z) & q(A+B,Z)): r(Z).            % 1.1
//!     - p :- q(X,Z): r(Z); q(Y,Z): r(Z), Y=A+B.  % 1.1.1
//! - case: A+B is undefined
//!   - p :- q(X,Z): r(Z).
//!   - this is the same as obtained from 1.1.1
[[nodiscard]] auto simplify_condlit(RewriteContext &ctx, ConditionalLiteral const &lit, bool conjunctive)
    -> SimplifyResult<ConditionalLiteral> {
    auto guard = ctx.push();
    auto [state_lit, res_lit] =
        simplify(conjunctive ? SimplifyLiteralFlags::head : SimplifyLiteralFlags::matchable, ctx, lit.lit_);
    auto [state_cond, res_cond] = simplify_litvec(ctx, lit.cond_);

    auto state_fixed = conjunctive ? TruthValue::top : TruthValue::bot;
    auto state = TruthValue::unknown;

    // elements of *junctions can be removed if their conclusion is neutral
    if (state_lit == state_fixed) {
        // ensure result: "#true/#false:"
        if (!lit.cond_.empty()) {
            res_cond = LiteralVec{};
        }
        state = state_fixed;
    }
    // elements of *junctions can be removed if their condition is false
    else if (state_cond == TruthValue::bot) {
        // ensure result: ":#false"
        res_lit = LiteralBoolean{lit.loc(), Sign::none, conjunctive};
        state = state_fixed;
    } else if (state_cond == TruthValue::top && state_lit != TruthValue::unknown) {
        state = state_lit;
    }

    if (res_lit || res_cond) {
        return {state, ConditionalLiteral{lit.loc(), std::move(res_lit).value_or(lit.lit_),
                                          std::move(res_cond).value_or(lit.cond_)}};
    }
    return {state};
}

//! Simplify the left guard of an aggregate.
[[nodiscard]] auto simplify_guard(RewriteContext &ctx, LGuard const &guard, bool matchable)
    -> Util::ResultState<LGuard::value_type> {
    if (guard.has_value()) {
        auto [state, res] =
            simplify(matchable ? SimplifyTermFlags::matchable : SimplifyTermFlags::none, ctx, guard->first);
        return {state, Util::transform(std::move(res), [&guard](auto &&term) {
                    return LGuard::value_type{std::move(term), guard->second};
                })};
    }
    return {true};
}

//! Simplify the right guard of an aggregate.
[[nodiscard]] auto simplify_guard(RewriteContext &ctx, RGuard const &guard, bool matchable)
    -> Util::ResultState<RGuard::value_type> {
    if (guard.has_value()) {
        auto [state, res] =
            simplify(matchable ? SimplifyTermFlags::matchable : SimplifyTermFlags::none, ctx, guard->second);
        return {state, Util::transform(std::move(res), [&guard](auto &&term) {
                    return RGuard::value_type{guard->first, std::move(term)};
                })};
    }
    return {true};
}

//! Simplify a head aggregate element.
[[nodiscard]] auto simplify_element(RewriteContext &ctx, HeadAggregate::Element const &elem)
    -> SimplifyResult<HeadAggregate::Element> {
    auto guard = ctx.push();
    auto [state_tuple, res_tuple] = simplify_termvec(ctx, elem.tuple_);
    auto [state_lit, res_lit] = simplify(SimplifyLiteralFlags::none, ctx, elem.lit_);
    auto [state_cond, res_cond] = simplify_litvec(ctx, elem.cond_);

    if (!state_tuple) {
        state_cond = TruthValue::bot;
    }

    auto state_elem = TruthValue::unknown;
    if (state_lit == TruthValue::top && state_cond == TruthValue::top) {
        auto const &tuple = res_tuple ? *res_tuple : elem.tuple_;
        state_elem = all_symbol(tuple) ? TruthValue::top : TruthValue::unknown;
        if (!elem.cond_.empty()) {
            res_cond = LiteralVec{};
        }
    }
    if (state_lit == TruthValue::bot || state_cond == TruthValue::bot) {
        state_elem = TruthValue::bot;
        if (!elem.tuple_.empty()) {
            res_tuple = TermVec{};
        }
        if (!elem.cond_.empty()) {
            res_cond = LiteralVec{};
        }
        if (state_lit != TruthValue::bot) {
            res_lit = LiteralBoolean{location(elem.lit_), Sign::none, false};
        }
    }
    auto const *rel_lit = std::get_if<LiteralRelation>(res_lit ? &*res_lit : &elem.lit_);
    if (rel_lit != nullptr) {
        assert(state_cond != TruthValue::bot);
        if (!res_cond.has_value()) {
            res_cond = elem.cond_;
        }
        res_cond->emplace_back(std::move(res_lit).value_or(elem.lit_));
        res_lit = make_constant(location(elem.lit_), true);
    }
    if (res_tuple.has_value() || res_lit.has_value() || res_cond.has_value()) {
        return {state_elem, HeadAggregate::Element{elem.loc(), std::move(res_tuple).value_or(elem.tuple_),
                                                   std::move(res_lit).value_or(elem.lit_),
                                                   std::move(res_cond).value_or(elem.cond_)}};
    }
    return {state_elem};
}

//! Simplify a body aggregate element.
[[nodiscard]] auto simplify_element(RewriteContext &ctx, BodyAggregate::Element const &elem)
    -> SimplifyResult<BodyAggregate::Element> {
    auto guard = ctx.push();
    auto [state_tuple, res_tuple] = simplify_termvec(ctx, elem.tuple_);
    auto [state_cond, res_cond] = simplify_litvec(ctx, elem.cond_);

    if (!state_tuple) {
        state_cond = TruthValue::bot;
    }

    auto state_elem = TruthValue::unknown;
    if (state_cond == TruthValue::top) {
        auto const &tuple = res_tuple ? *res_tuple : elem.tuple_;
        state_elem = all_symbol(tuple) ? TruthValue::top : TruthValue::unknown;
        if (!elem.cond_.empty()) {
            res_cond = LiteralVec{};
        }
    }
    if (state_cond == TruthValue::bot) {
        state_elem = TruthValue::bot;
        if (!elem.tuple_.empty()) {
            res_tuple = TermVec{};
        }
    }
    if (res_tuple.has_value() || res_cond.has_value()) {
        return {state_elem, BodyAggregate::Element{elem.loc(), std::move(res_tuple).value_or(elem.tuple_),
                                                   std::move(res_cond).value_or(elem.cond_)}};
    }
    return {state_elem};
}

//! Get the neutral value of the given aggregate.
//!
//! This correponds to the aggregate function applied to the empty set.
auto neutral_value(AggregateFunction fun) -> std::variant<Number, Symbol> {
    switch (fun) {
        case AggregateFunction::sum:
        case AggregateFunction::sump:
        case AggregateFunction::count: {
            return Number{0};
        }
        case AggregateFunction::min: {
            return SymbolStore::sup();
        }
        case AggregateFunction::max: {
            break;
        }
    }
    return SymbolStore::inf();
}

//! Get the weight of a tuple (if it has one).
auto value(TermVec const &tuple) -> std::optional<Symbol> {
    if (!tuple.empty()) {
        return std::get<TermSymbol>(tuple.front()).value();
    }
    return std::nullopt;
}

//! Get the weight of a tuple as a number (zero if it has none).
auto weight(TermVec const &tuple) -> NumberRef {
    if (!tuple.empty()) {
        auto const &sym = std::get<TermSymbol>(tuple.front());
        if (sym.value().type() == SymbolType::number) {
            return sym.value().num();
        }
    }
    return NumberRef{Number{0}};
}

//! Accumulate the given symbol to res.
//!
//! For count aggregates this should simply be one.
void accumulate(AggregateFunction fun, TermVec const &tuple, std::variant<Number, Symbol> &res) {
    switch (fun) {
        case AggregateFunction::sum: {
            std::get<Number>(res) += weight(tuple);
            break;
        }
        case AggregateFunction::sump: {
            auto val = weight(tuple);
            if (*val >= 0) {
                std::get<Number>(res) += val;
            }
            break;
        }
        case AggregateFunction::count: {
            std::get<Number>(res) += Number(1);
            break;
        }
        case AggregateFunction::min: {
            auto val = value(tuple);
            if (val.has_value() && *val < std::get<Symbol>(res)) {
                res = *val;
            }
            break;
        }
        case AggregateFunction::max: {
            auto val = value(tuple);
            if (val.has_value() && *val > std::get<Symbol>(res)) {
                res = *val;
            }
            break;
        }
    }
}

//! Check if the given tuple is relevant for the aggregate function.
[[nodiscard]] auto check_tuple(AggregateFunction fun, TermVec const &tuple) -> bool {
    if (fun == AggregateFunction::count) {
        return true;
    }
    if (tuple.empty()) {
        return false;
    }
    switch (fun) {
        case AggregateFunction::sum:
        case AggregateFunction::sump: {
            if (never_numeric(tuple.front())) {
                return false;
            }
            break;
        }
        case AggregateFunction::count:
        case AggregateFunction::min:
        case AggregateFunction::max: {
            break;
        }
    }
    auto const *sym = std::get_if<TermSymbol>(&tuple.front());
    if (sym == nullptr) {
        return true;
    }
    switch (fun) {
        case AggregateFunction::count: {
            break;
        }
        case AggregateFunction::sum: {
            return sym->value().type() == SymbolType::number && *sym->value().num() != 0;
        }
        case AggregateFunction::sump: {
            return sym->value().type() == SymbolType::number && *sym->value().num() > 0;
        }
        case AggregateFunction::min: {
            return sym->value().type() != SymbolType::sup;
        }
        case AggregateFunction::max: {
            return sym->value().type() != SymbolType::inf;
        }
    }
    return true;
}

//! Shortcut for head/body aggregates.
template <bool head> using HBAggregate = std::conditional_t<head, HeadAggregate, BodyAggregate>;
//! Shortcut for head/body literals.
template <bool head> using HBLiteral = std::conditional_t<head, HeadLiteral, BodyLiteral>;
//! Shortcut for simple head/body literals.
template <bool head> using SimpleHBLiteral = std::conditional_t<head, SimpleHeadLiteral, SimpleBodyLiteral>;

//! Check if the given relation forms an assignment together with the aggregate.
template <bool head> [[nodiscard]] auto is_assignment(HBAggregate<head> const &lit, Relation rel) -> bool {
    if constexpr (!head) {
        return lit.sign_ == Sign::once ? rel == Relation::inequal : rel == Relation::equal;
    }
    return false;
}

//! Simplify a head or body aggregate.
template <bool head>
[[nodiscard]] auto simplify_aggregate(RewriteContext &ctx, HBAggregate<head> const &lit)
    -> SimplifyResult<HBLiteral<head>> {
    auto [state_lhs, res_lhs] =
        simplify_guard(ctx, lit.lhs_, lit.lhs_.has_value() && is_assignment<head>(lit, lit.lhs_->second));
    auto [state_rhs, res_rhs] =
        simplify_guard(ctx, lit.rhs_, lit.rhs_.has_value() && is_assignment<head>(lit, lit.rhs_->first));
    auto res_elems = Util::ResultVec{lit.elems_};
    bool constant = true;
    auto value = neutral_value(lit.fun_);
    auto tuples = Util::unordered_set<TermVec, Util::value_hasher<TermVec>>{};
    for (auto const &elem : lit.elems_) {
        auto [state_elem, res_elem] = simplify_element(ctx, elem);
        auto const &tuple = (res_elem ? *res_elem : elem).tuple_;
        if (state_elem == TruthValue::bot || !check_tuple(lit.fun_, tuple)) {
            if (state_elem != TruthValue::bot) {
                GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, elem.loc())
                    << "aggregate function undefined for tuple:\n"
                    << "  " << elem << "\n";
            }
            res_elems.remove();
            continue;
        }
        if (state_elem == TruthValue::unknown) {
            constant = false;
        } else if (tuples.emplace(tuple).second) {
            if (!tuple.empty()) {
            }
            accumulate(lit.fun_, tuple, value);
        }
        res_elems.update(std::move(res_elem));
    }
    if (!state_lhs) {
        return {head ? TruthValue::top : TruthValue::bot, SimpleHBLiteral<head>{make_constant(location(lit), head)}};
    }
    // Note: value also gives a lower bound for the aggregate, which could be used to detect true/false aggregates
    // (unlikely to be relevant in practice)
    if constexpr (!head) {
        if (!lit.lhs_.has_value() && !lit.rhs_.has_value()) {
            return {TruthValue::top, SimpleHBLiteral<head>{make_constant(lit.loc(), lit.sign_ != Sign::once)}};
        }
    }
    if (constant) {
        auto sign = Sign::none;
        if constexpr (head) {
            if (!lit.lhs_.has_value() && !lit.rhs_.has_value()) {
                return {TruthValue::top, SimpleHBLiteral<head>{make_constant(lit.loc(), true)}};
            }
        } else {
            sign = lit.sign_;
        }
        auto lhs = Term{TermSymbol{lit.loc(), std::visit(
                                                  [&ctx](auto &&value) {
                                                      GRINGO_MATCH(value, Number) {
                                                          return ctx.store().num(GRINGO_FWD(value));
                                                      }
                                                      GRINGO_MATCH(value, Symbol) { return value; }
                                                  },
                                                  value)}};
        auto guards = std::vector<Guard>{};
        if (lit.lhs_.has_value()) {
            guards.emplace_back(lit.lhs_->second, std::move(lhs));
            lhs = Util::transform(std::move(res_lhs), [](auto guard) {
                      return std::move(guard).first;
                  }).value_or(lit.lhs_->first);
        }
        if (lit.rhs_.has_value()) {
            guards.emplace_back(lit.rhs_->first, Util::transform(std::move(res_rhs), [](auto guard) {
                                                     return std::move(guard).second;
                                                 }).value_or(lit.rhs_->second));
        }
        auto rel_lit = SimpleHBLiteral<head>{LiteralRelation{lit.loc(), sign, std::move(lhs), std::move(guards)}};
        auto [state_lit, res_lit] = simplify(ctx, rel_lit);
        return {state_lit, std::move(res_lit).value_or(std::move(rel_lit))};
    }
    if (res_lhs.has_value() || res_rhs.has_value() || res_elems.has_value()) {
        auto lhs =
            Util::transform(lit.lhs_, [&res_lhs](auto const &orig) { return std::move(res_lhs).value_or(orig); });
        auto rhs =
            Util::transform(lit.rhs_, [&res_rhs](auto const &orig) { return std::move(res_rhs).value_or(orig); });
        if constexpr (head) {
            return {TruthValue::unknown,
                    HeadAggregate{lit.loc(), std::move(lhs), lit.fun_, std::move(res_elems).value(), std::move(rhs)}};
        } else {
            return {TruthValue::unknown, BodyAggregate{lit.loc(), lit.sign_, std::move(lhs), lit.fun_,
                                                       std::move(res_elems).value(), std::move(rhs)}};
        }
    }
    return {TruthValue::unknown};
}

//! Simplify a theory atom element.
[[nodiscard]] auto simplify_element(RewriteContext &ctx, TheoryElement const &elem) -> SimplifyResult<TheoryElement> {
    using Util::UPA;

    auto guard = ctx.push();
    auto res_tuple = std::optional<TheoryTermVec>{};
    auto [state_cond, res_cond] = simplify_litvec(ctx, elem.cond());

    auto state_elem = TruthValue::unknown;
    if (state_cond == TruthValue::top) {
        state_elem = TruthValue::unknown;
        if (!elem.cond().empty()) {
            res_cond = LiteralVec{};
        }
    }
    if (state_cond == TruthValue::bot) {
        state_elem = TruthValue::bot;
        if (!elem.tuple().empty()) {
            res_tuple = TheoryTermVec{};
        }
    }
    return {state_elem, Util::update<TheoryElement>(elem.loc(), UPA{elem.tuple(), std::move(res_tuple)},
                                                    UPA{elem.cond(), std::move(res_cond)})};
}

//! Simplify a theory atom.
template <bool HasSign>
auto simplify_theory_atom(RewriteContext &ctx, TheoryAtom<HasSign> const &lit) -> SimplifyResult<HBLiteral<!HasSign>> {
    using Util::UPA;

    constexpr auto head = !HasSign;
    auto [state_name, res_name] = simplify(SimplifyTermFlags::none, ctx, lit.name());
    auto res_elems = Util::ResultVec{lit.elems()};
    for (auto const &elem : lit.elems()) {
        auto [state_elem, res_elem] = simplify_element(ctx, elem);
        if (state_elem == TruthValue::bot) {
            res_elems.remove();
            continue;
        }
        res_elems.update(std::move(res_elem));
    }
    if (!state_name) {
        return {head ? TruthValue::top : TruthValue::bot, SimpleHBLiteral<head>{make_constant(location(lit), head)}};
    }
    if constexpr (head) {
        return {TruthValue::unknown, Util::update<HeadTheoryAtom>(lit.loc(), UPA{lit.name(), std::move(res_name)},
                                                                  std::move(res_elems), lit.rhs())};
    } else {
        return {TruthValue::unknown,
                Util::update<BodyTheoryAtom>(lit.loc(), lit.sign_, UPA{lit.name(), std::move(res_name)},
                                             std::move(res_elems), lit.rhs())};
    }
}

//! Simplify head literals.
struct SimplifyHeadLiteral {
    auto operator()(auto const &lit) const -> SimplifyResult<HeadLiteral> = delete;

    auto operator()(HeadLiteral const &lit) const -> SimplifyResult<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> SimplifyResult<HeadLiteral> {
        auto [state, res] = simplify(SimplifyLiteralFlags::head, ctx, lit.lit_);
        return {state, Util::transform(std::move(res), [](auto &&res) { return SimpleHeadLiteral{GRINGO_FWD(res)}; })};
    }

    auto operator()(Disjunction const &lit) const -> SimplifyResult<HeadLiteral> {
        auto state_fixed = TruthValue::top;
        auto state_empty = TruthValue::bot;
        auto state_elems = state_empty;

        auto res_elems = Util::ResultVec{lit.elems_};
        for (auto const &elem : lit.elems_) {
            std::visit(
                [&, this](auto const &elem) {
                    auto [state, res_elem] = [&, this]() {
                        GRINGO_MATCH(elem, ConditionalLiteral) { return simplify_condlit(ctx, elem, false); }
                        else {
                            return simplify(SimplifyLiteralFlags::head, ctx, elem);
                        }
                    }();
                    if (state_elems == state_fixed) {
                        return;
                    }
                    if (state == state_empty) {
                        res_elems.remove();
                    } else if (state == TruthValue::unknown) {
                        state_elems = TruthValue::unknown;
                        res_elems.update(std::move(res_elem));
                    } else if (state == state_fixed) {
                        if (lit.elems_.size() != 1 || res_elem) {
                            res_elems.as_optional() =
                                Util::make_vec<Disjunction::Element>(std::move(res_elem).value_or(elem));
                        }
                        state_elems = state_fixed;
                    }
                },
                elem);
        }
        if (state_elems != TruthValue::unknown) {
            return {state_elems, SimpleHeadLiteral{make_constant(lit.loc(), state_elems == TruthValue::top)}};
        }
        return {state_elems, Util::transform(std::move(res_elems).as_optional(), [&](auto value) {
                    return Disjunction{lit.loc(), std::move(value)};
                })};
    }
    auto operator()(HeadSetAggregate const &lit) const -> SimplifyResult<HeadLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("set aggregates must be unpooled before simplifying");
    }

    auto operator()(HeadAggregate const &lit) const -> SimplifyResult<HeadLiteral> {
        return simplify_aggregate<true>(ctx, lit);
    }

    auto operator()(HeadTheoryAtom const &lit) const -> SimplifyResult<HeadLiteral> {
        return simplify_theory_atom(ctx, lit);
    }

    RewriteContext &ctx;
};

//! Simplify body literals.
struct SimplifyBodyLiteral {
    auto operator()(auto const &lit) const -> SimplifyResult<BodyLiteral> = delete;

    auto operator()(BodyLiteral const &lit) const -> SimplifyResult<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> SimplifyResult<BodyLiteral> {
        auto [state, res] = simplify(SimplifyLiteralFlags::matchable, ctx, lit.lit_);
        return {state, Util::transform(std::move(res), [](auto &&res) { return SimpleBodyLiteral{GRINGO_FWD(res)}; })};
    }

    auto operator()(Conjunction const &lit) const -> SimplifyResult<BodyLiteral> {
        return simplify_condlit(ctx, lit.lit_, true);
    }

    auto operator()(BodySetAggregate const &lit) const -> SimplifyResult<BodyLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("set aggregates must be unpooled before simplifying");
    }

    auto operator()(BodyAggregate const &lit) const -> SimplifyResult<BodyLiteral> {
        return simplify_aggregate<false>(ctx, lit);
    }

    auto operator()(BodyTheoryAtom const &lit) const -> SimplifyResult<BodyLiteral> {
        return simplify_theory_atom(ctx, lit);
    }

    RewriteContext &ctx;
};

//! Simplify a vector of body literals.
[[nodiscard]] auto simplify_body(RewriteContext &ctx, BodyLiteralVec const &body) -> SimplifyResult<BodyLiteralVec> {
    auto res_body = Util::ResultVec{body};
    auto state_body = TruthValue::top;
    for (auto const &lit : body) {
        auto [state_lit, res_lit] = simplify(ctx, lit);
        // ensure that all literals are processed to emit all messages
        if (state_body == TruthValue::bot) {
            continue;
        }
        if (state_lit == TruthValue::top) {
            res_body.remove();
        } else {
            res_body.update(std::move(res_lit));
        }
        if (state_lit == TruthValue::bot) {
            if (body.size() != 1) {
                res_body.as_optional() =
                    Util::make_vec<BodyLiteral>(SimpleBodyLiteral{LiteralBoolean{location(lit), Sign::none, false}});
            }
            state_body = TruthValue::bot;
        } else if (state_lit == TruthValue::unknown) {
            state_body = TruthValue::unknown;
        }
    }
    if (state_body != TruthValue::bot) {
        extend(res_body, ctx.aux());
    }
    return {state_body, std::move(res_body).as_optional()};
}

//! Simplify statements.
struct SimplifyStatement {
    auto operator()(auto const &lit) const -> SimplifyResult<Statement> = delete;

    auto operator()(Statement const &stm) const -> SimplifyResult<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> SimplifyResult<Statement> {
        auto [state_head, res_head] = simplify(ctx, stm.head_);
        auto [state_body, res_body] = simplify_body(ctx, stm.body_);
        auto state = TruthValue::unknown;
        if (state_head == TruthValue::top || state_body == TruthValue::bot) {
            if (!stm.body_.empty()) {
                res_head = make_constant(location(stm.head_), true);
                res_body = BodyLiteralVec{};
            }
            state = TruthValue::top;
        }
        if (state_head == TruthValue::bot && state_body == TruthValue::top) {
            state = TruthValue::bot;
        }
        if (res_head.has_value() || res_body.has_value()) {
            return {state,
                    Rule{stm.loc(), std::move(res_head).value_or(stm.head_), std::move(res_body).value_or(stm.body_)}};
        }
        return {state};
    }

    auto operator()(TheoryDefinition const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementOptimize const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::runtime_error("optimize statements must be unpooled first");
    }

    auto operator()(StatementWeakConstraint const &stm) const -> SimplifyResult<Statement> {
        auto [state_weight, res_weight] = simplify(SimplifyTermFlags::none, ctx, stm.tuple_.weight_);
        auto [state_prio, res_prio] = stm.tuple_.priority_
                                          ? simplify(SimplifyTermFlags::none, ctx, *stm.tuple_.priority_)
                                          : SimplifyTermResult{true};
        auto [state_terms, res_terms] = simplify_termvec(ctx, stm.tuple_.terms_);
        auto [state_body, res_body] = simplify_body(ctx, stm.body_);

        if (!state_weight || state_body == TruthValue::bot || !state_prio || !state_terms) {
            return {TruthValue::top, Rule{stm.loc(), make_constant(location(stm), true), {}}};
        }
        auto state = TruthValue::unknown;
        if (res_weight.has_value() || res_body.has_value() || res_prio.has_value() || res_terms.has_value()) {
            if (!res_prio && stm.tuple_.priority_) {
                res_prio = stm.tuple_.priority_;
            }
            return {state, StatementWeakConstraint{
                               stm.loc(), std::move(res_body).value_or(stm.body_),
                               StatementWeakConstraint::Tuple{std::move(res_weight).value_or(stm.tuple_.weight_),
                                                              std::move(res_prio),
                                                              std::move(res_terms).value_or(stm.tuple_.terms_)}

                           }};
        }
        return {state};
    }

    auto operator()(StatementShow const &stm) const -> SimplifyResult<Statement> {
        auto [state_term, res_term] = simplify(SimplifyTermFlags::none, ctx, stm.term_);
        auto [state_body, res_body] = simplify_body(ctx, stm.body_);
        if (!state_term || state_body == TruthValue::bot) {
            return {TruthValue::top, Rule{stm.loc(), make_constant(location(stm.term_), true), {}}};
        }
        auto state = TruthValue::unknown;
        if (res_term.has_value() || res_body.has_value()) {
            return {state, StatementShow{stm.loc(), std::move(res_term).value_or(stm.term_),
                                         std::move(res_body).value_or(stm.body_)}};
        }
        return {state};
    }

    auto operator()(StatementShowSig const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementProject const &stm) const -> SimplifyResult<Statement> {
        auto [state_term, res_term] = simplify(SimplifyTermFlags::matchable, ctx, stm.term_);
        auto [state_body, res_body] = simplify_body(ctx, stm.body_);
        if (!state_term || state_body == TruthValue::bot) {
            return {TruthValue::top, Rule{stm.loc(), make_constant(location(stm.term_), true), {}}};
        }
        auto state = TruthValue::unknown;
        if (res_term.has_value() || res_body.has_value()) {
            return {state, StatementProject{stm.loc(), std::move(res_term).value_or(stm.term_),
                                            std::move(res_body).value_or(stm.body_)}};
        }
        return {state};
    }

    auto operator()(StatementProjectSig const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementDefined const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementExternal const &stm) const -> SimplifyResult<Statement> {
        auto [state_term, res_term] = simplify(SimplifyTermFlags::matchable, ctx, stm.term_);
        auto [state_type, res_type] =
            stm.type_ ? simplify(SimplifyTermFlags::matchable, ctx, *stm.type_) : SimplifyTermResult{true};
        auto [state_body, res_body] = simplify_body(ctx, stm.body_);
        if (!state_term || !state_type || state_body == TruthValue::bot) {
            return {TruthValue::top, Rule{stm.loc(), make_constant(location(stm.term_), true), {}}};
        }
        auto state = TruthValue::unknown;
        if (res_term.has_value() || res_type.has_value() || res_body.has_value()) {
            if (stm.type_ && !res_type) {
                res_type = stm.type_;
            }
            return {state, StatementExternal{stm.loc(), std::move(res_term).value_or(stm.term_),
                                             std::move(res_body).value_or(stm.body_), std::move(res_type)}};
        }
        return {state};
    }

    auto operator()(StatementEdge const &stm) const -> SimplifyResult<Statement> {
        if (stm.edges_.size() != 1) {
            throw std::runtime_error("edge directives must be unpooled before simplifying");
        }
        auto edge = stm.edges_.front();
        auto [state_u, res_u] = simplify(SimplifyTermFlags::none, ctx, edge.u_);
        auto [state_v, res_v] = simplify(SimplifyTermFlags::none, ctx, edge.v_);
        auto [state_body, res_body] = simplify_body(ctx, stm.body_);
        if (!state_u || !state_v || state_body == TruthValue::bot) {
            return {TruthValue::top, Rule{stm.loc(), make_constant(stm.loc(), true), {}}};
        }
        auto state = TruthValue::unknown;
        if (res_u.has_value() || res_v.has_value() || res_body.has_value()) {
            return {state, StatementEdge{stm.loc(),
                                         Util::make_vec<StatementEdge::Edge>(StatementEdge::Edge{
                                             std::move(res_u).value_or(edge.u_), std::move(res_v).value_or(edge.v_)}),
                                         std::move(res_body).value_or(stm.body_)}};
        }
        return {state};
    }

    auto operator()(StatementHeuristic const &stm) const -> SimplifyResult<Statement> {
        auto [state_atom, res_atom] = simplify(SimplifyTermFlags::matchable, ctx, stm.atom_);
        auto [state_mod, res_mod] = simplify(SimplifyTermFlags::none, ctx, stm.mod_);
        auto [state_type, res_type] = simplify(SimplifyTermFlags::none, ctx, stm.type_);
        auto [state_prio, res_prio] =
            stm.prio_ ? simplify(SimplifyTermFlags::none, ctx, *stm.prio_) : SimplifyTermResult{true};
        auto [state_body, res_body] = simplify_body(ctx, stm.body_);

        if (!state_atom || !state_mod || !state_type || !state_prio || state_body == TruthValue::bot) {
            return {TruthValue::top, Rule{stm.loc(), make_constant(stm.loc(), true), {}}};
        }
        auto state = TruthValue::unknown;
        if (res_atom.has_value() || res_mod.has_value() || res_type.has_value() || res_prio.has_value() ||
            res_body.has_value()) {
            if (!res_prio && stm.prio_) {
                res_prio = stm.prio_;
            }
            return {state,
                    StatementHeuristic{stm.loc(), std::move(res_atom).value_or(stm.atom_),
                                       std::move(res_body).value_or(stm.body_), std::move(res_type).value_or(stm.type_),
                                       std::move(res_prio), std::move(res_mod).value_or(stm.mod_)}};
        }
        return {state};
    }

    auto operator()(StatementScript const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementInclude const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementProgram const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementConst const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::runtime_error("const statementments must be extracted first");
    }

    auto operator()(Comment const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    RewriteContext &ctx; //!< Context used during simplification.
};

} // namespace

[[nodiscard]] auto simplify(SimplifyTermFlags flags, RewriteContext &ctx, Term const &term) -> SimplifyTermResult {
    auto make_matchable = [&](auto &&target, bool self = true) -> SimplifyTermResult {
        if (test(flags, SimplifyTermFlags::matchable)) {
            if (auto ret = MakeMatchableTerm{ctx}(target, flags); ret.has_value()) {
                return {true, std::move(ret).value()};
            }
        }
        if (self && target != term) {
            return {true, std::forward<decltype(target)>(target)};
        }
        return {true};
    };
    return std::visit(
        [&](auto &&res) -> SimplifyTermResult {
            GRINGO_MATCH(res, TermResultFail) { return {false}; }
            GRINGO_MATCH(res, TermResultUnchanged) { return make_matchable(term, false); }
            GRINGO_MATCH(res, TermResultSymbol) {
                auto sym = Term{TermSymbol{location(term), res}};
                if (sym != term) {
                    return {true, std::move(sym)};
                }
                return {true};
            }
            GRINGO_MATCH(res, TermResultChanged) { return make_matchable(res.term); }
            GRINGO_MATCH(res, TermResultLinear) { return make_matchable(linear_as_term(ctx, std::move(res), false)); }
        },
        SimplifyTerm{ctx}(term, flags));
}

[[nodiscard]] auto simplify(SimplifyLiteralFlags flags, RewriteContext &ctx, Literal const &lit)
    -> SimplifyResult<Literal> {
    return SimplifyLiteral{ctx}(lit, flags);
}

[[nodiscard]] auto simplify(RewriteContext &ctx, HeadLiteral const &lit) -> SimplifyResult<HeadLiteral> {
    return SimplifyHeadLiteral{ctx}(lit);
}

[[nodiscard]] auto simplify(RewriteContext &ctx, BodyLiteral const &lit) -> SimplifyResult<BodyLiteral> {
    return SimplifyBodyLiteral{ctx}(lit);
}

[[nodiscard]] auto simplify(RewriteContext &ctx, Statement const &stm) -> SimplifyResult<Statement> {
    auto guard = ctx.push();
    return SimplifyStatement{ctx}(stm);
}

} // namespace Gringo::Input
