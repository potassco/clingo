#include <util/algorithm.hh>
#include <util/checked_math.hh>

#include <algorithm>
#include <ctime>

#include <input/algo/evaluate.hh>
#include <input/algo/print.hh>
#include <input/algo/simplify.hh>
#include <input/algo/visit_variables.hh>

/*
whole process as in gringo atm
1. apply #const statements (partially done)
2. unpool (done)
3. init theory
4. simplify (done for terms, literals)
  0. evaluate (done)
  1. extract atoms to project (done)
     - add option to forbid completely
     - only check in simplify
  2. dots/script (done)
     - simply remove them all starting from nested contexts
     - they should be ignored in specific settings to make the simplify function idempotent
       this has to be handled by the surrounding literal class
     (done for terms)
  4. terms that can fail (done)
     - needs option to avoid if unnecessary
     - applies to unary, binary, abs, and external in n-ary comparison literals with n > 2
       (probably the only context)
     - 1+a < 5 < 10
     - X < 5 < 10, X=1+a
  5. make matchable (done)
    - can be part of simplify (per option to avoid if unnecessary for example in negated literals)
    - probably best solved using a separate traversal
    - p(X+5,X*X)
      -> p(X+5,Aux), Aux=X*X
    -> p(X+5,@f(g(X*X))),
      -> p(X+5,Aux), Aux=@f(g(X*X)))
      - no traversal into external functions/intervals
6. unpool comparison
   the comparison
     not 1+a < 5 < 10
   is equivalent to
     X=1+a, not X < 5 < 10
   so any term that can fail to evaluate should be stripped during simplification
   this also includes intervals and scripts!
7. rewrite
  1. aggregates
  2. arithmetics
  4. comparisons to intervals
  5. assignment aggregates
*/

namespace Gringo::Input {

namespace {

template <class T> struct SimplifyVec {
    SimplifyVec(std::vector<T> const &in) : in_{in} {}
    void keep() {
        if (out_.has_value()) {
            out_->emplace_back(*cur_);
        }
        ++cur_;
    }
    void remove() {
        if (!out_.has_value()) {
            out_ = Util::copy_n(in_, std::distance(in_.begin(), cur_));
        }
        ++cur_;
    }
    void update(auto &&value) {
        if (!value.has_value()) {
            keep();
        } else {
            if (!out_.has_value()) {
                out_ = Util::copy_n(in_, std::distance(in_.begin(), cur_));
            }
            out_->emplace_back(std::forward<decltype(value)>(value).value());
            ++cur_;
        }
    }
    [[nodiscard]] auto has_value() -> bool { return out_.has_value(); }
    [[nodiscard]] auto opt_value() & -> std::optional<std::vector<T>> & { return out_; }
    [[nodiscard]] auto opt_value() && -> std::optional<std::vector<T>> { return out_; }
    [[nodiscard]] auto value() const & -> std::vector<T> const & { return out_.has_value() ? out_.value() : in_; }
    [[nodiscard]] auto value() && -> std::vector<T> {
        if (out_.has_value()) {
            return std::move(out_).value();
        }
        return in_;
    }
    void extend(AuxTermVec &aux, bool conjunctive = true) {
        if (aux.empty()) {
            return;
        }
        if (!out_.has_value()) {
            out_ = in_;
        }
        for (auto &[lhs, rhs] : aux) {
            auto loc = location(lhs);
            auto rel = conjunctive ? Relation::equal : Relation::inequal;
            auto lit =
                LiteralRelation{loc, Sign::none, std::move(lhs), Util::make_vec<Guard>(Guard{rel, std::move(rhs)})};
            if constexpr (std::is_same_v<T, Literal>) {
                out_->emplace_back(std::move(lit));
            } else {
                out_->emplace_back(SimpleBodyLiteral{std::move(lit)});
            }
        }
    }

  private:
    std::vector<T> const &in_;
    std::optional<std::vector<T>> out_;
    std::vector<T>::const_iterator cur_ = in_.begin();
};

[[nodiscard]] auto make_constant(Location loc, bool truth) -> Literal { return LiteralBoolean{loc, Sign::none, truth}; }

template <class T, class F>
auto transform_res(SimplifyResult<T> const &a, F &&f) -> SimplifyResult<std::invoke_result_t<F, T const &>> {
    if (a.value.has_value()) {
        return {a.state, std::invoke(f, a.value.value())};
    }
    return {a.state, std::nullopt};
}

template <class T, class F>
auto transform_res(SimplifyResult<T> &&a, F &&f) -> SimplifyResult<std::invoke_result_t<F, T &&>> {
    if (a.value.has_value()) {
        return {a.state, std::invoke(f, std::move(a.value).value())};
    }
    return {a.state, std::nullopt};
}

//! Ensure that the term only matches numbers.
[[nodiscard]] auto as_linear_term(SymbolStore &store, Term term) -> Term {
    auto loc = location(term);
    term = TermBinary(loc, TermSymbol{loc, store.num(1)}, BinaryOperator::times, std::move(term));
    return TermBinary(loc, std::move(term), BinaryOperator::plus, TermSymbol{loc, store.num(0)});
}

[[nodiscard]] auto map_term(RewriteContext &ctx, Term term, bool linear = false) -> Term {
    auto loc = location(term);
    ctx.aux().emplace_back(TermVariable{std::move(loc), ctx.gen().new_name()}, std::move(term));
    return linear ? as_linear_term(ctx.store(), ctx.aux().back().first) : ctx.aux().back().first;
}

struct IsNumeric {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(auto const &term) const -> bool = delete;

    auto operator()(TermSymbol const &term) const -> bool { return term.value.type() == SymbolType::number; }

    auto operator()(TermVariable const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermFunction const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermTuple const &term) const -> bool {
        assert(term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front()));
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermAbs const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op == UnaryOperator::invert || std::visit(*this, *term.rhs);
    }

    auto operator()(TermBinary const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }
};

//! Simplify terms.
//!
//! \todo Checking projectable terms could be done right away in the parser.
//! This would make any error reporting at this point unnecessary.
struct SimplifyTerm {
    //! The detected type of a term.
    enum class Type {
        numeric,  //!< Term evaluates to a number.
        symbolic, //!< Term evaluates to a function.
        tuple,    //!< Term evaluates to a tuple.
        any,      //!< Term evaluates to anything.
    };

    //! The evaluation of the term failed.
    struct ResultFail {};
    //! The evaluation resulted in a term of the given type.
    struct ResultChanged {
        Type type;
        Term term;
    };
    //! The evaluation resulted in a linear term.
    struct ResultLinear {
        Term x;
        Number m;
        Number n;
    };
    //! The evaluation resulted in a symbol.
    using ResultSymbol = Symbol;
    //! The evaluation did not change the term.
    using ResultUnchanged = Type;

    //! Variant for the different evaluation results.
    using Result = std::variant<ResultFail, ResultSymbol, ResultUnchanged, ResultChanged, ResultLinear>;

    //! Struct indicating a projected position.
    using Projected = std::monostate;
    //! Result indicating a changed tuple.
    using ResultTupleChanged = std::vector<std::variant<Projected, Symbol, Term>>;
    //! Result indicating an changed tuple.
    struct ResultTupleUnchanged {};
    //! Result indicating a tuples that failed to simplify.
    struct ResultTupleFail {};
    //! Variant for the different tuple evaluation results.
    using ResultTuple = std::variant<ResultTupleFail, ResultTupleUnchanged, ResultTupleChanged>;

    //! Construct an unchanged or changed term result depending on whether the old equals the new term or not.
    [[nodiscard]] static auto check_change(Type type, Term const &old, Term new_) -> Result {
        if (old != new_) {
            return ResultChanged{type, std::move(new_)};
        }
        return ResultUnchanged{type};
    }

