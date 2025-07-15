#include <clingo/input/print.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/evaluate.hh>
#include <clingo/input/rewrite/simplify.hh>
#include <clingo/input/rewrite/visit_variables.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/checked_math.hh>
#include <clingo/util/optional.hh>
#include <clingo/util/type_traits.hh>

#include <algorithm>
#include <ctime>
#include <utility>

namespace CppClingo::Input {

namespace {

//! Extend the contained vector with the given assignments.
template <class R> void extend(R &res, AuxTermVec &aux, bool conjunctive = true) {
    for (auto &[lhs, rhs] : aux) {
        auto loc = location(lhs);
        auto rel = conjunctive ? Relation::equal : Relation::not_equal;
        auto lit = LitComparison{loc, Sign::none, std::move(lhs), Util::make_vec<Guard>(Guard{rel, std::move(rhs)})};
        if constexpr (std::is_same_v<typename R::ValueType, Lit>) {
            res.append(std::move(lit));
        } else {
            res.append(BdLitSimple{std::move(lit)});
        }
    }
}

//! Return a Boolean literal with the given location and truth value.
[[nodiscard]] auto make_constant(Location loc, bool truth) -> Lit {
    return LitBool{std::move(loc), Sign::none, truth};
}

//! Ensure that the term only matches numbers.
[[nodiscard]] auto as_linear_term(Term term) -> Term {
    auto loc = location(term);
    term = TermBinary(loc, TermSymbol{loc, CppClingo::SymbolStore::num_ref(1)}, BinaryOperator::times, std::move(term));
    return TermBinary(loc, std::move(term), BinaryOperator::plus, TermSymbol{loc, CppClingo::SymbolStore::num_ref(0)});
}

//! Introduce a fresh variable for the given term.
//!
//! If linear is true, the term is assumed to be a number
//! and a linear term instead of a variable is returned.
[[nodiscard]] auto map_term(RewriteContext &ctx, Term term, bool linear = false) -> Term {
    auto loc = location(term);
    ctx.aux().emplace_back(TermVariable{loc, ctx.gen().new_name()}, std::move(term));
    return linear ? as_linear_term(ctx.aux().back().first) : ctx.aux().back().first;
}

//! Simplify a term vector.
[[nodiscard]] auto simplify_termvec(RewriteContext &ctx, TermArray const &terms) -> Util::ResultState<TermArray> {
    auto state_terms = true;
    auto res_terms = Util::ResultVec{terms};
    for (auto const &term : terms) {
        auto [state_term, res_term] = simplify(SimplifyTermFlags::none, ctx, term);
        state_terms = state_terms && state_term;
        if (state_terms) {
            res_terms.update(std::move(res_term));
        }
    }
    if (!state_terms) {
        return {false};
    }
    return {true, std::move(res_terms).as_optional()};
}

[[nodiscard]] auto all_symbol(TermArray const &terms) -> bool {
    return std::ranges::all_of(terms, is_symbol);
}

//! The detected type of a term.
enum class TermType : uint8_t {
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
        mxn = TermBinary(loc, TermSymbol{loc, ctx.store().num_ref(std::move(res.m))}, BinaryOperator::times,
                         std::move(mxn));
    }
    if (!simplify || res.n != 0) {
        mxn = TermBinary(loc, std::move(mxn), BinaryOperator::plus,
                         TermSymbol{loc, ctx.store().num_ref(std::move(res.n))});
    }
    return mxn;
}

//! Convert term results to terms reusing the old term if possible.
class ResultAsTerm {
  public:
    ResultAsTerm(RewriteContext &ctx, Term const &term) : ctx_{&ctx}, term_{&term} {}

    //! Convert a linear result.
    [[nodiscard]] auto operator()(TermResultLinear res) -> Term {
        auto ret = linear_as_term(*ctx_, std::move(res));
        return ret != *term_ ? std::move(ret) : static_cast<Term>(*term_);
    }

    //! Convert a failed result.
    [[nodiscard]] auto operator()([[maybe_unused]] TermResultFail res) const -> Term {
        throw std::logic_error("cannot happen");
    }

    //! Convert an unchanged result.
    [[nodiscard]] auto operator()([[maybe_unused]] TermResultUnchanged res) const -> Term { return *term_; }

    //! Convert a changed result.
    [[nodiscard]] auto operator()(TermResultChanged res) const -> Term { return std::move(res.term); }

    //! Convert a symbol result.
    [[nodiscard]] auto operator()(TermResultSymbol res) const -> Term {
        Term ret = TermSymbol{location(*term_), res};
        return ret != *term_ ? std::move(ret) : static_cast<Term>(*term_);
    }

  private:
    //! The rewrite context.
    RewriteContext *ctx_;
    //! The original term.
    Term const *term_;
};

//! Convert terms of form V and -V where V is a variable to linear terms.
class VarToLinear {
  public:
    VarToLinear(Term const &term) : term_{&term} {}

    //! Handle remaining term results.
    auto operator()(auto res) const -> TermResult { return res; }

    //! Handle unchanged results.
    auto operator()(TermResultUnchanged res) const -> TermResult {
        if (std::holds_alternative<TermVariable>(*term_)) {
            return TermResultLinear{*term_, Number(1), Number(0)};
        }
        auto const *term_unary = std::get_if<TermUnary>(term_);
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

  private:
    //! The original term.
    Term const *term_;
};

//! Result indicating a changed tuple.
using TupleResultChanged = std::vector<std::variant<Projection, Symbol, Term>>;
//! Result indicating an unchanged tuple.
struct TupleResultUnchanged {};
//! Result indicating a tuples that failed to simplify.
struct TupleResultFail {};
//! Variant for the different tuple evaluation results.
using TupleResult = std::variant<TupleResultFail, TupleResultUnchanged, TupleResultChanged>;

//! Convert the given simplified arguments to a symbol vector.
//!
//! The result vector must only store symbols.
auto result_as_symbol_vec(TupleResultChanged const &args_tuple) -> std::vector<Symbol> {
    std::vector<Symbol> args;
    args.reserve(args_tuple.size());
    for (auto const &arg : args_tuple) {
        args.emplace_back(std::get<Symbol>(arg));
    }
    return args;
}

//! Convert the given simplified arguments to term tuple.
auto result_as_tuple(ArgumentTuple const &tuple, TupleResultChanged args_tuple) -> ArgumentTuple {
    std::vector<Argument> args;
    auto it = tuple.elems().begin();
    args.reserve(tuple.elems().size());
    for (auto &arg : args_tuple) {
        std::visit(
            [&]<class T>(T val) {
                if constexpr (std::is_same_v<T, Symbol>) {
                    args.emplace_back(TermSymbol{location(std::get<Term>(*it)), val});
                } else {
                    args.emplace_back(std::move(val));
                }
            },
            std::move(arg));
        ++it;
    }
    return ArgumentTuple{std::move(args)};
}

//! Simplify terms.
class SimplifyTerm {
  public:
    SimplifyTerm(RewriteContext &ctx) : ctx_{&ctx} {}

