#include <util/algorithm.hh>
#include <util/checked_math.hh>

#include <ctime>

#include <input/algo/evaluate.hh>
#include <input/algo/print.hh>
#include <input/algo/simplify.hh>

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

[[nodiscard]] auto map_term(SimplifyContext const &ctx, Term term, bool linear = false) -> Term {
    auto loc = location(term);
    ctx.aux.emplace_back(TermVariable{std::move(loc), ctx.gen.new_name()}, std::move(term));
    return linear ? as_linear_term(ctx.store, ctx.aux.back().first) : ctx.aux.back().first;
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
            mxn = TermBinary(loc, TermSymbol{loc, ctx.store.num(std::move(res.m))}, BinaryOperator::times,
                             std::move(mxn));
        }
        if (!simplify || res.n != 0) {
            mxn =
                TermBinary(loc, std::move(mxn), BinaryOperator::plus, TermSymbol{loc, ctx.store.num(std::move(res.n))});
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
                    GRINGO_REPORT_LOC(ctx.log, error, term.loc) << "projection not permitted in this context:\n"
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
                    return ctx.store.fun(term.name, {}, false);
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
                    return ctx.store.fun(term.name, args_symbol(std::move(res)), false);
                }
            },
            simplify_tuple(flags, term, tuple, constant));
    }

    //! Simplify the given term tuple.
    auto operator()(TermTuple const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front()));

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
                    return ctx.store.tup(args_symbol({}));
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
                    return ctx.store.tup(args_symbol(std::move(res)));
                }
                // the term evaluated to a symbol
            },
            simplify_tuple(flags, term, tuple, constant));
    }

    //! Simplify the given absolute term.
    auto operator()(TermAbs const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1);

        // the term and nested terms are not projectable
        flags &= ~SimplifyFlags::projectable;

        auto simplify = [&term, this](auto &&res) -> Result {
            // evaluation of argument failed
            GRINGO_MATCH(res, ResultFail) { return {}; }
            // the argument evaluated to a symbol
            GRINGO_MATCH(res, ResultSymbol) {
                if (res.type() != SymbolType::number) {
                    GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                   << "  " << term << "\n";
                    return ResultFail{};
                }
                return ctx.store.num(abs(*res.num()));
            }
            GRINGO_MATCH(res, ResultLinear) {
                TermVec pool;
                pool.emplace_back(linear_as_term(std::move(res)));
                return check_change(Type::numeric, term, TermAbs(term.loc, std::move(pool)));
            }
            // the argument did not change
            GRINGO_MATCH(res, ResultUnchanged) {
                if (res == Type::symbolic || res == Type::tuple) {
                    GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                   << "  " << term << "\n";
                    return ResultFail{};
                }
                return ResultUnchanged{Type::numeric};
            }
            // the argument changed
            GRINGO_MATCH(res, ResultChanged) {
                // handle invalid terms
                if (res.type == Type::symbolic || res.type == Type::tuple) {
                    GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
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
        flags &= ~SimplifyFlags::projectable;

        auto simplify = [&term, this](auto &&res) -> Result {
            // evaluation of argument failed
            GRINGO_MATCH(res, ResultFail) { return ResultFail{}; }
            // the argument evaluated to a symbol
            GRINGO_MATCH(res, ResultSymbol) {
                // we can always evaluate constants
                auto opt_sym = evaluate(ctx.store, term.op, res);
                if (!opt_sym.has_value()) {
                    GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
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
                    GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
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
                    GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                                   << "  " << term << "\n";
                    return {};
                }
                return ResultLinear{
                    map_term(ctx, TermBinary{term.loc, result_as_term(term.lhs, std::move(res_lhs)),
                                             BinaryOperator::dots, result_as_term(term.rhs, std::move(res_rhs))}),
                    Number{1}, Number{0}};
            };
            return std::visit(simplify, operator()(*term.lhs, flags), operator()(*term.rhs, flags));
        }
        auto simplify = [&, this](auto &&res_lhs, auto &&res_rhs) -> Result {
            // check arguments
            if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
                                                                               << "  " << term << "\n";
                return {};
            }

            // evaluate to symbol
            GRINGO_MATCH2(res_lhs, Symbol, res_rhs, Symbol) {
                auto res = evaluate(ctx.store, res_lhs, term.op, res_rhs);
                if (!res.has_value()) {
                    GRINGO_REPORT_LOC(ctx.log, info_operation_undefined, term.loc) << "operation undefined:\n"
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

    SimplifyContext ctx; //!< Context used during simplification.
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
        if (!test(flags, SimplifyFlags::unfailable) && is_linear(term)) {
            return std::nullopt;
        }
        if (!test(flags, SimplifyFlags::unfailable) && test(flags, SimplifyFlags::nested_matchable)) {
            return std::nullopt;
        }
        return map_term(ctx, term, !test(flags, SimplifyFlags::unfailable));
    }

    SimplifyContext ctx; //!< Context used during simplification.
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
        auto state = value ? SimplifyState::top : SimplifyState::bot;

        if (lit.sign != Sign::none) {
            return {state, LiteralBoolean{lit.loc, Sign::none, value}};
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
    auto operator()(LiteralRelation const &lit, SimplifyFlags flags) const -> SimplifyResult<Literal> {
        flags &= ~SimplifyFlags::matchable;
        if (lit.sign == Sign::once) {
            flags ^= SimplifyFlags::disjunctive;
        }
        auto fixed_flags = SimplifyFlags::none;
        if (lit.rhs.size() > 1 && test(flags, SimplifyFlags::disjunctive)) {
            fixed_flags = SimplifyFlags::matchable | SimplifyFlags::unfailable;
        }
        auto assign = test(flags, SimplifyFlags::disjunctive) ? Relation::inequal : Relation::equal;

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

        // simplify lhs
        auto match_flags = SimplifyFlags::none;
        if (lit.rhs.front().first == assign) {
            match_flags = SimplifyFlags::matchable | SimplifyFlags::nested_matchable;
        }
        auto [state, res_lhs] = simplify(fixed_flags | match_flags, ctx, lit.lhs);
        auto prev_symbol = get_constant(lit.lhs, res_lhs);

        // simplify rhs
        auto res_rhs = SimplifyVec{lit.rhs};
        size_t n = 0;
        if (state != SimplifyState::fail) {
            // the truth value of the relation literal if all comparisions are true
            state = lit.sign != Sign::once ? SimplifyState::top : SimplifyState::bot;
        }
        auto fixed_state = lit.sign != Sign::once ? SimplifyState::bot : SimplifyState::top;
        for (auto const &[rel, term] : lit.rhs) {
            ++n;
            match_flags = SimplifyFlags::none;
            if (rel == assign || (n < lit.rhs.size() && lit.rhs[n].first == assign)) {
                match_flags = SimplifyFlags::matchable | SimplifyFlags::nested_matchable;
            }
            auto [state_term, res_term] = simplify(fixed_flags | match_flags, ctx, term);
            if (state == SimplifyState::fail || state_term == SimplifyState::fail) {
                state = SimplifyState::fail;
                continue;
            }
            if (state == fixed_state) {
                continue;
            }
            res_rhs.update(std::move(res_term).transform([&rel](auto term) { return Guard{rel, std::move(term)}; }));
            auto cur_symbol = get_constant(term, res_term);
            if (prev_symbol.has_value() && cur_symbol.has_value()) {
                // the truth value of the relation literal is fixed if the comparison is false
                if (!evaluate(prev_symbol.value(), rel, cur_symbol.value())) {
                    state = fixed_state;
                }
            } else {
                state = SimplifyState::unknown;
            }

            prev_symbol = cur_symbol;
        }

        // construct result
        if (state == SimplifyState::fail) {
            return {SimplifyState::fail};
        }
        if (state != SimplifyState::unknown) {
            return {state, LiteralBoolean{lit.loc, Sign::none, state == SimplifyState::top}};
        }
        if (res_lhs.has_value() || res_rhs.has_value() || lit.sign == Sign::twice) {
            auto sign = lit.sign == Sign::twice ? Sign::none : lit.sign;
            return {SimplifyState::unknown,
                    LiteralRelation{lit.loc, sign, std::move(res_lhs).value_or(lit.lhs), std::move(res_rhs).value()}};
        }
        return {SimplifyState::unknown};
    }

    //! Simplify symbolic literals.
    //!
    //! The function ensures the following properties:
    //! (1) the literal is matchable if the corresponding flag has been set,
    //! (2) projection is accepted if the corresponding flag has been set.
    auto operator()(LiteralSymbolic const &lit, SimplifyFlags flags) const -> SimplifyResult<Literal> {
        auto sub_flags = flags & (SimplifyFlags::matchable | SimplifyFlags::projectable);
        if (lit.sign != Sign::none) {
            sub_flags &= ~SimplifyFlags::matchable;
        }
        auto [state, res] = simplify(sub_flags, ctx, lit.term);
        if (state == SimplifyState::fail) {
            return {SimplifyState::fail};
        }
        return {SimplifyState::unknown, res.transform([&](auto term) {
                    return LiteralSymbolic{lit.loc, lit.sign, std::move(term)};
                })};
    }

    SimplifyContext ctx; //!< Context used during simplification.
};

//! Simplify head/body literals.
struct SimplifyHBLiteral {
    //! Simplify a conjunction of literals.
    //!
    //! \note The automatic extension with the aux elements makes for a somewhat awkward interface.
    [[nodiscard]] auto simplify_litvec(LiteralVec const &lits, bool conjunctive = true) const
        -> SimplifyResult<LiteralVec> {
        auto state_fixed = conjunctive ? SimplifyState::bot : SimplifyState::top;
        auto state_empty = conjunctive ? SimplifyState::top : SimplifyState::bot;
        auto state_lits = state_empty;
        SimplifyVec res_lits{lits};
        for (auto const &lit : lits) {
            auto [state, value] =
                simplify(conjunctive ? SimplifyFlags::matchable : SimplifyFlags::disjunctive, ctx, lit);
            if (state_lits == SimplifyState::fail || state == SimplifyState::fail) {
                state_lits = SimplifyState::fail;
                continue;
            }
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
                state_lits = SimplifyState::unknown;
                res_lits.update(std::move(value));
            }
        }
        if (state_lits == SimplifyState::fail) {
            res_lits.opt_value() = std::nullopt;
        }
        if (state_lits == SimplifyState::unknown) {
            res_lits.extend(ctx.aux, conjunctive);
        }
        ctx.aux.clear();
        return {state_lits, std::move(res_lits).opt_value()};
    }

    //! Simplify a conditional literal.
    [[nodiscard]] auto simplify_condlit(ConditionalLiteral const &lit, bool conjunctive) const
        -> SimplifyResult<ConditionalLiteral> {
        auto [state_lits, res_lits] = simplify_litvec(lit.lits, conjunctive);
        auto [state_cond, res_cond] = simplify_litvec(lit.cond);

        if (state_lits == SimplifyState::fail || state_cond == SimplifyState::fail) {
            return {SimplifyState::fail};
        }

        auto state_fixed = conjunctive ? SimplifyState::top : SimplifyState::bot;
        auto state = SimplifyState::unknown;

        // elements of *junctions can be removed if their conclusion is neutral
        if (state_lits == state_fixed) {
            // ensure result: ":"
            if (!lit.cond.empty()) {
                res_cond = LiteralVec{};
            }
            state = state_fixed;
        }
        // elements of *junctions can be removed if their condition is false
        else if (state_cond == SimplifyState::bot) {
            // ensure result: ":#false"
            if (!lit.lits.empty()) {
                res_lits = LiteralVec{};
            }
            state = state_fixed;
        } else if (state_cond == SimplifyState::top && state_lits != SimplifyState::unknown) {
            state = state_lits;
        }

        if (res_lits.has_value() || res_cond.has_value()) {
            return {state, ConditionalLiteral{lit.loc, std::move(res_lits).value_or(lit.lits),
                                              std::move(res_cond).value_or(lit.cond)}};
        }
        return {state};
    }

    //! Simplify a conjunction/disjunction of conditional literals.
    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const
        -> SimplifyResult<std::conditional_t<Conjunctive, BodyLiteral, HeadLiteral>> {
        auto state_fixed = Conjunctive ? SimplifyState::bot : SimplifyState::top;
        auto state_empty = Conjunctive ? SimplifyState::top : SimplifyState::bot;
        auto state_elems = state_empty;

        auto res_elems = SimplifyVec{lit.elems};
        for (auto const &cond_lit : lit.elems) {
            auto [state, res_elem] = simplify_condlit(cond_lit, Conjunctive);
            if (state_elems == state_fixed) {
                continue;
            }
            if (state == SimplifyState::fail || state == state_empty) {
                res_elems.remove();
            } else if (state == SimplifyState::unknown) {
                state_elems = SimplifyState::unknown;
                res_elems.update(std::move(res_elem));
            } else if (state == state_fixed) {
                if (lit.elems.size() != 1 || res_elem.has_value()) {
                    res_elems.opt_value() = Util::make_vec<ConditionalLiteral>(std::move(res_elem).value_or(cond_lit));
                }
                state_elems = state_fixed;
            }
        }
        if (state_elems == SimplifyState::top || state_elems == SimplifyState::bot) {
            using SimpleLiteral = std::conditional_t<Conjunctive, SimpleBodyLiteral, SimpleHeadLiteral>;
            return {state_elems, SimpleLiteral{LiteralBoolean{lit.loc, Sign::none, state_elems == SimplifyState::top}}};
        }
        return {state_elems, std::move(res_elems).opt_value().transform([&](auto value) {
                    return Junction<Conjunctive>{lit.loc, std::move(value)};
                })};
    }

    //! Simplify a set aggregate element.
    //!
    //! Status top and fail will be used if the element is statically true/false.
    //! In case the element is statically false, no updated aggregate element is provided.
    [[nodiscard]] auto simplify_element(SetAggregateElement const &elem) const -> SimplifyResult<SetAggregateElement> {
        auto [state_lit, res_lit] = simplify(SimplifyFlags::none, ctx, elem.lit);
        auto [state_cond, res_cond] = simplify_litvec(elem.cond);
        auto make_elem = [&]() -> std::optional<SetAggregateElement> {
            if (res_cond.has_value() || res_lit.has_value()) {
                return SetAggregateElement{std::move(res_lit).value_or(elem.lit),
                                           std::move(res_cond).value_or(elem.cond)};
            }
            return std::nullopt;
        };
        // the literal or condition is false
        if (state_lit == SimplifyState::fail || state_cond == SimplifyState::fail || state_lit == SimplifyState::bot ||
            state_cond == SimplifyState::bot) {
            return {SimplifyState::fail};
        }
        auto state = SimplifyState::unknown;
        // each true literal can be subtracted from the bounds of the aggregate
        if (state_lit == SimplifyState::top && state_cond == SimplifyState::top) {
            // result: "#true:"
            if (!elem.cond.empty()) {
                res_cond = LiteralVec{};
            }
            state = SimplifyState::top;
        }
        return {state, make_elem()};
    }

    //! Simplify the left guard of an aggregate.
    //!
    //! \todo Add option to handle assignments.
    [[nodiscard]] auto simplify_guard(LGuard const &guard) const -> std::optional<SimplifyResult<LGuard>> {
        return guard.transform([this](auto const &guard) {
            return transform_res(simplify(SimplifyFlags::none, ctx, guard.first), [&guard](auto term) -> LGuard {
                return LGuard::value_type{std::move(term), guard.second};
            });
        });
    }

    //! Simplify the right guard of an aggregate.
    //!
    //! \todo Add option to handle assignments.
    [[nodiscard]] auto simplify_guard(RGuard const &guard) const -> std::optional<SimplifyResult<RGuard>> {
        return guard.transform([this](auto const &guard) {
            return transform_res(simplify(SimplifyFlags::none, ctx, guard.second), [&guard](auto term) -> RGuard {
                return RGuard::value_type{guard.first, std::move(term)};
            });
        });
    }

    SimplifyContext ctx; //!< Context used during simplification.
};

struct SimplifyHeadLiteral : SimplifyHBLiteral {
    using SimplifyHBLiteral::simplify_element;

    [[nodiscard]] static auto simplify_element(HeadAggregate::Element const &elem)
        -> SimplifyResult<HeadAggregate::Element> {
        static_cast<void>(elem);
        throw std::logic_error("implement me");
    }

    auto operator()(auto const &lit) const -> SimplifyResult<HeadLiteral> = delete;

    using SimplifyHBLiteral::operator();

    auto operator()(HeadLiteral const &lit) const -> SimplifyResult<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> SimplifyResult<HeadLiteral> {
        auto [state, res] = simplify(SimplifyFlags::disjunctive, ctx, lit.lit);
        if (state == SimplifyState::fail) {
            res = LiteralBoolean{location(lit), Sign::none, true};
            state = SimplifyState::top;
        }
        return {state, res.transform([](auto &&res) { return SimpleHeadLiteral{GRINGO_FWD(res)}; })};
    }

    auto operator()(HeadSetAggregate const &lit) const -> SimplifyResult<HeadLiteral> {
        auto res_lhs = simplify_guard(lit.lhs);
        auto res_rhs = simplify_guard(lit.rhs);
        AuxTermVec aux;
        auto res_elems = SimplifyVec{lit.elems};
        bool constant = true;
        for (auto const &elem : lit.elems) {
            auto sub = SimplifyHBLiteral{SimplifyContext{ctx.log, ctx.store, ctx.gen, aux}};
            auto [state_elem, res_elem] = sub.simplify_element(elem);
            if (state_elem == SimplifyState::fail || state_elem == SimplifyState::bot) {
                res_elems.remove();
                continue;
            }
            if (state_elem == SimplifyState::unknown) {
                constant = false;
            }
            res_elems.update(std::move(res_elem));
        }
        if ((res_lhs.has_value() && res_lhs->state == SimplifyState::fail) ||
            (res_rhs.has_value() && res_rhs->state == SimplifyState::fail)) {
            return {SimplifyState::top, SimpleHeadLiteral{LiteralBoolean{location(lit), Sign::none, true}}};
        }
        if (constant) {
            throw std::logic_error("we can turn this into a relation literal by accumulating values");
        }
        // TODO: think about transforming this into a general head aggregate right away
        // to avoid the complexity of handling two kinds of aggregates later
        if ((res_lhs.has_value() && res_lhs->value.has_value()) ||
            (res_rhs.has_value() && res_rhs->value.has_value()) || res_elems.has_value()) {
            auto lhs =
                lit.lhs.and_then([&res_lhs](auto const &orig) { return std::move(res_lhs->value).value_or(orig); });
            auto rhs =
                lit.rhs.and_then([&res_rhs](auto const &orig) { return std::move(res_rhs->value).value_or(orig); });
            return {SimplifyState::unknown,
                    HeadSetAggregate{lit.loc, std::move(lhs), std::move(res_elems).value(), std::move(rhs)}};
        }
        return {SimplifyState::unknown};
    }

    auto operator()(HeadAggregate const &lit) const -> SimplifyResult<HeadLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me");
    }

    auto operator()(HeadTheoryAtom const &lit) const -> SimplifyResult<HeadLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me");
    }
};

struct SimplifyBodyLiteral : SimplifyHBLiteral {
    using SimplifyHBLiteral::simplify_element;

    [[nodiscard]] static auto simplify_element(BodyAggregate::Element const &elem)
        -> SimplifyResult<BodyAggregate::Element> {
        static_cast<void>(elem);
        throw std::logic_error("implement me");
    }

    auto operator()(auto const &lit) const -> SimplifyResult<BodyLiteral> = delete;

    using SimplifyHBLiteral::operator();

    auto operator()(BodyLiteral const &lit) const -> SimplifyResult<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> SimplifyResult<BodyLiteral> {
        auto [state, res] = simplify(SimplifyFlags::matchable, ctx, lit.lit);
        if (state == SimplifyState::fail) {
            res = LiteralBoolean{location(lit), Sign::none, false};
            state = SimplifyState::bot;
        }
        return {state, res.transform([](auto &&res) { return SimpleBodyLiteral{GRINGO_FWD(res)}; })};
    }

    auto operator()(BodySetAggregate const &lit) const -> SimplifyResult<BodyLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me");
    }

    auto operator()(BodyAggregate const &lit) const -> SimplifyResult<BodyLiteral> {
        static_cast<void>(lit);
        throw std::logic_error("implement me");
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
        auto state_body = SimplifyState::top;
        for (auto const &lit : body) {
            auto [state_lit, res_lit] = simplify(ctx, lit);
            assert(state_lit != SimplifyState::fail);
            // ensure that all literals are processed to emit all messages
            if (state_body == SimplifyState::bot) {
                continue;
            }
            if (state_lit == SimplifyState::top) {
                res_body.remove();
            } else {
                res_body.update(std::move(res_lit));
            }
            if (state_lit == SimplifyState::bot) {
                if (body.size() != 1) {
                    res_body.opt_value() = Util::make_vec<BodyLiteral>(
                        SimpleBodyLiteral{LiteralBoolean{location(lit), Sign::none, false}});
                }
                state_body = SimplifyState::bot;
            } else if (state_lit == SimplifyState::unknown) {
                state_body = SimplifyState::unknown;
            }
        }
        if (state_body != SimplifyState::bot) {
            res_body.extend(ctx.aux);
        }
        return {state_body, std::move(res_body).opt_value()};
    }

    auto operator()(auto const &lit) const -> SimplifyResult<Statement> = delete;

    auto operator()(Statement const &stm) const -> SimplifyResult<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> SimplifyResult<Statement> {
        auto [state_head, res_head] = simplify(ctx, stm.head);
        auto [state_body, res_body] = simplify_body(stm.body);
        auto state = SimplifyState::unknown;
        assert(state_head != SimplifyState::fail && state_body != SimplifyState::fail);
        if (state_head == SimplifyState::top || state_body == SimplifyState::bot) {
            if (!stm.body.empty()) {
                res_head = SimpleHeadLiteral{LiteralBoolean{location(stm.head), Sign::none, true}};
                res_body = BodyLiteralVec{};
            }
            state = SimplifyState::top;
        }
        if (state_head == SimplifyState::bot && state_body == SimplifyState::top) {
            state = SimplifyState::bot;
        }
        if (res_head.has_value() || res_body.has_value()) {
            return {state,
                    Rule{stm.loc, std::move(res_head).value_or(stm.head), std::move(res_body).value_or(stm.body)}};
        }
        return {state};
    }

    auto operator()(TheoryDefinition const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {SimplifyState::unknown};
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
        return {SimplifyState::unknown};
    }

    auto operator()(StatementProject const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementProjectSig const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {SimplifyState::unknown};
    }

    auto operator()(StatementDefined const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {SimplifyState::unknown};
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
        return {SimplifyState::unknown};
    }

    auto operator()(StatementInclude const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {SimplifyState::unknown};
    }

    auto operator()(StatementProgram const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {SimplifyState::unknown};
    }

    auto operator()(StatementConst const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(Comment const &stm) const -> SimplifyResult<Statement> {
        static_cast<void>(stm);
        return {SimplifyState::unknown};
    }

    SimplifyContext ctx; //!< Context used during simplification.
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

[[nodiscard]] auto is_numeric(Term const &term) -> bool { return IsNumeric{}(term); }

[[nodiscard]] auto simplify(SimplifyFlags flags, SimplifyContext ctx, Term const &term) -> SimplifyResult<Term> {
    auto make_matchable = [&](auto &&target, bool self = true) -> SimplifyResult<Term> {
        if (test(flags, SimplifyFlags::matchable)) {
            if (auto ret = MakeMatchableTerm{ctx}(target, flags); ret.has_value()) {
                return {SimplifyState::unknown, std::move(ret).value()};
            }
        }
        if (self && target != term) {
            return {SimplifyState::unknown, std::forward<decltype(target)>(target)};
        }
        return {SimplifyState::unknown};
    };
    auto simp = SimplifyTerm{ctx};
    return std::visit(
        [&](auto &&res) -> SimplifyResult<Term> {
            GRINGO_MATCH(res, SimplifyTerm::ResultFail) { return {SimplifyState::fail}; }
            GRINGO_MATCH(res, SimplifyTerm::ResultUnchanged) { return make_matchable(term, false); }
            GRINGO_MATCH(res, SimplifyTerm::ResultSymbol) {
                auto sym = Term{TermSymbol{location(term), res}};
                if (sym != term) {
                    return {SimplifyState::unknown, std::move(sym)};
                }
                return {SimplifyState::unknown};
            }
            GRINGO_MATCH(res, SimplifyTerm::ResultChanged) { return make_matchable(res.term); }
            GRINGO_MATCH(res, SimplifyTerm::ResultLinear) {
                return make_matchable(simp.linear_as_term(std::move(res), false));
            }
        },
        simp(term, flags));
}

[[nodiscard]] auto simplify(SimplifyFlags flags, SimplifyContext ctx, Literal const &lit) -> SimplifyResult<Literal> {
    return SimplifyLiteral{ctx}(lit, flags);
}

[[nodiscard]] auto simplify(SimplifyContext ctx, HeadLiteral const &lit) -> SimplifyResult<HeadLiteral> {
    return SimplifyHeadLiteral{ctx}(lit);
}

[[nodiscard]] auto simplify(SimplifyContext ctx, BodyLiteral const &lit) -> SimplifyResult<BodyLiteral> {
    return SimplifyBodyLiteral{ctx}(lit);
}

[[nodiscard]] auto simplify(Logger &log, SymbolStore &store, Statement const &stm) -> SimplifyResult<Statement> {
    AuxTermVec aux;
    NameGen gen{store, {}, "__A_"};
    return SimplifyStatement{SimplifyContext{log, store, gen, aux}}(stm);
}

} // namespace Gringo::Input