    //! Convert a linear result into a term.
    [[nodiscard]] auto linear_as_term(ResultLinear res, bool simplify = true) const -> Term {
        auto mxn = std::move(res.x);
        auto loc = location(mxn);
        if (!simplify || res.m != 1) {
            mxn = TermBinary(loc, TermSymbol{loc, ctx.store().num(std::move(res.m))}, BinaryOperator::times,
                             std::move(mxn));
        }
        if (!simplify || res.n != 0) {
            mxn = TermBinary(loc, std::move(mxn), BinaryOperator::plus,
                             TermSymbol{loc, ctx.store().num(std::move(res.n))});
        }
        return mxn;
    }

    //! Convert a linear result into a term reusing the old term if possible.
    [[nodiscard]] auto linear_as_term(Util::shared_ptr<Term> const &term, ResultLinear res, bool simplify = true) const
        -> Util::shared_ptr<Term> {
        auto ret = linear_as_term(std::move(res), simplify);
        return ret != *term ? Util::construct_shared<Term>(std::move(ret)) : term;
    }

    //! Convert a result into a term.
    //!
    //! If the result does not indicate a change, the given term is used instead.
    [[nodiscard]] auto result_as_term(Util::shared_ptr<Term> const &term, auto &&res, bool simplify = true) const
        -> Util::shared_ptr<Term> {
        GRINGO_MATCH(res, ResultFail) { throw std::logic_error("cannot happen"); }
        GRINGO_MATCH(res, ResultLinear) { return linear_as_term(term, std::move(res), simplify); }
        GRINGO_MATCH(res, ResultUnchanged) { return term; }
        GRINGO_MATCH(res, ResultChanged) { return Util::construct_shared<Term>(std::move(res.term)); }
        GRINGO_MATCH(res, ResultSymbol) {
            Term ret = TermSymbol{location(*term), res};
            return ret != *term ? Util::construct_shared<Term>(std::move(ret)) : term;
        }
    }

    //! Helper to simplify the arguments of the tuple.
    //!
    //! The resulting vector is nullopt if there were no simplifications.
    //! Otherwise, each element is either a monostate in case of projection, a
    //! symbol if it could evaluated right away, or a term in case of some
    //! other simplification.
    auto simplify_tuple(SimplifyFlags flags, auto const &term, TupleVec const &tuple, bool &constant) const
        -> ResultTuple {
        size_t n = 0;

        ResultTuple res_tuple = ResultTupleUnchanged{};

        // helper to initialize the optional result vector
        auto init = [&]() -> ResultTupleChanged & {
            auto *res_changed = std::get_if<ResultTupleChanged>(&res_tuple);
            if (res_changed == nullptr) {
                res_changed = &res_tuple.emplace<ResultTupleChanged>();
                res_changed->reserve(tuple.size());
                for (auto it = tuple.begin(), ie = it + n; it != ie; ++it) {
                    std::visit([res_changed](auto &&res) { res_changed->emplace_back(res); }, *it);
                }
            }
            return *res_changed;
        };

        auto simplify = [&, this](auto &&arg) -> bool {
            // projected argument
            GRINGO_MATCH(arg, Projected) {
                if (!test(flags, SimplifyFlags::projectable)) {
                    GRINGO_REPORT_LOC(ctx.logger(), error, term.loc) << "projection not permitted in this context:\n"
                                                                     << "  " << term << "\n";
                    return false;
                }
                constant = false;
                init().emplace_back();
                return true;
            }
            // term argument
            GRINGO_MATCH(arg, Term) {
                auto simplify = [&](auto &&res) -> bool {
                    // evaluation of argument failed
                    GRINGO_MATCH(res, ResultFail) { return false; }
                    // argument evaluated to symbol
                    GRINGO_MATCH(res, ResultSymbol) {
                        // see note at check_change for function/tuple visitor
                        init().emplace_back(res);
                    }
                    else {
                        constant = false;
                    }
                    GRINGO_MATCH(res, ResultLinear) { init().emplace_back(linear_as_term(std::move(res), false)); }
                    // argument did not change
                    GRINGO_MATCH(res, ResultUnchanged) {
                        if (auto *res_changed = std::get_if<ResultTupleChanged>(&res_tuple); res_changed != nullptr) {
                            res_changed->emplace_back(arg);
                        }
                    }
                    // argument changed
                    GRINGO_MATCH(res, ResultChanged) { init().emplace_back(std::move(res.term)); }
                    return true;
                };
                return std::visit(simplify, operator()(arg, flags));
            }
        };

        // evaluate arguments
        auto res = true;
        for (auto const &arg : tuple) {
            res = std::visit(simplify, arg) && res;
            ++n;
        }
        if (!res) {
            return ResultTupleFail{};
        }
        return res_tuple;
    }

    //! Convert the given simplified arguments to a symbol vector.
    //!
    //! The result vector must only store symbols.
    static auto args_symbol(ResultTupleChanged args_tuple) -> std::vector<Symbol> {
        std::vector<Symbol> args;
        args.reserve(args_tuple.size());
        for (auto const &arg : args_tuple) {
            args.emplace_back(std::get<Symbol>(arg));
        }
        return args;
    }

    //! Convert the given simplified arguments to term tuple.
    static auto args_term(TupleVec const &tuple, ResultTupleChanged args_tuple) -> TupleVec {
        TupleVec args;
        auto it = tuple.begin();
        args.reserve(tuple.size());
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
        return args;
    }

    //! Convert terms of form V and -V where V is a variable to linear terms.
    struct var_to_linear {
        auto operator()(auto &&res) const -> Result {
            GRINGO_MATCH(res, ResultUnchanged) {
                if (std::holds_alternative<TermVariable>(term)) {
                    return ResultLinear{term, Number(1), Number(0)};
                }
                auto const *term_unary = std::get_if<TermUnary>(&term);
                if (term_unary != nullptr && std::holds_alternative<TermVariable>(*term_unary->rhs)) {
                    return ResultLinear{*term_unary->rhs, Number(-1), Number(0)};
                }
            }
            GRINGO_MATCH(res, ResultChanged) {
                if (std::holds_alternative<TermVariable>(res.term)) {
                    return ResultLinear{std::move(res.term), Number(1), Number(0)};
                }
                auto const *term_unary = std::get_if<TermUnary>(&res.term);
                if (term_unary != nullptr && std::holds_alternative<TermVariable>(*term_unary->rhs)) {
                    return ResultLinear{*term_unary->rhs, Number(-1), Number(0)};
                }
            }
            return std::move(res);
        }
        Term const &term;
    };

    //! Simplify the given term.
    auto operator()(Term const &term, SimplifyFlags flags) const -> Result {
        return std::visit(*this, term, std::variant<SimplifyFlags>{flags});
    }

    //! Protect from calling unindented overloads.
    auto operator()(auto const &term, SimplifyFlags flags) const -> Result = delete;

    //! Simplify the given symbolic term.
    auto operator()(TermSymbol const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(flags);
        return term.value;
    }

    //! Simplify the given variable.
    auto operator()(TermVariable const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        // a variable can represent any term
        return Type::any;
    }

    //! Simplify the given function term.
    auto operator()(TermFunction const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1);

        flags &= ~SimplifyFlags::preserve_toplevel_dots;

        if (term.external) {
            flags &= ~SimplifyFlags::projectable;
        }

        bool constant = !term.external;
        auto type = term.external ? Type::any : Type::symbolic;
        auto const &tuple = term.pool.front();

        // simplify arguments
        return std::visit(
            [&, this](auto &&res) -> Result {
                GRINGO_MATCH(res, ResultTupleFail) { return ResultFail{}; }
                GRINGO_MATCH(res, ResultTupleUnchanged) {
                    if (term.external) {
                        return ResultChanged{type, map_term(ctx, term)};
                    }
                    if (!constant) {
                        return type;
                    }
                    return ctx.store().fun(term.name, {}, false);
                }
                GRINGO_MATCH(res, ResultTupleChanged) {
                    if (!constant) {
                        auto fun =
                            TermFunction{term.loc, term.name,
                                         Util::make_vec<TupleVec>(args_term(tuple, std::move(res))), term.external};
                        if (term.external) {
                            return ResultChanged{type, map_term(ctx, std::move(fun))};
                        }
                        // Note: this is somewhat inefficient because the
                        // equality comparision recurses into the structure
                        return check_change(type, term, std::move(fun));
                    }
                    return ctx.store().fun(term.name, args_symbol(std::move(res)), false);
                }
            },
            simplify_tuple(flags, term, tuple, constant));
    }