    //! Helper to simplify the arguments of the tuple.
    //!
    //! The resulting vector is nullopt if there were no simplifications.
    //! Otherwise, each element is either a projection, a symbol if it could
    //! evaluated right away, or a term in case of some other simplification.
    auto handle_tuple(SimplifyTermFlags flags, ArgumentTuple const &tuple, bool &constant) const -> TupleResult {
        auto n = std::ptrdiff_t{0};

        TupleResult res_tuple = TupleResultUnchanged{};

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

        auto simplify = [&, this]<class T>(T const &arg) -> bool {
            // projected argument
            if constexpr (std::is_same_v<T, Projection>) {
                constant = false;
                init().emplace_back(arg);
                return true;
            }
            // term argument
            if constexpr (std::is_same_v<T, Term>) {
                auto simplify = [&]<class U>(U res) -> bool {
                    // evaluation of argument failed
                    if constexpr (std::is_same_v<U, TermResultFail>) {
                        return false;
                    }
                    // argument evaluated to symbol
                    if constexpr (std::is_same_v<U, TermResultSymbol>) {
                        // see note at check_change for function/tuple visitor
                        init().emplace_back(res);
                    } else {
                        constant = false;
                    }
                    if constexpr (std::is_same_v<U, TermResultLinear>) {
                        init().emplace_back(linear_as_term(*ctx_, std::move(res), false));
                    }
                    // argument did not change
                    if constexpr (std::is_same_v<U, TermResultUnchanged>) {
                        if (auto *res_changed = std::get_if<TupleResultChanged>(&res_tuple); res_changed != nullptr) {
                            res_changed->emplace_back(arg);
                        }
                    }
                    // argument changed
                    if constexpr (std::is_same_v<U, TermResultChanged>) {
                        init().emplace_back(std::move(res.term));
                    }
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
    auto operator()(auto const &term, SimplifyTermFlags flags) const = delete;

    //! Simplify the given symbolic term.
    auto operator()(TermSymbol const &term, [[maybe_unused]] SimplifyTermFlags flags) const -> TermResult {
        return term.value();
    }

    //! Simplify the given variable.
    auto operator()([[maybe_unused]] TermVariable const &term, [[maybe_unused]] SimplifyTermFlags flags) const
        -> TermResult {
        // a variable can represent any term
        return TermType::any;
    }

    //! Simplify the given function term.
    auto operator()(TermFunction const &term, SimplifyTermFlags flags) const -> TermResult {
        if (term.pool().size() != 1) {
            throw std::runtime_error("functions must be unpooled before simplifying");
        }

        bool preserve = intersects(flags, SimplifyTermFlags::preserve_toplevel);

        flags &= ~SimplifyTermFlags::preserve_toplevel;

        bool constant = !term.external();
        auto type = term.external() ? TermType::any : TermType::symbolic;
        auto const &tuple = term.pool().front();

        // simplify arguments
        return std::visit(
            [&, this]<class T>(T res) -> TermResult {
                if constexpr (std::is_same_v<T, TupleResultFail>) {
                    return TermResultFail{};
                }
                if constexpr (std::is_same_v<T, TupleResultUnchanged>) {
                    if (term.external() && !preserve) {
                        return TermResultChanged{type, map_term(*ctx_, term)};
                    }
                    if (!constant) {
                        return type;
                    }
                    return ctx_->store().fun_ref(term.name(), {}, false);
                }
                if constexpr (std::is_same_v<T, TupleResultChanged>) {
                    if (!constant) {
                        auto fun = term.update(a_pool = PoolArray{result_as_tuple(tuple, std::move(res))});
                        if (term.external() && !preserve) {
                            return TermResultChanged{type, map_term(*ctx_, std::move(fun))};
                        }
                        // Note: this is somewhat inefficient because the
                        // equality comparison recurses into the structure
                        return check_change(type, term, std::move(fun));
                    }
                    return ctx_->store().fun_ref(term.name(), result_as_symbol_vec(std::move(res)), false);
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
            [&, this]<class T>(T res) -> TermResult {
                if constexpr (std::is_same_v<T, TupleResultFail>) {
                    return TermResultFail{};
                }
                if constexpr (std::is_same_v<T, TupleResultUnchanged>) {
                    // unchanged term that did not evaluate to a symbol
                    if (!constant) {
                        return type;
                    }
                    return ctx_->store().tup_ref(result_as_symbol_vec({}));
                }
                if constexpr (std::is_same_v<T, TupleResultChanged>) {
                    // changed term that did not evaluate to a symbol
                    if (!constant) {
                        // Note: this is somewhat inefficient because the
                        // equality comparison recurses into the structure
                        return check_change(
                            type, term,
                            term.update(a_pool = Util::make_vec<TupleElement>(result_as_tuple(tuple, std::move(res)))));
                    }
                    return ctx_->store().tup_ref(result_as_symbol_vec(std::move(res)));
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

        auto simplify = [&term, this]<class T>(T res) -> TermResult {
            // evaluation of argument failed
            if constexpr (std::is_same_v<T, TermResultFail>) {
                return {};
            }
            // the argument evaluated to a symbol
            if constexpr (std::is_same_v<T, TermResultSymbol>) {
                if (res.type() != SymbolType::number) {
                    CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                            << "  " << term << "\n";
                    return TermResultFail{};
                }
                return ctx_->store().num_ref(abs(res.num()));
            }
            if constexpr (std::is_same_v<T, TermResultLinear>) {
                std::vector<Term> pool;
                pool.emplace_back(linear_as_term(*ctx_, std::move(res)));
                return check_change(TermType::numeric, term, term.update(a_pool = std::move(pool)));
            }
            // the argument did not change
            if constexpr (std::is_same_v<T, TermResultUnchanged>) {
                if (res == TermType::symbolic || res == TermType::tuple) {
                    CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                            << "  " << term << "\n";
                    return TermResultFail{};
                }
                return TermResultUnchanged{TermType::numeric};
            }
            // the argument changed
            if constexpr (std::is_same_v<T, TermResultChanged>) {
                // handle invalid terms
                if (res.type == TermType::symbolic || res.type == TermType::tuple) {
                    CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                            << "  " << term << "\n";
                    return TermResultFail{};
                }
                // construct a new term
                std::vector<Term> pool;
                pool.emplace_back(std::move(res.term));
                return TermResultChanged{TermType::numeric, term.update(a_pool = std::move(pool))};
            }
        };

        return std::visit(simplify, operator()(term.pool().front(), flags));
    }

    //! Simplify the given unary term.
    auto operator()(TermUnary const &term, SimplifyTermFlags flags) const -> TermResult {
        flags &= ~SimplifyTermFlags::preserve_toplevel;

        auto simplify = [&term, this]<class T>(T res) -> TermResult {
            // evaluation of argument failed
            if constexpr (std::is_same_v<T, TermResultFail>) {
                return TermResultFail{};
            }
            // the argument evaluated to a symbol
            if constexpr (std::is_same_v<T, TermResultSymbol>) {
                // we can always evaluate constants
                auto opt_sym = evaluate(ctx_->store(), term.op(), res);
                if (!opt_sym.has_value()) {
                    CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                            << "  " << term << "\n";
                    return TermResultFail{};
                }
                return TermResultSymbol{opt_sym.value()};
            }
            if constexpr (std::is_same_v<T, TermResultLinear>) {
                if (term.op() == UnaryOperator::minus) {
                    res.m = -std::move(res.m);
                    res.n = -std::move(res.n);
                    return res;
                }
                return check_change(TermType::numeric, term,
                                    term.update(a_rhs = linear_as_term(*ctx_, std::move(res))));
            }
            // get type of term based on the given type of its argument
            auto check_type = [this, &term](TermType type) -> std::optional<TermType> {
                if (type == TermType::tuple || (term.op() == UnaryOperator::negate && type == TermType::symbolic)) {
                    CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                            << "  " << term << "\n";
                    return std::nullopt;
                }
                // ~term is always numeric
                return term.op() == UnaryOperator::negate ? TermType::numeric : type;
            };
            // simplify --symbolic to symbolic and ---any to -any
            auto fold = [&term](TermType type, Term const &rhs) -> std::optional<TermResultChanged> {
                if (term.op() != UnaryOperator::minus) {
                    return std::nullopt;
                }
                auto const *rhs_unary = std::get_if<TermUnary>(&rhs);
                if (rhs_unary == nullptr || rhs_unary->op() != UnaryOperator::minus) {
                    return std::nullopt;
                }
                // --symbolic
                if (type == TermType::symbolic) {
                    return TermResultChanged{type, rhs_unary->rhs()};
                }
                auto const *rhs_rhs_unary = std::get_if<TermUnary>(&rhs_unary->rhs().get());
                if (rhs_rhs_unary == nullptr || rhs_rhs_unary->op() != UnaryOperator::minus) {
                    return std::nullopt;
                }
                // --any
                return TermResultChanged{type, rhs_unary->rhs()};
            };
            // the argument did not change
            if constexpr (std::is_same_v<T, TermType>) {
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
            if constexpr (std::is_same_v<T, TermResultChanged>) {
                auto type = check_type(res.type);
                if (!type.has_value()) {
                    return TermResultFail{};
                }
                // fold if possible
                if (auto opt_res = fold(type.value(), res.term); opt_res.has_value()) {
                    return std::move(opt_res).value();
                }
                return TermResultChanged{type.value(), term.update(a_rhs = std::move(res.term))};
            }
        };
        return std::visit(simplify, operator()(*term.rhs(), flags));
    }

    //! Simplify the given binary term.
    auto operator()(TermBinary const &term, SimplifyTermFlags flags) const -> TermResult {
        // check if the result can evaluate to a number
        auto is_numeric = []<class T>(T const &res) -> bool {
            if constexpr (std::is_same_v<T, TermResultFail>) {
                return false;
            }
            if constexpr (std::is_same_v<T, TermResultLinear>) {
                return true;
            }
            if constexpr (std::is_same_v<T, TermResultUnchanged>) {
                return res == TermType::any || res == TermType::numeric;
            }
            if constexpr (std::is_same_v<T, TermResultChanged>) {
                return res.type == TermType::any || res.type == TermType::numeric;
            }
            if constexpr (std::is_same_v<T, TermResultSymbol>) {
                return res.type() == SymbolType::number;
            }
        };

        if (term.op() == BinaryOperator::dots) {
            auto simplify = [&, this]<class L, class R>(L &&res_lhs, R &&res_rhs) -> TermResult {
                // check arguments
                if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                    CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                            << "  " << term << "\n";
                    return {};
                }
                if (intersects(flags, SimplifyTermFlags::preserve_toplevel)) {
                    return check_change(TermType::numeric, term,
                                        term.update(a_lhs = ResultAsTerm{*ctx_, term.lhs()}(std::forward<L>(res_lhs)),
                                                    a_op = BinaryOperator::dots,
                                                    a_rhs = ResultAsTerm{*ctx_, term.rhs()}(std::forward<R>(res_rhs))));
                }
                // Note: If the surrounding term does not have to be matchable,
                // then the variable can be returned as is.
                auto var =
                    map_term(*ctx_, term.update(a_lhs = ResultAsTerm{*ctx_, term.lhs()}(std::forward<L>(res_lhs)),
                                                a_op = BinaryOperator::dots,
                                                a_rhs = ResultAsTerm{*ctx_, term.rhs()}(std::forward<R>(res_rhs))));
                if (intersects(flags, SimplifyTermFlags::matchable)) {
                    return TermResultLinear{std::move(var), Number{1}, Number{0}};
                }
                return TermResultChanged{TermType::numeric, std::move(var)};
            };
            return std::visit(simplify, operator()(*term.lhs(), flags), operator()(*term.rhs(), flags));
        }
        flags &= ~SimplifyTermFlags::preserve_toplevel;

        auto simplify = [&, this]<class T, class U>(T res_lhs, U res_rhs) -> TermResult {
            // check arguments
            if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                return {};
            }

            // evaluate to symbol
            if constexpr (std::is_same_v<T, Symbol> && std::is_same_v<U, Symbol>) {
                auto res = evaluate(ctx_->store(), res_lhs, term.op(), res_rhs);
                if (!res.has_value()) {
                    CLINGO_REPORT_LOC(ctx_->logger(), info_operation_undefined, term.loc()) << "operation undefined:\n"
                                                                                            << "  " << term << "\n";
                    return TermResultFail{};
                }
                return res.value();
            }
            if constexpr (std::is_same_v<T, Symbol> && std::is_same_v<U, TermResultLinear>) {
                if (term.op() == BinaryOperator::plus) {
                    res_rhs.n += res_lhs.num();
                    return res_rhs;
                }
                if (term.op() == BinaryOperator::minus) {
                    res_rhs.m = -std::move(res_rhs.m);
                    res_rhs.n = res_lhs.num() - std::move(res_rhs.n);
                    return res_rhs;
                }
                if (term.op() == BinaryOperator::times && res_lhs.num() != 0) {
                    res_rhs.m *= res_lhs.num();
                    res_rhs.n *= res_lhs.num();
                    return res_rhs;
                }
                return check_change(TermType::numeric, term,
                                    term.update(a_lhs = ResultAsTerm{*ctx_, term.lhs()}(std::move(res_lhs)),
                                                a_rhs = ResultAsTerm{*ctx_, term.rhs()}(std::move(res_rhs))));
            }
            if constexpr (std::is_same_v<T, TermResultLinear> && std::is_same_v<U, Symbol>) {
                if (term.op() == BinaryOperator::plus) {
                    res_lhs.n += res_rhs.num();
                    return res_lhs;
                }
                if (term.op() == BinaryOperator::minus) {
                    res_lhs.n -= res_rhs.num();
                    return res_lhs;
                }
                if (term.op() == BinaryOperator::times && res_rhs.num() != 0) {
                    res_lhs.m *= res_rhs.num();
                    res_lhs.n *= res_rhs.num();
                    return res_lhs;
                }
                return check_change(TermType::numeric, term,
                                    term.update(a_lhs = ResultAsTerm{*ctx_, term.lhs()}(std::move(res_lhs)),
                                                a_rhs = ResultAsTerm{*ctx_, term.rhs()}(std::move(res_rhs))));
            }
            if constexpr (std::is_same_v<T, TermResultLinear> && std::is_same_v<U, TermResultLinear>) {
                if (term.op() == BinaryOperator::plus) {
                    if (res_lhs.x == res_rhs.x) {
                        res_lhs.n += res_rhs.n;
                        res_lhs.m += res_rhs.m;
                        return res_lhs;
                    }
                    res_rhs.n += res_lhs.n;
                    res_lhs.n = Number(0);
                }
                if (term.op() == BinaryOperator::minus) {
                    if (res_lhs.x == res_rhs.x) {
                        res_lhs.n -= res_rhs.n;
                        res_lhs.m -= res_rhs.m;
                        return res_lhs;
                    }
                    res_rhs.n -= res_lhs.n;
                    res_lhs.n = Number(0);
                }
                return check_change(TermType::numeric, term,
                                    term.update(a_lhs = linear_as_term(*ctx_, std::move(res_lhs)),
                                                a_rhs = linear_as_term(*ctx_, std::move(res_rhs))));
            }

            // none of the arguments changed
            if constexpr (std::is_same_v<T, TermType> && std::is_same_v<U, TermType>) {
                return TermType::numeric;
            }

            // at least one of the arguments changed
            return check_change(TermType::numeric, term,
                                term.update(a_lhs = ResultAsTerm{*ctx_, term.lhs()}(std::move(res_lhs)),
                                            a_rhs = ResultAsTerm{*ctx_, term.rhs()}(std::move(res_rhs))));
        };

        // construct result
        return std::visit(simplify, std::visit(VarToLinear{term.lhs()}, operator()(*term.lhs(), flags)),
                          std::visit(VarToLinear{term.rhs()}, operator()(*term.rhs(), flags)));
    }

  private:
    RewriteContext *ctx_; //!< Context used during simplification.
};

//! Make a term matchable by removing terms that cannot be matched.
//!
//! If the unfailable flag is set, all terms that can evaluate to undefined are
//! removed as well. Only produces a result if one of the arguments changed;
//! there are no failures to handle.
class MakeMatchableTerm {
  public:
    using Result = std::optional<Term>;
    using ResultTuple = std::optional<std::vector<Argument>>;

    MakeMatchableTerm(RewriteContext &ctx) : ctx_{&ctx} {}

    //! Make the arguments of the given tuple matchable.
    [[nodiscard]] auto handle_tuple(SimplifyTermFlags flags, ArgumentTuple const &tuple) const -> ResultTuple {
        size_t n = 0;

        ResultTuple res_tuple;

        // helper to initialize the optional result vector
        auto init = [&]() -> std::vector<Argument> & {
            if (!res_tuple.has_value()) {
                res_tuple = Util::copy_n(tuple.elems(), n);
            }
            return *res_tuple;
        };

        auto handle_argument = [&, this]<class T>(T const &arg) -> void {
            // projected argument
            if constexpr (std::is_same_v<T, Projection>) {
                if (res_tuple.has_value()) {
                    init().emplace_back(arg);
                }
            }
            // term argument
            if constexpr (std::is_same_v<T, Term>) {
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
    auto operator()([[maybe_unused]] TermSymbol const &term, [[maybe_unused]] SimplifyTermFlags flags) const -> Result {
        return std::nullopt;
    }

    //! Make the given variable term matchable.
    auto operator()([[maybe_unused]] TermVariable const &term, [[maybe_unused]] SimplifyTermFlags flags) const
        -> Result {
        return std::nullopt;
    }

    //! Make the given function term matchable.
    auto operator()(TermFunction const &term, SimplifyTermFlags flags) const -> Result {
        assert(term.pool().size() == 1);
        return Util::transform(handle_tuple(flags, term.pool().front()), [&term]<class A>(A &&args) {
            return term.update(a_pool = Util::make_immutable_array<ArgumentTuple>(std::forward<A>(args)));
        });
    }

    //! Make the given tuple term matchable.
    auto operator()(TermTuple const &term, SimplifyTermFlags flags) const -> Result {
        assert(term.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(term.pool().front()));
        return Util::transform(handle_tuple(flags, std::get<ArgumentTuple>(term.pool().front())), [&term]<class A>(
                                                                                                      A &&args) {
            return term.update(a_pool = Util::make_immutable_array<TupleElement>(ArgumentTuple{std::forward<A>(args)}));
        });
    }

    //! Make the given absolute term matchable.
    auto operator()(TermAbs const &term, SimplifyTermFlags flags) const -> Result {
        if (!intersects(flags, SimplifyTermFlags::unfailable) &&
            intersects(flags, SimplifyTermFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(*ctx_, term, !intersects(flags, SimplifyTermFlags::unfailable));
    }

    //! Make the given unary term matchable.
    auto operator()(TermUnary const &term, SimplifyTermFlags flags) const -> Result {
        if (!intersects(flags, SimplifyTermFlags::unfailable) && term.op() == UnaryOperator::minus) {
            return Util::transform(operator()(term.rhs(), flags),
                                   [&term](auto arg) -> Term { return term.update(a_rhs = std::move(arg)); });
        }
        if (!intersects(flags, SimplifyTermFlags::unfailable) &&
            intersects(flags, SimplifyTermFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(*ctx_, term, !intersects(flags, SimplifyTermFlags::unfailable) && always_numeric(term));
    }

    //! Make the given binary term matchable.
    auto operator()(TermBinary const &term, SimplifyTermFlags flags) const -> Result {
        if (is_linear(term)) {
            // The goal here is to avoid adding additional assignments for auxiliary variables
            // that correspond to variables having a numeric value.
            if (intersects(flags, SimplifyTermFlags::unfailable)) {
                const auto &n = std::get<TermSymbol>(*term.rhs());
                const auto &mx = std::get<TermBinary>(*term.lhs());
                const auto &m = std::get<TermSymbol>(*mx.lhs());
                if (n.value().num() == 0 && m.value().num() == 1) {
                    for (auto &[lhs, rhs] : ctx_->aux()) {
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
        if (!intersects(flags, SimplifyTermFlags::unfailable) &&
            intersects(flags, SimplifyTermFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(*ctx_, term, !intersects(flags, SimplifyTermFlags::unfailable));
    }

  private:
    RewriteContext *ctx_; //!< Context used during simplification.
};

//! Simplify literals.
//!
//! Does not return a value if the literal did not change.
class SimplifyLiteral {
  public:
    SimplifyLiteral(RewriteContext &ctx) : ctx_{&ctx} {}

    //! Simplify literals dispatching based on type stored in variant.
    auto operator()(Lit const &lit, SimplifyLiteralFlags flags) const -> SimplifyResult<Lit> {
        return std::visit(*this, lit, std::variant<SimplifyLiteralFlags>{flags});
    }

    //! Simplify Boolean literals.
    //!
    //! Ensures that the literal is either true or false.
    auto operator()(LitBool const &lit, [[maybe_unused]] SimplifyLiteralFlags flags) const -> SimplifyResult<Lit> {
        auto value = (lit.sign() != Sign::once) == lit.value();
        auto state = value ? TruthValue::top : TruthValue::bot;

        if (lit.sign() != Sign::none) {
            return {state, make_constant(lit.loc(), value)};
        }
        return {state};
    }

    //! Simplify relation literals.
    //!
    //! The function ensures the following properties:
    //! (1) both sides of assignments are matchable (if parts of the terms are matchable),
    //! (2) terms in disjunctive non-binary relations cannot evaluate to empty pools.
    //! The latter is important to ensure that relations can be split into multiple rules
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
    auto operator()(LitComparison const &lit, SimplifyLiteralFlags flags) const -> SimplifyResult<Lit> {
        // whether pools are treated disjunctively or conjunctively
        bool head = intersects(flags, SimplifyLiteralFlags::head);
        // whether the elements of the relation are disjunctive or conjunctive
        // (after applying the sign)
        bool disjunctive = head != (lit.sign() == Sign::once);

        auto fixed_flags = SimplifyTermFlags::none;
        if (lit.rhs().size() > 1 && disjunctive) {
            // ensure that unpooling preserves terms that can fail
            fixed_flags = SimplifyTermFlags::matchable | SimplifyTermFlags::unfailable;
        }
        // the relation symbol that corresponds to assignment
        // (in the head it is inequality)
        auto assign = disjunctive ? Relation::not_equal : Relation::equal;

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

        // the truth value of the relation literal if all (signed) comparisons are true
        auto state = lit.sign() != Sign::once ? TruthValue::top : TruthValue::bot;
        // the truth value of the literal fixed by one of the  comparisons
        auto state_fixed = lit.sign() != Sign::once ? TruthValue::bot : TruthValue::top;
        // the truth value if evaluation of a term fails
        auto state_fail = head ? TruthValue::top : TruthValue::bot;

        // simplify lhs
        auto match_flags = SimplifyTermFlags::none;
        if (lit.rhs().front().first == assign) {
            match_flags = SimplifyTermFlags::matchable | SimplifyTermFlags::nested_matchable;
        }

        // binary assignment
        if (lit.rhs().size() == 1 && lit.rhs().front().first == assign) {
            // Note: in theory the left hand side could even be a more complex term that
            // is made machable (but not nested matchable).
            if (!is_variable(lit.lhs()) && is_variable(lit.rhs().front().second)) {
                auto inv = lit.update(a_lhs = lit.rhs().front().second,
                                      a_rhs = Util::make_vec<Guard>(Guard{assign, lit.lhs()}));
                auto res = operator()(inv, flags);
                if (!res.value.has_value()) {
                    res.value = std::move(inv);
                }
                return res;
            }

            if (lit.rhs().size() == 1 && lit.rhs().front().first == assign && is_variable(lit.lhs())) {
                fixed_flags |= SimplifyTermFlags::preserve_toplevel;
            }
        }

        auto [succeeded, res_lhs] = simplify(fixed_flags | match_flags, *ctx_, lit.lhs());

        // simplify rhs
        auto res_rhs = Util::ResultVec{lit.rhs()};
        auto prev_symbol = get_constant(lit.lhs(), res_lhs);
        size_t n = 0;
        for (auto const &[rel, term] : lit.rhs()) {
            ++n;
            match_flags = SimplifyTermFlags::none;
            if (rel == assign || (n < lit.rhs().size() && lit.rhs()[n].first == assign)) {
                match_flags = SimplifyTermFlags::matchable | SimplifyTermFlags::nested_matchable;
            }
            auto [state_term, res_term] = simplify(fixed_flags | match_flags, *ctx_, term);
            succeeded = succeeded && state_term;
            if (!succeeded || state == state_fixed) {
                continue;
            }
            res_rhs.update(Util::transform(res_term, [&rel](auto term) { return Guard{rel, std::move(term)}; }));
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
        auto res_sign = lit.sign() == Sign::twice ? std::make_optional(Sign::none) : std::optional<Sign>();
        return {TruthValue::unknown,
                lit.rewrite(a_sign = res_sign, a_lhs = std::move(res_lhs), a_rhs = std::move(res_rhs))};
    }

    //! Simplify symbolic literals.
    //!
    //! The function ensures the following properties:
    //! (1) the literal is matchable if the corresponding flag has been set,
    //! (2) projection is accepted if the corresponding flag has been set.
    auto operator()(LitSymbolic const &lit, SimplifyLiteralFlags flags) const -> SimplifyResult<Lit> {
        bool head = intersects(flags, SimplifyLiteralFlags::head);
        auto sub_flags = SimplifyTermFlags::none;

        if (intersects(flags, SimplifyLiteralFlags::unfailable)) {
            sub_flags |= SimplifyTermFlags::matchable | SimplifyTermFlags::unfailable;
        } else if (intersects(flags, SimplifyLiteralFlags::matchable) && (!head && lit.sign() == Sign::none)) {
            sub_flags |= SimplifyTermFlags::matchable;
        }
        auto [state, res] = simplify(sub_flags, *ctx_, lit.term());
        if (!state) {
            return {head ? TruthValue::top : TruthValue::bot, make_constant(lit.loc(), head)};
        }
        return {TruthValue::unknown,
                Util::transform(std::move(res), [&](auto term) { return lit.update(a_term = std::move(term)); })};
    }

  private:
    RewriteContext *ctx_; //!< Context used during simplification.
};

class LiteralToTuple {
  public:
    LiteralToTuple() = default;
    LiteralToTuple(LiteralToTuple const &) = delete;
    auto operator=(LiteralToTuple const &) -> LiteralToTuple & = delete;

    auto operator()(Lit const &lit) -> TermArray { return std::visit(*this, lit); }

    auto operator()(LitBool const &lit) -> TermArray {
        ++n_;
        return Util::make_vec<Term>(TermSymbol{lit.loc(), CppClingo::SymbolStore::num_ref(n_)});
    }

    auto operator()(LitComparison const &lit) -> TermArray {
        ++n_;
        auto var_set = select_variables(lit);
        auto var_vec = VariableVec(var_set.begin(), var_set.end());
        std::ranges::sort(var_vec);
        std::vector<Term> res;
        res.reserve(var_vec.size() + 1);
        res.emplace_back(TermSymbol{lit.loc(), CppClingo::SymbolStore::num_ref(n_)});
        for (auto const &var : var_vec) {
            res.emplace_back(TermVariable{lit.loc(), var});
        }
        return res;
    }

    auto operator()(LitSymbolic const &lit) -> TermArray {
        std::vector<Term> res;
        res.reserve(2);
        int i = 0;
        switch (lit.sign()) {
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
        res.emplace_back(TermSymbol{lit.loc(), CppClingo::SymbolStore::num_ref(i)});
        res.emplace_back(lit.term());
        return res;
    }

  private:
    int n_ = 2;
};

//! Simplify a conjunction of literals.
//!
//! In the conjunctive case empty pools evaluate disjunctively, and the result is bot.
//! In the disjunctive case empty pools evaluate conjunctively, and the result is top.
//!
//! @note The automatic extension with the aux elements makes for a somewhat awkward interface.
[[nodiscard]] auto simplify_litvec(RewriteContext &ctx, LitArray const &lits, bool conjunctive = true)
    -> SimplifyResult<std::vector<Lit>> {
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
                res_lits.as_optional() = Util::make_vec<Lit>(std::move(value).value_or(lit));
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
[[nodiscard]] auto simplify_condlit(RewriteContext &ctx, CondLit const &lit, bool conjunctive)
    -> SimplifyResult<CondLit> {
    auto guard = ctx.push();
    auto [state_lit, res_lit] =
        simplify(conjunctive ? SimplifyLiteralFlags::head : SimplifyLiteralFlags::matchable, ctx, lit.lit());
    auto [state_cond, res_cond] = simplify_litvec(ctx, lit.cond());

    auto state_fixed = conjunctive ? TruthValue::top : TruthValue::bot;
    auto state = TruthValue::unknown;

    // elements of *junctions can be removed if their conclusion is neutral
    if (state_lit == state_fixed) {
        // ensure result: "#true/#false:"
        if (!lit.cond().empty()) {
            res_cond = std::vector<Lit>{};
        }
        state = state_fixed;
    }
    // elements of *junctions can be removed if their condition is false
    else if (state_cond == TruthValue::bot) {
        // ensure result: ":#false"
        res_lit = LitBool{lit.loc(), Sign::none, conjunctive};
        state = state_fixed;
    } else if (state_cond == TruthValue::top && state_lit != TruthValue::unknown) {
        state = state_lit;
    }

    return {state, lit.rewrite(a_lit = std::move(res_lit), a_cond = std::move(res_cond))};
}

//! Simplify the left guard of an aggregate.
[[nodiscard]] auto simplify_guard(RewriteContext &ctx, LGuard const &guard, bool matchable)
    -> Util::ResultState<LGuard::value_type> {
    if (guard.has_value()) {
        auto [state, res] =
            simplify(matchable ? SimplifyTermFlags::matchable : SimplifyTermFlags::none, ctx, guard->first);
        return {state, Util::transform(std::move(res), [&guard]<class T>(T &&term) {
                    return LGuard::value_type{std::forward<T>(term), guard->second};
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
        return {state, Util::transform(std::move(res), [&guard]<class T>(T &&term) {
                    return RGuard::value_type{guard->first, std::forward<T>(term)};
                })};
    }
    return {true};
}

//! Simplify a head aggregate element.
[[nodiscard]] auto simplify_element(RewriteContext &ctx, HdLitAggregateElement const &elem)
    -> SimplifyResult<HdLitAggregateElement> {
    auto guard = ctx.push();
    auto [state_tuple, res_tuple] = simplify_termvec(ctx, elem.tuple());
    auto [state_lit, res_lit] = simplify(SimplifyLiteralFlags::none, ctx, elem.lit());
    auto [state_cond, res_cond] = simplify_litvec(ctx, elem.cond());

    if (!state_tuple) {
        state_cond = TruthValue::bot;
    }

    auto state_elem = TruthValue::unknown;
    if (state_lit == TruthValue::top && state_cond == TruthValue::top) {
        auto const &tuple = res_tuple ? *res_tuple : elem.tuple();
        state_elem = all_symbol(tuple) ? TruthValue::top : TruthValue::unknown;
        if (!elem.cond().empty()) {
            res_cond = std::vector<Lit>{};
        }
    }
    if (state_lit == TruthValue::bot || state_cond == TruthValue::bot) {
        state_elem = TruthValue::bot;
        if (!elem.tuple().empty()) {
            res_tuple = TermArray{};
        }
        if (!elem.cond().empty()) {
            res_cond = std::vector<Lit>{};
        }
        if (state_lit != TruthValue::bot) {
            res_lit = LitBool{location(elem.lit()), Sign::none, false};
        }
    }
    auto const *rel_lit = std::get_if<LitComparison>(res_lit ? &*res_lit : &elem.lit());
    if (rel_lit != nullptr) {
        assert(state_cond != TruthValue::bot);
        if (!res_cond.has_value()) {
            res_cond.emplace(elem.cond().begin(), elem.cond().end());
        }
        res_cond->emplace_back(std::move(res_lit).value_or(elem.lit()));
        res_lit = make_constant(location(elem.lit()), true);
    }
    return {state_elem,
            elem.rewrite(a_tuple = std::move(res_tuple), a_lit = std::move(res_lit), a_cond = std::move(res_cond))};
}

//! Simplify a body aggregate element.
[[nodiscard]] auto simplify_element(RewriteContext &ctx, BdLitAggregateElement const &elem)
    -> SimplifyResult<BdLitAggregateElement> {
    auto guard = ctx.push();
    auto [state_tuple, res_tuple] = simplify_termvec(ctx, elem.tuple());
    auto [state_cond, res_cond] = simplify_litvec(ctx, elem.cond());

    if (!state_tuple) {
        state_cond = TruthValue::bot;
    }

    auto state_elem = TruthValue::unknown;
    if (state_cond == TruthValue::top) {
        auto const &tuple = res_tuple ? *res_tuple : elem.tuple();
        state_elem = all_symbol(tuple) ? TruthValue::top : TruthValue::unknown;
        if (!elem.cond().empty()) {
            res_cond = std::vector<Lit>{};
        }
    }
    if (state_cond == TruthValue::bot) {
        state_elem = TruthValue::bot;
        if (!elem.tuple().empty()) {
            res_tuple = TermArray{};
        }
    }
    return {state_elem, elem.rewrite(a_tuple = std::move(res_tuple), a_cond = std::move(res_cond))};
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
auto value(TermArray const &tuple) -> std::optional<Symbol> {
    if (!tuple.empty()) {
        return std::get<TermSymbol>(tuple.front()).value();
    }
    return std::nullopt;
}

//! Get the weight of a tuple as a number (zero if it has none).
auto weight(TermArray const &tuple) -> Number const & {
    if (!tuple.empty()) {
        auto const &sym = std::get<TermSymbol>(tuple.front());
        if (sym.value().type() == SymbolType::number) {
            return sym.value().num();
        }
    }
    static auto zero = Number{0};
    return zero;
}

//! Accumulate the given symbol to res.
//!
//! For count aggregates this should simply be one.
void accumulate(AggregateFunction fun, TermArray const &tuple, std::variant<Number, Symbol> &res) {
    switch (fun) {
        case AggregateFunction::sum: {
            std::get<Number>(res) += weight(tuple);
            break;
        }
        case AggregateFunction::sump: {
            auto const &val = weight(tuple);
            if (val >= 0) {
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
[[nodiscard]] auto check_tuple(AggregateFunction fun, TermArray const &tuple) -> bool {
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
            return sym->value().type() == SymbolType::number && sym->value().num() != 0;
        }
        case AggregateFunction::sump: {
            return sym->value().type() == SymbolType::number && sym->value().num() > 0;
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
template <bool head> using HBAggregate = std::conditional_t<head, HdLitAggregate, BdLitAggregate>;
//! Shortcut for head/body literals.
template <bool head> using HBLiteral = std::conditional_t<head, HdLit, BdLit>;
//! Shortcut for simple head/body literals.
template <bool head> using SimpleHBLiteral = std::conditional_t<head, HdLitSimple, BdLitSimple>;

//! Check if the given relation forms an assignment together with the aggregate.
template <bool head> [[nodiscard]] auto is_assignment(HBAggregate<head> const &lit, Relation rel) -> bool {
    if constexpr (!head) {
        return lit.sign() == Sign::once ? rel == Relation::not_equal : rel == Relation::equal;
    }
    return false;
}

//! Simplify a head or body aggregate.
template <bool head>
[[nodiscard]] auto simplify_aggregate(RewriteContext &ctx, HBAggregate<head> const &lit)
    -> SimplifyResult<HBLiteral<head>> {
    auto const &lit_lhs = lit.lhs();
    auto const &lit_rhs = lit.rhs();
    auto [state_lhs, res_lhs] = simplify_guard(ctx, lit_lhs, lit_lhs && is_assignment<head>(lit, lit_lhs->second));
    auto [state_rhs, res_rhs] = simplify_guard(ctx, lit_rhs, lit_rhs && is_assignment<head>(lit, lit_rhs->first));
    auto res_elems = Util::ResultVec{lit.elems()};
    bool constant = true;
    auto value = neutral_value(lit.fun());
    auto tuples = Util::unordered_set<TermArray>{};
    for (auto const &elem : lit.elems()) {
        auto [state_elem, res_elem] = simplify_element(ctx, elem);
        auto const &tuple = (res_elem ? *res_elem : elem).tuple();
        if (state_elem == TruthValue::bot || !check_tuple(lit.fun(), tuple)) {
            if (state_elem != TruthValue::bot) {
                CLINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, elem.loc())
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
            accumulate(lit.fun(), tuple, value);
        }
        res_elems.update(std::move(res_elem));
    }
    if (!state_lhs) {
        return {head ? TruthValue::top : TruthValue::bot, SimpleHBLiteral<head>{make_constant(location(lit), head)}};
    }
    // Note: value also gives a lower bound for the aggregate, which could be used to detect true/false aggregates
    // (unlikely to be relevant in practice)
    if constexpr (!head) {
        if (!lit_lhs && !lit_rhs) {
            return {TruthValue::top, SimpleHBLiteral<head>{make_constant(lit.loc(), lit.sign() != Sign::once)}};
        }
    }
    if (constant) {
        auto sign = Sign::none;
        if constexpr (head) {
            if (!lit_lhs && !lit_rhs) {
                return {TruthValue::top, SimpleHBLiteral<head>{make_constant(lit.loc(), true)}};
            }
        } else {
            sign = lit.sign();
        }
        auto lhs = Term{TermSymbol{lit.loc(), std::visit(
                                                  [&ctx]<class T>(T value) {
                                                      if constexpr (std::is_same_v<T, Number>) {
                                                          return ctx.store().num_ref(std::move(value));
                                                      }
                                                      if constexpr (std::is_same_v<T, Symbol>) {
                                                          return value;
                                                      }
                                                  },
                                                  std::move(value))}};
        auto guards = std::vector<Guard>{};
        if (lit_lhs) {
            guards.emplace_back(lit_lhs->second, std::move(lhs));
            lhs = Util::transform(std::move(res_lhs), [](auto guard) {
                      return std::move(guard).first;
                  }).value_or(lit_lhs->first);
        }
        if (lit_rhs) {
            guards.emplace_back(lit_rhs->first, Util::transform(std::move(res_rhs), [](auto guard) {
                                                    return std::move(guard).second;
                                                }).value_or(lit_rhs->second));
        }
        auto rel_lit = SimpleHBLiteral<head>{LitComparison{lit.loc(), sign, std::move(lhs), std::move(guards)}};
        auto [state_lit, res_lit] = simplify(ctx, rel_lit);
        return {state_lit, std::move(res_lit).value_or(std::move(rel_lit))};
    }
    return {TruthValue::unknown,
            lit.rewrite(a_lhs = std::move(res_lhs), a_elems = std::move(res_elems), a_rhs = std::move(res_rhs))};
}

//! Simplify a theory atom element.
[[nodiscard]] auto simplify_element(RewriteContext &ctx, TheoryElement const &elem) -> SimplifyResult<TheoryElement> {
    auto guard = ctx.push();
    auto res_tuple = std::optional<TheoryTermArray>{};
    auto [state_cond, res_cond] = simplify_litvec(ctx, elem.cond());

    auto state_elem = TruthValue::unknown;
    if (state_cond == TruthValue::top) {
        state_elem = TruthValue::unknown;
        if (!elem.cond().empty()) {
            res_cond = std::vector<Lit>{};
        }
    }
    if (state_cond == TruthValue::bot) {
        state_elem = TruthValue::bot;
        if (!elem.tuple().empty()) {
            res_tuple = TheoryTermArray{};
        }
    }
    return {state_elem, elem.rewrite(a_tuple = std::move(res_tuple), a_cond = std::move(res_cond))};
}

//! Simplify a theory atom.
template <bool HasSign>
auto simplify_theory_atom(RewriteContext &ctx, TheoryAtom<HasSign> const &lit) -> SimplifyResult<HBLiteral<!HasSign>> {
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
    return {TruthValue::unknown, lit.rewrite(a_name = std::move(res_name), a_elems = std::move(res_elems))};
}

//! Simplify head literals.
class SimplifyHeadLiteral {
  public:
    SimplifyHeadLiteral(RewriteContext &ctx) : ctx_{&ctx} {}

    auto operator()(auto const &lit) const -> SimplifyResult<HdLit> = delete;

    auto operator()(HdLit const &lit) const -> SimplifyResult<HdLit> { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> SimplifyResult<HdLit> {
        auto [state, res] = simplify(SimplifyLiteralFlags::head, *ctx_, lit.lit());
        return {state, Util::transform(std::move(res), [](auto res) { return HdLitSimple{std::move(res)}; })};
    }

    auto operator()(HdLitDisjunction const &lit) const -> SimplifyResult<HdLit> {
        auto state_fixed = TruthValue::top;
        auto state_empty = TruthValue::bot;
        auto state_elems = state_empty;

        auto res_elems = Util::ResultVec{lit.elems()};
        for (auto const &elem : lit.elems()) {
            std::visit(
                [&, this]<class T>(T const &elem) {
                    auto [state, res_elem] = [&, this]() {
                        if constexpr (std::is_same_v<T, CondLit>) {
                            return simplify_condlit(*ctx_, elem, false);
                        } else {
                            return simplify(SimplifyLiteralFlags::head, *ctx_, elem);
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
                        if (lit.elems().size() != 1 || res_elem) {
                            res_elems.as_optional() =
                                Util::make_vec<HdLitDisjunctionElement>(std::move(res_elem).value_or(elem));
                        }
                        state_elems = state_fixed;
                    }
                },
                elem);
        }
        if (state_elems != TruthValue::unknown) {
            return {state_elems, HdLitSimple{make_constant(lit.loc(), state_elems == TruthValue::top)}};
        }
        return {state_elems, Util::transform(std::move(res_elems).as_optional(),
                                             [&lit](auto elems) { return lit.update(a_elems = std::move(elems)); })};
    }
    auto operator()([[maybe_unused]] HdLitSetAggregate const &lit) const -> SimplifyResult<HdLit> {
        throw std::runtime_error("set aggregates must be unpooled before simplifying");
    }

    auto operator()(HdLitAggregate const &lit) const -> SimplifyResult<HdLit> {
        return simplify_aggregate<true>(*ctx_, lit);
    }

    auto operator()(HdLitTheoryAtom const &lit) const -> SimplifyResult<HdLit> {
        return simplify_theory_atom(*ctx_, lit);
    }

  private:
    RewriteContext *ctx_;
};

//! Simplify body literals.
class SimplifyBodyLiteral {
  public:
    SimplifyBodyLiteral(RewriteContext &ctx) : ctx_{&ctx} {}

    auto operator()(auto const &lit) const = delete;

    auto operator()(BdLit const &lit) const -> SimplifyResult<BdLit> { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> SimplifyResult<BdLit> {
        auto [state, res] = simplify(SimplifyLiteralFlags::matchable, *ctx_, lit.lit());
        return {state, Util::transform(std::move(res), [](auto res) { return BdLitSimple{std::move(res)}; })};
    }

    auto operator()(BdLitConjunction const &lit) const -> SimplifyResult<BdLit> {
        return simplify_condlit(*ctx_, lit.lit(), true);
    }

    auto operator()([[maybe_unused]] BdLitSetAggregate const &lit) const -> SimplifyResult<BdLit> {
        throw std::runtime_error("set aggregates must be unpooled before simplifying");
    }

    auto operator()(BdLitAggregate const &lit) const -> SimplifyResult<BdLit> {
        return simplify_aggregate<false>(*ctx_, lit);
    }

    auto operator()(BdLitTheoryAtom const &lit) const -> SimplifyResult<BdLit> {
        return simplify_theory_atom(*ctx_, lit);
    }

  private:
    RewriteContext *ctx_;
};

//! Simplify a vector of body literals.
[[nodiscard]] auto simplify_body(RewriteContext &ctx, BdLitArray const &body) -> SimplifyResult<BdLitArray> {
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
                res_body.as_optional() = Util::make_vec<BdLit>(BdLitSimple{LitBool{location(lit), Sign::none, false}});
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
class SimplifyStatement {
  public:
    SimplifyStatement(RewriteContext &ctx) : ctx_{&ctx} {}

    auto operator()(Stm const &stm) const -> SimplifyResult<Stm> { return std::visit(*this, stm); }

    auto operator()(StmRule const &stm) const -> SimplifyResult<Stm> {
        auto [state_head, res_head] = simplify(*ctx_, stm.head());
        auto [state_body, res_body] = simplify_body(*ctx_, stm.body());
        auto state = TruthValue::unknown;
        if (state_head == TruthValue::top || state_body == TruthValue::bot) {
            if (!stm.body().empty()) {
                res_head = make_constant(location(stm.head()), true);
                res_body = BdLitArray{};
            }
            state = TruthValue::top;
        } else if (state_head == TruthValue::bot && state_body == TruthValue::top) {
            state = TruthValue::bot;
        }
        return {state, stm.rewrite(a_head = std::move(res_head), a_body = std::move(res_body))};
    }

    auto operator()([[maybe_unused]] StmOptimize const &stm) const -> SimplifyResult<Stm> {
        throw std::runtime_error("optimize statements must be unpooled first");
    }

    auto operator()(StmWeakConstraint const &stm) const -> SimplifyResult<Stm> {
        auto const &tuple = stm.tuple();
        auto const &prio = tuple.prio();
        auto [state_weight, res_weight] = simplify(SimplifyTermFlags::none, *ctx_, tuple.weight());
        auto [state_prio, res_prio] = prio ? simplify(SimplifyTermFlags::none, *ctx_, *prio) : SimplifyTermResult{true};
        auto [state_terms, res_terms] = simplify_termvec(*ctx_, tuple.terms());
        auto [state_body, res_body] = simplify_body(*ctx_, stm.body());
        if (!state_weight || state_body == TruthValue::bot || !state_prio || !state_terms) {
            return {TruthValue::top, StmRule{stm.loc(), make_constant(location(stm), true), {}}};
        }
        auto res_tuple = tuple.rewrite(a_weight = std::move(res_weight), a_prio = std::move(res_prio),
                                       a_terms = std::move(res_terms));
        return {TruthValue::unknown, stm.rewrite(a_body = std::move(res_body), a_tuple = std::move(res_tuple))};
    }

    auto operator()(StmShow const &stm) const -> SimplifyResult<Stm> {
        auto [state_term, res_term] = simplify(SimplifyTermFlags::none, *ctx_, stm.term());
        auto [state_body, res_body] = simplify_body(*ctx_, stm.body());
        if (!state_term || state_body == TruthValue::bot) {
            return {TruthValue::top, StmRule{stm.loc(), make_constant(location(stm.term()), true), {}}};
        }
        return {TruthValue::unknown, stm.rewrite(a_term = std::move(res_term), a_body = std::move(res_body))};
    }

    auto operator()(StmProject const &stm) const -> SimplifyResult<Stm> {
        auto [state_atom, res_atom] = simplify(SimplifyTermFlags::matchable, *ctx_, stm.atom());
        auto [state_body, res_body] = simplify_body(*ctx_, stm.body());
        if (!state_atom || state_body == TruthValue::bot) {
            return {TruthValue::top, StmRule{stm.loc(), make_constant(location(stm.atom()), true), {}}};
        }
        return {TruthValue::unknown, stm.rewrite(a_atom = std::move(res_atom), a_body = std::move(res_body))};
    }

    auto operator()(StmExternal const &stm) const -> SimplifyResult<Stm> {
        auto [state_atom, res_atom] = simplify(SimplifyTermFlags::matchable, *ctx_, stm.atom());
        auto const &type = stm.type();
        auto [state_type, res_type] =
            type ? simplify(SimplifyTermFlags::matchable, *ctx_, *type) : SimplifyTermResult{true};
        auto [state_body, res_body] = simplify_body(*ctx_, stm.body());
        if (!state_atom || !state_type || state_body == TruthValue::bot) {
            return {TruthValue::top, StmRule{stm.loc(), make_constant(location(stm.atom()), true), {}}};
        }
        return {TruthValue::unknown,
                stm.rewrite(a_atom = std::move(res_atom), a_body = std::move(res_body), a_type = std::move(res_type))};
    }

    auto operator()(StmEdge const &stm) const -> SimplifyResult<Stm> {
        if (stm.edges().size() != 1) {
            throw std::runtime_error("edge directives must be unpooled before simplifying");
        }
        auto edge = stm.edges().front();
        auto [state_src, res_src] = simplify(SimplifyTermFlags::none, *ctx_, edge.src());
        auto [state_dst, res_dst] = simplify(SimplifyTermFlags::none, *ctx_, edge.dst());
        auto [state_body, res_body] = simplify_body(*ctx_, stm.body());
        if (!state_src || !state_dst || state_body == TruthValue::bot) {
            return {TruthValue::top, StmRule{stm.loc(), make_constant(stm.loc(), true), {}}};
        }
        auto res_edge = edge.rewrite(a_src = std::move(res_src), a_dst = std::move(res_dst));
        auto res_edges =
            Util::transform(std::move(res_edge), [](auto edge) { return Util::make_vec<Edge>(std::move(edge)); });
        return {TruthValue::unknown, stm.rewrite(a_edges = std::move(res_edges), a_body = std::move(res_body))};
    }

    auto operator()(StmHeuristic const &stm) const -> SimplifyResult<Stm> {
        auto const &prio = stm.prio();
        auto [state_atom, res_atom] = simplify(SimplifyTermFlags::matchable, *ctx_, stm.atom());
        auto [state_type, res_type] = simplify(SimplifyTermFlags::none, *ctx_, stm.type());
        auto [state_weight, res_weight] = simplify(SimplifyTermFlags::none, *ctx_, stm.weight());
        auto [state_prio, res_prio] = prio ? simplify(SimplifyTermFlags::none, *ctx_, *prio) : SimplifyTermResult{true};
        auto [state_body, res_body] = simplify_body(*ctx_, stm.body());
        if (!state_atom || !state_type || !state_weight || !state_prio || state_body == TruthValue::bot) {
            return {TruthValue::top, StmRule{stm.loc(), make_constant(stm.loc(), true), {}}};
        }
        return {TruthValue::unknown, stm.rewrite(a_atom = std::move(res_atom), a_body = std::move(res_body),
                                                 a_weight = std::move(res_weight), a_prio = std::move(res_prio),
                                                 a_type = std::move(res_type))};
    }

    template <class T> auto operator()([[maybe_unused]] T const &stm) const -> SimplifyResult<Stm> {
        static_assert(Util::is_among_v<T, StmTheory, StmShowNothing, StmShowSig, StmProjectSig, StmDefined, StmScript,
                                       StmInclude, StmProgram, StmProgram, StmConst, StmParts, StmComment>);
        return {TruthValue::unknown};
    }

  private:
    RewriteContext *ctx_;
};

} // namespace

[[nodiscard]] auto simplify(SimplifyTermFlags flags, RewriteContext &ctx, Term const &term) -> SimplifyTermResult {
    auto make_matchable = [&](auto &&target, bool self = true) -> SimplifyTermResult {
        if (intersects(flags, SimplifyTermFlags::matchable)) {
            if (auto ret = MakeMatchableTerm{ctx}(target, flags); ret.has_value()) {
                return {true, std::move(ret)};
            }
        }
        if (self && target != term) {
            return {true, std::forward<decltype(target)>(target)};
        }
        return {true};
    };
    return std::visit(
        [&]<class T>(T res) -> SimplifyTermResult {
            if constexpr (std::is_same_v<T, TermResultFail>) {
                return {false};
            }
            if constexpr (std::is_same_v<T, TermResultUnchanged>) {
                return make_matchable(term, false);
            }
            if constexpr (std::is_same_v<T, TermResultSymbol>) {
                auto sym = Term{TermSymbol{location(term), res}};
                if (sym != term) {
                    return {true, std::move(sym)};
                }
                return {true};
            }
            if constexpr (std::is_same_v<T, TermResultChanged>) {
                return make_matchable(res.term);
            }
            if constexpr (std::is_same_v<T, TermResultLinear>) {
                return make_matchable(linear_as_term(ctx, std::move(res), false));
            }
        },
        SimplifyTerm{ctx}(term, flags));
}

[[nodiscard]] auto simplify(SimplifyLiteralFlags flags, RewriteContext &ctx, Lit const &lit) -> SimplifyResult<Lit> {
    return SimplifyLiteral{ctx}(lit, flags);
}

[[nodiscard]] auto simplify(RewriteContext &ctx, HdLit const &lit) -> SimplifyResult<HdLit> {
    return SimplifyHeadLiteral{ctx}(lit);
}

[[nodiscard]] auto simplify(RewriteContext &ctx, BdLit const &lit) -> SimplifyResult<BdLit> {
    return SimplifyBodyLiteral{ctx}(lit);
}

[[nodiscard]] auto simplify(RewriteContext &ctx, Stm const &stm) -> SimplifyResult<Stm> {
    auto guard = ctx.push();
    return SimplifyStatement{ctx}(stm);
}

} // namespace CppClingo::Input