    //! Simplify the given term tuple.
    auto operator()(TermTuple const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front()));

        flags &= ~SimplifyFlags::preserve_toplevel_dots;

        bool constant = true;
        auto type = Type::tuple;
        auto const &tuple = std::get<TupleVec>(term.pool.front());

        // simplify arguments
        return std::visit(
            [&, this](auto &&res) -> Result {
                GRINGO_MATCH(res, ResultTupleFail) { return ResultFail{}; }
                GRINGO_MATCH(res, ResultTupleUnchanged) {
                    // unchanged term that did not evaluate to a symbol
                    if (!constant) {
                        return type;
                    }
                    return ctx.store().tup(args_symbol({}));
                }
                GRINGO_MATCH(res, ResultTupleChanged) {
                    // changed term that did not evaluate to a symbol
                    if (!constant) {
                        // Note: this is somewhat inefficient because the
                        // equality comparision recurses into the structure
                        return check_change(
                            type, term,
                            TermTuple{term.loc, Util::make_vec<TermTuple::Element>(args_term(tuple, std::move(res)))});
                    }
                    return ctx.store().tup(args_symbol(std::move(res)));
                }
                // the term evaluated to a symbol
            },
            simplify_tuple(flags, term, tuple, constant));
    }

    //! Simplify the given absolute term.
    auto operator()(TermAbs const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1);

        // the term and nested terms are not projectable
        flags &= ~(SimplifyFlags::projectable | SimplifyFlags::preserve_toplevel_dots);

        auto simplify = [&term, this](auto &&res) -> Result {
            // evaluation of argument failed
            GRINGO_MATCH(res, ResultFail) { return {}; }
            // the argument evaluated to a symbol
            GRINGO_MATCH(res, ResultSymbol) {
                if (res.type() != SymbolType::number) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                    return ResultFail{};
                }
                return ctx.store().num(abs(*res.num()));
            }
            GRINGO_MATCH(res, ResultLinear) {
                TermVec pool;
                pool.emplace_back(linear_as_term(std::move(res)));
                return check_change(Type::numeric, term, TermAbs(term.loc, std::move(pool)));
            }
            // the argument did not change
            GRINGO_MATCH(res, ResultUnchanged) {
                if (res == Type::symbolic || res == Type::tuple) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                    return ResultFail{};
                }
                return ResultUnchanged{Type::numeric};
            }
            // the argument changed
            GRINGO_MATCH(res, ResultChanged) {
                // handle invalid terms
                if (res.type == Type::symbolic || res.type == Type::tuple) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                    return ResultFail{};
                }
                // construct a new term
                TermVec pool;
                pool.emplace_back(std::move(res.term));
                return ResultChanged{Type::numeric, TermAbs{term.loc, std::move(pool)}};
            }
        };

        return std::visit(simplify, operator()(term.pool.front(), flags));
    }

    //! Simplify the given unary term.
    auto operator()(TermUnary const &term, SimplifyFlags flags) const -> Result {
        // the term and nested terms are not projectable
        flags &= ~(SimplifyFlags::projectable | SimplifyFlags::preserve_toplevel_dots);

        auto simplify = [&term, this](auto &&res) -> Result {
            // evaluation of argument failed
            GRINGO_MATCH(res, ResultFail) { return ResultFail{}; }
            // the argument evaluated to a symbol
            GRINGO_MATCH(res, ResultSymbol) {
                // we can always evaluate constants
                auto opt_sym = evaluate(ctx.store(), term.op, res);
                if (!opt_sym.has_value()) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                    return ResultFail{};
                }
                return ResultSymbol{opt_sym.value()};
            }
            GRINGO_MATCH(res, ResultLinear) {
                if (term.op == UnaryOperator::negate) {
                    res.m = -std::move(res.m);
                    res.n = -std::move(res.n);
                    return std::move(res);
                }
                return check_change(Type::numeric, term, TermUnary(term.loc, term.op, linear_as_term(std::move(res))));
            }
            // get type of term based on the given type of its argument
            auto check_type = [this, &term](Type type) -> std::optional<Type> {
                if (type == Type::tuple || (term.op == UnaryOperator::invert && type == Type::symbolic)) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                    return std::nullopt;
                }
                // ~term is always numeric
                return term.op == UnaryOperator::invert ? Type::numeric : type;
            };
            // simplify --symbolic to symbolic and ---any to -any
            auto fold = [&term](Type type, Term const &rhs) -> std::optional<ResultChanged> {
                if (term.op != UnaryOperator::negate) {
                    return std::nullopt;
                }
                auto const *rhs_unary = std::get_if<TermUnary>(&rhs);
                if (rhs_unary == nullptr || rhs_unary->op != UnaryOperator::negate) {
                    return std::nullopt;
                }
                // --symbolic
                if (type == Type::symbolic) {
                    return ResultChanged{type, *rhs_unary->rhs};
                }
                auto const *rhs_rhs_unary = std::get_if<TermUnary>(rhs_unary->rhs.get());
                if (rhs_rhs_unary == nullptr || rhs_rhs_unary->op != UnaryOperator::negate) {
                    return std::nullopt;
                }
                // --any
                return ResultChanged{type, *rhs_unary->rhs};
            };
            // the argument did not change
            GRINGO_MATCH(res, Type) {
                auto type = check_type(res);
                if (!type.has_value()) {
                    return ResultFail{};
                }
                // fold if possible
                if (auto opt_res = fold(type.value(), *term.rhs); opt_res.has_value()) {
                    return std::move(opt_res).value();
                }
                return ResultUnchanged{type.value()};
            }
            // the argument changed
            GRINGO_MATCH(res, ResultChanged) {
                auto type = check_type(res.type);
                if (!type.has_value()) {
                    return ResultFail{};
                }
                // fold if possible
                if (auto opt_res = fold(type.value(), res.term); opt_res.has_value()) {
                    return std::move(opt_res).value();
                }
                return ResultChanged{type.value(),
                                     TermUnary{term.loc, term.op, Util::construct_shared<Term>(std::move(res.term))}};
            }
        };
        return std::visit(simplify, operator()(*term.rhs, flags));
    }

    //! Simplify the given binary term.
    auto operator()(TermBinary const &term, SimplifyFlags flags) const -> Result {
        // the term and nested terms are not projectable
        flags &= ~SimplifyFlags::projectable;

        // check if the result can evaluate to a number
        auto is_numeric = [](auto const &res) -> bool {
            GRINGO_MATCH(res, ResultFail) { return false; }
            GRINGO_MATCH(res, ResultLinear) { return true; }
            GRINGO_MATCH(res, ResultUnchanged) { return res == Type::any || res == Type::numeric; }
            GRINGO_MATCH(res, ResultChanged) { return res.type == Type::any || res.type == Type::numeric; }
            GRINGO_MATCH(res, ResultSymbol) { return res.type() == SymbolType::number; }
        };

        if (term.op == BinaryOperator::dots) {
            auto simplify = [&, this](auto &&res_lhs, auto &&res_rhs) -> Result {
                // check arguments
                if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                    return {};
                }
                if (test(flags, SimplifyFlags::preserve_toplevel_dots)) {
                    return check_change(Type::numeric, term,
                                        TermBinary{term.loc, result_as_term(term.lhs, std::move(res_lhs)),
                                                   BinaryOperator::dots, result_as_term(term.rhs, std::move(res_rhs))});
                }
                return ResultLinear{
                    map_term(ctx, TermBinary{term.loc, result_as_term(term.lhs, std::move(res_lhs)),
                                             BinaryOperator::dots, result_as_term(term.rhs, std::move(res_rhs))}),
                    Number{1}, Number{0}};
            };
            return std::visit(simplify, operator()(*term.lhs, flags), operator()(*term.rhs, flags));
        }
        flags &= ~SimplifyFlags::preserve_toplevel_dots;

        auto simplify = [&, this](auto &&res_lhs, auto &&res_rhs) -> Result {
            // check arguments
            if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                    << "  " << term << "\n";
                return {};
            }

            // evaluate to symbol
            GRINGO_MATCH2(res_lhs, Symbol, res_rhs, Symbol) {
                auto res = evaluate(ctx.store(), res_lhs, term.op, res_rhs);
                if (!res.has_value()) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                        << "  " << term << "\n";
                    return ResultFail{};
                }
                return res.value();
            }
            GRINGO_MATCH2(res_lhs, Symbol, res_rhs, ResultLinear) {
                if (term.op == BinaryOperator::plus) {
                    res_rhs.n += res_lhs.num();
                    return std::move(res_rhs);
                }
                if (term.op == BinaryOperator::minus) {
                    res_rhs.m = -std::move(res_rhs.m);
                    res_rhs.n = res_lhs.num() - std::move(res_rhs.n);
                    return std::move(res_rhs);
                }
                if (term.op == BinaryOperator::times && *res_lhs.num() != 0) {
                    res_rhs.m *= res_lhs.num();
                    res_rhs.n *= res_lhs.num();
                    return std::move(res_rhs);
                }
                return check_change(Type::numeric, term,
                                    TermBinary(term.loc, result_as_term(term.lhs, res_lhs), term.op,
                                               linear_as_term(term.rhs, std::move(res_rhs))));
            }
            GRINGO_MATCH2(res_lhs, ResultLinear, res_rhs, Symbol) {
                if (term.op == BinaryOperator::plus) {
                    res_lhs.n += res_rhs.num();
                    return std::move(res_lhs);
                }
                if (term.op == BinaryOperator::minus) {
                    res_lhs.n -= res_rhs.num();
                    return std::move(res_lhs);
                }
                if (term.op == BinaryOperator::times && *res_rhs.num() != 0) {
                    res_lhs.m *= res_rhs.num();
                    res_lhs.n *= res_rhs.num();
                    return std::move(res_lhs);
                }
                return check_change(Type::numeric, term,
                                    TermBinary(term.loc, linear_as_term(term.lhs, std::move(res_lhs)), term.op,
                                               result_as_term(term.rhs, res_rhs)));
            }
            GRINGO_MATCH2(res_lhs, ResultLinear, res_rhs, ResultLinear) {
                if (term.op == BinaryOperator::plus) {
                    if (res_lhs.x == res_rhs.x) {
                        res_lhs.n += res_rhs.n;
                        res_lhs.m += res_rhs.m;
                        return std::move(res_lhs);
                    }
                    res_rhs.n += res_lhs.n;
                    res_lhs.n = Number(0);
                }
                if (term.op == BinaryOperator::minus) {
                    if (res_lhs.x == res_rhs.x) {
                        res_lhs.n -= res_rhs.n;
                        res_lhs.m -= res_rhs.m;
                        return std::move(res_lhs);
                    }
                    res_rhs.n -= res_lhs.n;
                    res_lhs.n = Number(0);
                }
                return check_change(Type::numeric, term,
                                    TermBinary(term.loc, linear_as_term(std::move(res_lhs)), term.op,
                                               linear_as_term(std::move(res_rhs))));
            }

            // none of the arguments changed
            GRINGO_MATCH2(res_lhs, Type, res_rhs, Type) { return Type::numeric; }

            // at least one of the arguments changed
            return check_change(
                Type::numeric, term,
                TermBinary(term.loc, result_as_term(term.lhs, res_lhs), term.op, result_as_term(term.rhs, res_rhs)));
        };

        // construct result
        return std::visit(simplify, std::visit(var_to_linear{*term.lhs}, operator()(*term.lhs, flags)),
                          std::visit(var_to_linear{*term.rhs}, operator()(*term.rhs, flags)));
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
    using ResultTuple = std::optional<TupleVec>;

    //! Make the arguments of the given tuple matchable.
    [[nodiscard]] auto handle_tuple(SimplifyFlags flags, TupleVec const &tuple) const -> ResultTuple {
        size_t n = 0;

        ResultTuple res_tuple;

        // helper to initialize the optional result vector
        auto init = [&]() -> TupleVec & {
            if (!res_tuple.has_value()) {
                res_tuple = Util::copy_n(tuple, n);
            }
            return *res_tuple;
        };

        auto handle_argument = [&, this](auto &&arg) -> void {
            // projected argument
            GRINGO_MATCH(arg, std::monostate) {
                if (res_tuple.has_value()) {
                    init().emplace_back();
                }
            }
            // term argument
            GRINGO_MATCH(arg, Term) {
                if (auto res_arg = operator()(arg, flags & ~SimplifyFlags::nested_matchable);
                    res_arg.has_value() || res_tuple.has_value()) {
                    init().emplace_back(std::move(res_arg).value_or(arg));
                }
            }
        };

        // evaluate arguments
        for (auto const &arg : tuple) {
            std::visit(handle_argument, arg);
            ++n;
        }
        return res_tuple;
    }

    //! Make the given term matchable.
    auto operator()(Term const &term, SimplifyFlags flags) const -> Result {
        return std::visit(*this, term, std::variant<SimplifyFlags>{flags});
    }

    //! Make the given symbolic term matchable.
    auto operator()(TermSymbol const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        return std::nullopt;
    }

    //! Make the given variable term matchable.
    auto operator()(TermVariable const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        return std::nullopt;
    }

    //! Make the given function term matchable.
    auto operator()(TermFunction const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1);
        return handle_tuple(flags, term.pool.front()).transform([&term](auto &&args) {
            return TermFunction{term.loc, term.name, Util::make_vec<TupleVec>(std::move(args)), term.external};
        });
    }

    //! Make the given tuple term matchable.
    auto operator()(TermTuple const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front()));
        return handle_tuple(flags, std::get<TupleVec>(term.pool.front())).transform([&term](auto &&args) {
            return TermTuple{term.loc, Util::make_vec<TermTuple::Element>(std::move(args))};
        });
    }

    //! Make the given absolute term matchable.
    auto operator()(TermAbs const &term, SimplifyFlags flags) const -> Result {
        if (!test(flags, SimplifyFlags::unfailable) && test(flags, SimplifyFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(ctx, term, !test(flags, SimplifyFlags::unfailable));
    }

    //! Make the given unary term matchable.
    auto operator()(TermUnary const &term, SimplifyFlags flags) const -> Result {
        if (!test(flags, SimplifyFlags::unfailable) && term.op == UnaryOperator::negate) {
            return operator()(*term.rhs, flags).transform([&term](auto &&arg) -> Term {
                return TermUnary{term.loc, term.op, Util::construct_shared<Term>(std::forward<decltype(arg)>(arg))};
            });
        }
        if (!test(flags, SimplifyFlags::unfailable) && test(flags, SimplifyFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(ctx, term, !test(flags, SimplifyFlags::unfailable) && is_numeric(term));
    }

    //! Make the given binary term matchable.
    auto operator()(TermBinary const &term, SimplifyFlags flags) const -> Result {
        if (is_linear(term)) {
            // The goal here is to avoid adding additional assignments for auxiliary variables
            // that correspond to variables having a numeric value.
            if (test(flags, SimplifyFlags::unfailable)) {
                auto &n = std::get<TermSymbol>(*term.rhs);
                auto &mx = std::get<TermBinary>(*term.lhs);
                auto &m = std::get<TermSymbol>(*mx.lhs);
                if (*n.value.num() == 0 && *m.value.num() == 1) {
                    for (auto &[lhs, rhs] : ctx.aux()) {
                        if (*mx.rhs == lhs) {
                            if (is_numeric(rhs)) {
                                return *mx.rhs;
                            }
                            break;
                        }
                    }
                }
            } else {
                return std::nullopt;
            }
        }
        if (!test(flags, SimplifyFlags::unfailable) && test(flags, SimplifyFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(ctx, term, !test(flags, SimplifyFlags::unfailable));
    }

    RewriteContext &ctx; //!< Context used during simplification.
};

//! Simplify literals.
//!
//! Does not return a value if the literal did not change.
struct SimplifyLiteral {
    //! Simplify literals dispatching based on type stored in variant.
    auto operator()(Literal const &lit, SimplifyFlags flags) const -> SimplifyResult<Literal> {
        return std::visit(*this, lit, std::variant<SimplifyFlags>{flags});
    }

    //! Simplify Boolean literals.
    //!
    //! Ensures that the literal is either true or false.
    auto operator()(LiteralBoolean const &lit, SimplifyFlags flags) const -> SimplifyResult<Literal> {
        static_cast<void>(flags);
        auto value = (lit.sign != Sign::once) == lit.value;
        auto state = value ? TruthValue::top : TruthValue::bot;

        if (lit.sign != Sign::none) {
            return {state, make_constant(lit.loc, value)};
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
    auto operator()(LiteralRelation const &lit, SimplifyFlags flags) const -> SimplifyResult<Literal> {
        // whether pools are treated disjunctively or conjunctively
        bool head = test(flags, SimplifyFlags::head);
        // whether the elements of the relation are disjunctive or conjunctive
        // (after applying the sign)
        bool disjunctive = head != (lit.sign == Sign::once);

        flags &= ~SimplifyFlags::matchable;
        auto fixed_flags = SimplifyFlags::none;
        if (lit.rhs.size() > 1 && disjunctive) {
            // ensure that unpooling preserves terms that can fail
            fixed_flags = SimplifyFlags::matchable | SimplifyFlags::unfailable;
        }
        // the relation symbol that corresponds to assignment
        // (in the head it is inequality)
        auto assign = disjunctive ? Relation::inequal : Relation::equal;

        auto get_constant = [](Term const &orig, std::optional<Term> const &res) -> std::optional<Symbol> {
            if (res.has_value()) {
                if (auto const *sym = std::get_if<TermSymbol>(&res.value()); sym != nullptr) {
                    return sym->value;
                }
            } else {
                if (auto const *sym = std::get_if<TermSymbol>(&orig); sym != nullptr) {
                    return sym->value;
                }
            }
            return std::nullopt;
        };

        // the truth value of the relation literal if all (signed) comparisions are true
        auto state = lit.sign != Sign::once ? TruthValue::top : TruthValue::bot;
        // the truth value of the literal fixed by one of the  comparisons
        auto state_fixed = lit.sign != Sign::once ? TruthValue::bot : TruthValue::top;
        // the truth value if evaluation of a term fails
        auto state_fail = head ? TruthValue::top : TruthValue::bot;

        // simplify lhs
        auto match_flags = SimplifyFlags::none;
        if (lit.rhs.front().first == assign) {
            match_flags = SimplifyFlags::matchable | SimplifyFlags::nested_matchable;
        }

        // binary assignment
        if (lit.rhs.size() == 1 && lit.rhs.front().first == assign) {
            if (!is_variable(lit.lhs) && is_variable(lit.rhs.front().second)) {
                auto inv = LiteralRelation{lit.loc, lit.sign, lit.rhs.front().second,
                                           Util::make_vec<Guard>(Guard{assign, lit.lhs})};
                auto res = operator()(inv, flags);
                if (!res.value.has_value()) {
                    res.value = std::move(inv);
                }
                return res;
            }

            if (lit.rhs.size() == 1 && lit.rhs.front().first == assign && is_variable(lit.lhs) &&
                is_interval(lit.rhs.front().second)) {
                fixed_flags |= SimplifyFlags::preserve_toplevel_dots;
            }
        }

        auto [succeeded, res_lhs] = simplify(fixed_flags | match_flags, ctx, lit.lhs);

        // simplify rhs
        auto res_rhs = SimplifyVec{lit.rhs};
        auto prev_symbol = get_constant(lit.lhs, res_lhs);
        size_t n = 0;
        for (auto const &[rel, term] : lit.rhs) {
            ++n;
            match_flags = SimplifyFlags::none;
            if (rel == assign || (n < lit.rhs.size() && lit.rhs[n].first == assign)) {
                match_flags = SimplifyFlags::matchable | SimplifyFlags::nested_matchable;
            }
            auto [state_term, res_term] = simplify(fixed_flags | match_flags, ctx, term);
            succeeded = succeeded && state_term;
            if (!succeeded || state == state_fixed) {
                continue;
            }
            res_rhs.update(std::move(res_term).transform([&rel](auto term) { return Guard{rel, std::move(term)}; }));
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
            return {state, make_constant(lit.loc, state == TruthValue::top)};
        }
        if (res_lhs.has_value() || res_rhs.has_value() || lit.sign == Sign::twice) {
            auto sign = lit.sign == Sign::twice ? Sign::none : lit.sign;
            return {TruthValue::unknown,
                    LiteralRelation{lit.loc, sign, std::move(res_lhs).value_or(lit.lhs), std::move(res_rhs).value()}};
        }
        return {TruthValue::unknown};
    }

    //! Simplify symbolic literals.
    //!
    //! The function ensures the following properties:
    //! (1) the literal is matchable if the corresponding flag has been set,
    //! (2) projection is accepted if the corresponding flag has been set.
    auto operator()(LiteralSymbolic const &lit, SimplifyFlags flags) const -> SimplifyResult<Literal> {
        bool head = test(flags, SimplifyFlags::head);
        auto sub_flags = flags & (SimplifyFlags::matchable | SimplifyFlags::projectable | SimplifyFlags::unfailable);
        if (lit.sign != Sign::none && !test(flags, SimplifyFlags::unfailable)) {
            sub_flags &= ~SimplifyFlags::matchable;
        }
        auto [state, res] = simplify(sub_flags, ctx, lit.term);
        if (!state) {
            return {head ? TruthValue::top : TruthValue::bot, make_constant(lit.loc, head)};
        }
        return {TruthValue::unknown, res.transform([&](auto term) {
                    return LiteralSymbolic{lit.loc, lit.sign, std::move(term)};
                })};
    }

    RewriteContext &ctx; //!< Context used during simplification.
};
struct LiteralToTuple {
    auto operator()(Literal const &lit) -> TermVec { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) -> TermVec {
        ++n;
        return Util::make_vec<Term>(TermSymbol{lit.loc, store.num(n)});
    }

    auto operator()(LiteralRelation const &lit) -> TermVec {
        ++n;
        auto var_set = select_variables(lit);
        auto var_vec = std::vector(var_set.begin(), var_set.end());
        std::sort(var_vec.begin(), var_vec.end());
        TermVec res;
        res.reserve(var_vec.size() + 1);
        res.emplace_back(TermSymbol{lit.loc, store.num(n)});
        for (auto const &var : var_vec) {
            res.emplace_back(TermVariable{lit.loc, var});
        }
        return res;
    }

    auto operator()(LiteralSymbolic const &lit) -> TermVec {
        TermVec res;
        res.reserve(2);
        int i = 0;
        switch (lit.sign) {
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
        res.emplace_back(TermSymbol{lit.loc, store.num(i)});
        res.emplace_back(lit.term);
        return res;
    }

    SymbolStore &store;
    int n = 2;
};

//! Simplify head/body literals.
struct SimplifyHBLiteral {
    //! Simplify a conjunction of literals.
    //!
    //! In the conjunctive case empty pools evaluate disjunctively, and the result is bot.
    //! In the disjunctive case empty pools evaluate conjunctively, and the result is top.
    //!
    //! \note The automatic extension with the aux elements makes for a somewhat awkward interface.
    [[nodiscard]] auto simplify_litvec(LiteralVec const &lits, bool conjunctive = true) const
        -> SimplifyResult<LiteralVec> {
        auto state_fixed = conjunctive ? TruthValue::bot : TruthValue::top;
        auto state_empty = conjunctive ? TruthValue::top : TruthValue::bot;
        auto state_lits = state_empty;
        SimplifyVec res_lits{lits};
        for (auto const &lit : lits) {
            auto [state, value] = simplify(conjunctive ? SimplifyFlags::matchable : SimplifyFlags::head, ctx, lit);
            if (state_lits == state_fixed) {
                continue;
            }
            if (state == state_fixed) {
                if (lits.size() != 1 || value.has_value()) {
                    res_lits.opt_value() = Util::make_vec<Literal>(std::move(value).value_or(lit));
                }
                state_lits = state_fixed;
            } else if (state == state_empty) {
                res_lits.remove();
            } else {
                state_lits = TruthValue::unknown;
                res_lits.update(std::move(value));
            }
        }
        if (state_lits == TruthValue::unknown) {
            res_lits.extend(ctx.aux(), conjunctive);
        }
        return {state_lits, std::move(res_lits).opt_value()};
    }

    //! Simplify a term vector.
    [[nodiscard]] auto simplify_termvec(TermVec const &terms) const -> SimplifyResult<TermVec, bool> {
        auto state_terms = true;
        SimplifyVec res_terms{terms};
        for (auto const &term : terms) {
            auto [state_term, res_term] = simplify(SimplifyFlags::none, ctx, term);
            state_terms = state_terms && state_term;
            if (state_terms) {
                res_terms.update(res_term);
            }
        }
        if (!state_terms) {
            return {false};
        }
        return {true, std::move(res_terms).opt_value()};
    }

    [[nodiscard]] static auto all_symbol(TermVec const &terms) -> bool {
        return std::all_of(terms.begin(), terms.end(), is_symbol);
    }

    //! Simplify a conditional literal.
    //!
    //! Example for the head (not conjunctive):
    //! - p((X;A+B),Z): q(Z) :- r.
    //!   - p(X,Z) & (p(Y,Z)|Y!=A+B): q(Z) :- r.
    //!     - p(X,Z): q(Z) :- r.
    //!     - p(Y,Z)|Y!=A+B: q(Z) :- r.
    //!   - case: A+B is undefined
    //!     - #true : q(Z) :- r.
    //!     - this is the same as obtained from p(A+B,Z)
    //!   - case: Y=A+B
    //!     - p(A+B,Z): q(Z) :- r.
    //!     - this corresponds to the original form
    //!   - case: Y!=A+B
    //!     - #true : q(Z) :- r.
    //!     - this is fine because Y is a global variable
    //!     - it gives rise to a weaker rule as compared to the previous one
    //!       (the rule can be ignored)
    //!     - only values of Y equal to A+B are relevant
    //!       (noting  that Y is an auxiliary variable that does not appear anywhere else)
    //!
    //! Example for the body (conjunctive):
    //! - p :- q((X;A+B),Z): r.
    //!   - p :- (q(X,Z)|q(A+B,Z)): r(Z).
    //!     - p :- (q(X,Z)|(q(Y,Z)&Y!=A+B)): r(Z).
    //!       - p :- q(X,Z): r(Z).
    //!       - p :- q(Y,Z) & Y=A+B: r(Z).
    //!   - case: A+B is undefined
    //!     - p :- #false: r(Z).
    //!     - this is the same as obtained from q(A+B,Z)
    //!   - cases Y=A+B and Y!=A+B analogous to head case
    [[nodiscard]] auto simplify_condlit(ConditionalLiteral const &lit, bool conjunctive) const
        -> SimplifyResult<ConditionalLiteral> {
        auto guard = ctx.push();
        auto [state_lits, res_lits] = simplify_litvec(lit.lits, conjunctive);
        guard.reset();
        guard = ctx.push();
        auto [state_cond, res_cond] = simplify_litvec(lit.cond);

        auto state_fixed = conjunctive ? TruthValue::top : TruthValue::bot;
        auto state = TruthValue::unknown;

        // elements of *junctions can be removed if their conclusion is neutral
        if (state_lits == state_fixed) {
            // ensure result: ":"
            if (!lit.cond.empty()) {
                res_cond = LiteralVec{};
            }
            state = state_fixed;
        }
        // elements of *junctions can be removed if their condition is false
        else if (state_cond == TruthValue::bot) {
            // ensure result: ":#false"
            if (!lit.lits.empty()) {
                res_lits = LiteralVec{};
            }
            state = state_fixed;
        } else if (state_cond == TruthValue::top && state_lits != TruthValue::unknown) {
            state = state_lits;
        }

        if (res_lits.has_value() || res_cond.has_value()) {
            return {state, ConditionalLiteral{lit.loc, std::move(res_lits).value_or(lit.lits),
                                              std::move(res_cond).value_or(lit.cond)}};
        }
        return {state};
    }

    //! Simplify a conjunction/disjunction of conditional literals.
    //!
    //!
    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const
        -> SimplifyResult<std::conditional_t<Conjunctive, BodyLiteral, HeadLiteral>> {
        auto state_fixed = Conjunctive ? TruthValue::bot : TruthValue::top;
        auto state_empty = Conjunctive ? TruthValue::top : TruthValue::bot;
        auto state_elems = state_empty;

        auto res_elems = SimplifyVec{lit.elems};
        for (auto const &cond_lit : lit.elems) {
            auto [state, res_elem] = simplify_condlit(cond_lit, Conjunctive);
            if (state_elems == state_fixed) {
                continue;
            }
            if (state == state_empty) {
                res_elems.remove();
            } else if (state == TruthValue::unknown) {
                state_elems = TruthValue::unknown;
                res_elems.update(std::move(res_elem));
            } else if (state == state_fixed) {
                if (lit.elems.size() != 1 || res_elem.has_value()) {
                    res_elems.opt_value() = Util::make_vec<ConditionalLiteral>(std::move(res_elem).value_or(cond_lit));
                }
                state_elems = state_fixed;
            }
        }
        if (state_elems != TruthValue::unknown) {
            using SimpleLiteral = std::conditional_t<Conjunctive, SimpleBodyLiteral, SimpleHeadLiteral>;
            return {state_elems, SimpleLiteral{make_constant(lit.loc, state_elems == TruthValue::top)}};
        }
        return {state_elems, std::move(res_elems).opt_value().transform([&](auto value) {
                    return Junction<Conjunctive>{lit.loc, std::move(value)};
                })};
    }

    //! Simplify the left guard of an aggregate.
    //!
    //! \todo Add option to handle assignments.
    [[nodiscard]] auto simplify_guard(LGuard const &guard) const -> SimplifyResult<LGuard::value_type, bool> {
        if (guard.has_value()) {
            auto [state, res] = simplify(SimplifyFlags::none, ctx, guard->first);
            return {state, std::move(res).transform([&guard](auto &&term) {
                        return LGuard::value_type{std::move(term), guard->second};
                    })};
        }
        return {true};
    }

    //! Simplify the right guard of an aggregate.
    //!
    //! \todo Add option to handle assignments.
    [[nodiscard]] auto simplify_guard(RGuard const &guard) const -> SimplifyResult<RGuard::value_type, bool> {
        if (guard.has_value()) {
            auto [state, res] = simplify(SimplifyFlags::none, ctx, guard->second);
            return {state, std::move(res).transform([&guard](auto &&term) {
                        return RGuard::value_type{guard->first, std::move(term)};
                    })};
        }
        return {true};
    }

    //! Simplify a head aggregate element.
    [[nodiscard]] auto simplify_element(HeadAggregate::Element const &elem) const
        -> SimplifyResult<HeadAggregate::Element> {
        auto guard = ctx.push();
        auto [state_tuple, res_tuple] = simplify_termvec(elem.tuple);
        auto [state_lit, res_lit] = simplify(SimplifyFlags::none, ctx, elem.lit);
        auto [state_cond, res_cond] = simplify_litvec(elem.cond);

        if (!state_tuple) {
            state_cond = TruthValue::bot;
        }

        auto state_elem = TruthValue::unknown;
        if (state_lit == TruthValue::top && state_cond == TruthValue::top) {
            auto const &tuple = res_tuple ? *res_tuple : elem.tuple;
            state_elem = all_symbol(tuple) ? TruthValue::top : TruthValue::unknown;
            if (!elem.cond.empty()) {
                res_cond = LiteralVec{};
            }
        }
        if (state_lit == TruthValue::bot || state_cond == TruthValue::bot) {
            state_elem = TruthValue::bot;
            if (!elem.tuple.empty()) {
                res_tuple = TermVec{};
            }
            if (!elem.cond.empty()) {
                res_cond = LiteralVec{};
            }
            if (state_lit != TruthValue::bot) {
                res_lit = LiteralBoolean{location(elem.lit), Sign::none, false};
            }
        }
        if (res_tuple.has_value() || res_lit.has_value() || res_cond.has_value()) {
            return {state_elem, HeadAggregate::Element{elem.loc, std::move(res_tuple).value_or(elem.tuple),
                                                       std::move(res_lit).value_or(elem.lit),
                                                       std::move(res_cond).value_or(elem.cond)}};
        }
        return {state_elem};
    }

    //! Simplify a body aggregate element.
    [[nodiscard]] auto simplify_element(BodyAggregate::Element const &elem) const
        -> SimplifyResult<BodyAggregate::Element> {
        auto guard = ctx.push();
        auto [state_tuple, res_tuple] = simplify_termvec(elem.tuple);
        auto [state_cond, res_cond] = simplify_litvec(elem.cond);

        if (!state_tuple) {
            state_cond = TruthValue::bot;
        }

        auto state_elem = TruthValue::unknown;
        if (state_cond == TruthValue::top) {
            auto const &tuple = res_tuple ? *res_tuple : elem.tuple;
            state_elem = all_symbol(tuple) ? TruthValue::top : TruthValue::unknown;
            if (!elem.cond.empty()) {
                res_cond = LiteralVec{};
            }
        }
        if (state_cond == TruthValue::bot) {
            state_elem = TruthValue::bot;
            if (!elem.tuple.empty()) {
                res_tuple = TermVec{};
            }
        }
        if (res_tuple.has_value() || res_cond.has_value()) {
            return {state_elem, BodyAggregate::Element{elem.loc, std::move(res_tuple).value_or(elem.tuple),
                                                       std::move(res_cond).value_or(elem.cond)}};
        }
        return {state_elem};
    }

    template <bool head> using HBAggregate = std::conditional_t<head, HeadAggregate, BodyAggregate>;
    template <bool head> using HBLiteral = std::conditional_t<head, HeadLiteral, BodyLiteral>;
    template <bool head> using SimpleHBLiteral = std::conditional_t<head, SimpleHeadLiteral, SimpleBodyLiteral>;

    //! Get the neutral value of the given aggregate.
    //!
    //! This correponds to the aggregate function applied to the empty set.
    static auto neutral_value(AggregateFunction fun) -> std::variant<Number, Symbol> {
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

    static auto value(TermVec const &tuple) -> std::optional<Symbol> {
        if (!tuple.empty()) {
            return std::get<TermSymbol>(tuple.front()).value;
        }
        return std::nullopt;
    }

    static auto weight(TermVec const &tuple) -> NumberRef {
        if (!tuple.empty()) {
            auto const &sym = std::get<TermSymbol>(tuple.front());
            if (sym.value.type() == SymbolType::number) {
                return sym.value.num();
            }
        }
        return NumberRef{Number{0}};
    }

    //! Accumulate the given symbol to res.
    //!
    //! For count aggregates this should simply be one.
    static void accumulate(AggregateFunction fun, TermVec const &tuple, std::variant<Number, Symbol> &res) {
        switch (fun) {
            case AggregateFunction::sum: {
                std::get<Number>(res) += weight(tuple);
            }
            case AggregateFunction::sump: {
                auto val = weight(tuple);
                if (*val >= 0) {
                    std::get<Number>(res) += val;
                }
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

    [[nodiscard]] static auto check_tuple(AggregateFunction fun, TermVec const &tuple) -> bool {
        if (fun == AggregateFunction::count) {
            return true;
        }
        if (tuple.empty()) {
            return false;
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
                return sym->value.type() == SymbolType::number && *sym->value.num() != 0;
            }
            case AggregateFunction::sump: {
                return sym->value.type() == SymbolType::number && *sym->value.num() > 0;
            }
            case AggregateFunction::min: {
                return sym->value.type() != SymbolType::sup;
            }
            case AggregateFunction::max: {
                return sym->value.type() != SymbolType::inf;
            }
        }
        return true;
    }

    //! Simplify a head or body aggregate.
    template <bool head>
    [[nodiscard]] auto simplify_aggregate(HBAggregate<head> const &lit) const -> SimplifyResult<HBLiteral<head>> {
        auto [state_lhs, res_lhs] = simplify_guard(lit.lhs);
        auto [state_rhs, res_rhs] = simplify_guard(lit.rhs);
        AuxTermVec aux;
        auto res_elems = SimplifyVec{lit.elems};
        bool constant = true;
        auto value = neutral_value(lit.fun);
        auto tuples = std::unordered_set<TermVec, Util::value_hasher<TermVec>>{};
        for (auto const &elem : lit.elems) {
            auto [state_elem, res_elem] = simplify_element(elem);
            auto const &tuple = (res_elem ? *res_elem : elem).tuple;
            if (state_elem == TruthValue::bot || !check_tuple(lit.fun, tuple)) {
                if (state_elem != TruthValue::bot) {
                    GRINGO_REPORT_LOC(ctx.logger(), info_operation_undefined, elem.loc)
                        << "aggregate function undefined for tuple:\n"
                        << "  " << elem << "\n";
                }
                res_elems.remove();
                continue;
            }
            if (state_elem == TruthValue::unknown) {
                constant = false;
            } else if (tuples.emplace(tuple).second) {
                accumulate(lit.fun, tuple, value);
            }
            res_elems.update(std::move(res_elem));
        }
        if (!state_lhs) {
            return {head ? TruthValue::top : TruthValue::bot,
                    SimpleHBLiteral<head>{make_constant(location(lit), head)}};
        }
        // Note: value also gives a lower bound for the aggregate, which could be used to detect false aggregates
        // (unlikely to be relevant in practice)
        if constexpr (!head) {
            if (!lit.lhs.has_value() && !lit.rhs.has_value()) {
                return {TruthValue::top, SimpleHBLiteral<head>{make_constant(lit.loc, lit.sign != Sign::once)}};
            }
        }
        if (constant) {
            auto sign = Sign::none;
            if constexpr (head) {
                if (!lit.lhs.has_value() && !lit.rhs.has_value()) {
                    return {TruthValue::top, SimpleHBLiteral<head>{make_constant(lit.loc, true)}};
                }
            } else {
                sign = lit.sign;
            }
            auto lhs = Term{TermSymbol{lit.loc, std::visit(
                                                    [this](auto &&value) {
                                                        GRINGO_MATCH(value, Number) {
                                                            return ctx.store().num(GRINGO_FWD(value));
                                                        }
                                                        GRINGO_MATCH(value, Symbol) { return value; }
                                                    },
                                                    value)}};
            auto guards = GuardVec{};
            if (lit.lhs.has_value()) {
                guards.emplace_back(lit.lhs->second, std::move(lhs));
                lhs = std::move(res_lhs.transform([](auto guard) {
                          return std::move(guard).first;
                      })).value_or(lit.lhs->first);
            }
            if (lit.rhs.has_value()) {
                guards.emplace_back(lit.rhs->first, std::move(res_rhs.transform([](auto guard) {
                                                        return std::move(guard).second;
                                                    })).value_or(lit.rhs->second));
            }
            auto rel_lit = SimpleHBLiteral<head>{LiteralRelation{lit.loc, sign, std::move(lhs), std::move(guards)}};
            auto [state_lit, res_lit] = simplify(ctx, rel_lit);
            return {state_lit, std::move(res_lit).value_or(std::move(rel_lit))};
        }
        if (res_lhs.has_value() || res_rhs.has_value() || res_elems.has_value()) {
            auto lhs = lit.lhs.transform([&res_lhs](auto const &orig) { return std::move(res_lhs).value_or(orig); });
            auto rhs = lit.rhs.transform([&res_rhs](auto const &orig) { return std::move(res_rhs).value_or(orig); });
            if constexpr (head) {
                return {TruthValue::unknown,
                        HeadAggregate{lit.loc, std::move(lhs), lit.fun, std::move(res_elems).value(), std::move(rhs)}};
            } else {
                return {TruthValue::unknown, BodyAggregate{lit.loc, lit.sign, std::move(lhs), lit.fun,
                                                           std::move(res_elems).value(), std::move(rhs)}};
            }
        }
        return {TruthValue::unknown};
    }
    RewriteContext &ctx; //!< Context used during simplification.
};

struct SimplifyHeadLiteral : SimplifyHBLiteral {
    auto operator()(auto const &lit) const -> SimplifyResult<HeadLiteral> = delete;

    using SimplifyHBLiteral::operator();

    auto operator()(HeadLiteral const &lit) const -> SimplifyResult<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> SimplifyResult<HeadLiteral> {
        auto [state, res] = simplify(SimplifyFlags::head, ctx, lit.lit);
        return {state, res.transform([](auto &&res) { return SimpleHeadLiteral{GRINGO_FWD(res)}; })};
    }

    auto operator()(HeadSetAggregate const &lit) const -> SimplifyResult<HeadLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("set aggregates must be unpooled before simplifying");
    }

    auto operator()(HeadAggregate const &lit) const -> SimplifyResult<HeadLiteral> {
        return simplify_aggregate<true>(lit);
    }

    auto operator()(HeadTheoryAtom const &lit) const -> SimplifyResult<HeadLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me");
    }
};

struct SimplifyBodyLiteral : SimplifyHBLiteral {
    auto operator()(auto const &lit) const -> SimplifyResult<BodyLiteral> = delete;

    using SimplifyHBLiteral::operator();

    auto operator()(BodyLiteral const &lit) const -> SimplifyResult<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> SimplifyResult<BodyLiteral> {
        auto [state, res] = simplify(SimplifyFlags::matchable, ctx, lit.lit);
        return {state, res.transform([](auto &&res) { return SimpleBodyLiteral{GRINGO_FWD(res)}; })};
    }

    auto operator()(BodySetAggregate const &lit) const -> SimplifyResult<BodyLiteral> {
        static_cast<void>(lit);
        throw std::runtime_error("set aggregates must be unpooled before simplifying");
    }

    auto operator()(BodyAggregate const &lit) const -> SimplifyResult<BodyLiteral> {
        return simplify_aggregate<false>(lit);
    }

    auto operator()(BodyTheoryAtom const &lit) const -> SimplifyResult<BodyLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me");
    }
};

} // namespace

struct SimplifyStatement {
    [[nodiscard]] static auto simplify_tuple(StatementOptimize::Tuple const &elem)
        -> SimplifyResult<StatementOptimize::Tuple> {
        static_cast<void>(elem);
        throw std::logic_error("implement me!!!");
    }

    [[nodiscard]] static auto simplify_element(StatementOptimize::Element const &elem)
        -> SimplifyResult<StatementOptimize::Element> {
        static_cast<void>(elem);
        throw std::logic_error("implement me!!!");
    }

    [[nodiscard]] static auto simplify_edge(StatementEdge::Edge const &edge) -> SimplifyResult<StatementEdge::Edge> {
        static_cast<void>(edge);
        throw std::logic_error("implement me!!!");
    }

    [[nodiscard]] auto simplify_body(BodyLiteralVec const &body) const -> SimplifyResult<BodyLiteralVec> {
        auto res_body = SimplifyVec{body};
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
                    res_body.opt_value() = Util::make_vec<BodyLiteral>(
                        SimpleBodyLiteral{LiteralBoolean{location(lit), Sign::none, false}});
                }
                state_body = TruthValue::bot;
            } else if (state_lit == TruthValue::unknown) {
                state_body = TruthValue::unknown;
            }
        }
        if (state_body != TruthValue::bot) {
            res_body.extend(ctx.aux());
        }
        return {state_body, std::move(res_body).opt_value()};
    }

    auto operator()(auto const &lit) const -> SimplifyResult<Statement> = delete;

    auto operator()(Statement const &stm) const -> SimplifyResult<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> SimplifyResult<Statement> {
        auto [state_head, res_head] = simplify(ctx, stm.head);
        auto [state_body, res_body] = simplify_body(stm.body);
        auto state = TruthValue::unknown;
        if (state_head == TruthValue::top || state_body == TruthValue::bot) {
            if (!stm.body.empty()) {
                res_head = SimpleHeadLiteral{LiteralBoolean{location(stm.head), Sign::none, true}};
                res_body = BodyLiteralVec{};
            }
            state = TruthValue::top;
        }
        if (state_head == TruthValue::bot && state_body == TruthValue::top) {
            state = TruthValue::bot;
        }
        if (res_head.has_value() || res_body.has_value()) {
            return {state,
                    Rule{stm.loc, std::move(res_head).value_or(stm.head), std::move(res_body).value_or(stm.body)}};
        }
        return {state};
    }

    auto operator()(TheoryDefinition const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementOptimize const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementWeakConstraint const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementShow const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementShowSig const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    auto operator()(StatementProject const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
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
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementEdge const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementHeuristic const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
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
        throw std::logic_error("implement me!!!");
    }

    auto operator()(Comment const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {TruthValue::unknown};
    }

    RewriteContext &ctx; //!< Context used during simplification.
};

[[nodiscard]] auto is_linear(TermBinary const &term) -> bool {
    if (term.op != BinaryOperator::plus) {
        return false;
    }
    auto const *mul = std::get_if<TermBinary>(term.lhs.get());
    if (mul == nullptr || mul->op != BinaryOperator::times) {
        return false;
    }
    auto const *n = std::get_if<TermSymbol>(term.rhs.get());
    if (n == nullptr || n->value.type() != SymbolType::number) {
        return false;
    }
    auto const *m = std::get_if<TermSymbol>(mul->lhs.get());
    if (m == nullptr || m->value.type() != SymbolType::number || *m->value.num() == 0) {
        return false;
    }
    return std::holds_alternative<TermVariable>(*mul->rhs);
}

[[nodiscard]] auto is_linear(Term const &term) -> bool {
    auto const *plus = std::get_if<TermBinary>(&term);
    return plus != nullptr && is_linear(*plus);
}

[[nodiscard]] auto is_interval(Term const &term) -> bool {
    auto const *bin = std::get_if<TermBinary>(&term);
    return bin != nullptr && is_interval(*bin);
}

[[nodiscard]] auto is_interval(TermBinary const &term) -> bool { return term.op == BinaryOperator::dots; }

[[nodiscard]] auto is_numeric(Term const &term) -> bool { return IsNumeric{}(term); }

[[nodiscard]] auto simplify(SimplifyFlags flags, RewriteContext &ctx, Term const &term) -> SimplifyResult<Term, bool> {
    auto make_matchable = [&](auto &&target, bool self = true) -> SimplifyResult<Term, bool> {
        if (test(flags, SimplifyFlags::matchable)) {
            if (auto ret = MakeMatchableTerm{ctx}(target, flags); ret.has_value()) {
                return {true, std::move(ret).value()};
            }
        }
        if (self && target != term) {
            return {true, std::forward<decltype(target)>(target)};
        }
        return {true};
    };
    auto simp = SimplifyTerm{ctx};
    return std::visit(
        [&](auto &&res) -> SimplifyResult<Term, bool> {
            GRINGO_MATCH(res, SimplifyTerm::ResultFail) { return {false}; }
            GRINGO_MATCH(res, SimplifyTerm::ResultUnchanged) { return make_matchable(term, false); }
            GRINGO_MATCH(res, SimplifyTerm::ResultSymbol) {
                auto sym = Term{TermSymbol{location(term), res}};
                if (sym != term) {
                    return {true, std::move(sym)};
                }
                return {true};
            }
            GRINGO_MATCH(res, SimplifyTerm::ResultChanged) { return make_matchable(res.term); }
            GRINGO_MATCH(res, SimplifyTerm::ResultLinear) {
                return make_matchable(simp.linear_as_term(std::move(res), false));
            }
        },
        simp(term, flags));
}

[[nodiscard]] auto simplify(SimplifyFlags flags, RewriteContext &ctx, Literal const &lit) -> SimplifyResult<Literal> {
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
