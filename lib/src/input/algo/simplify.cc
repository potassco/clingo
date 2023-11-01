#include <util/algorithm.hh>
#include <util/checked_math.hh>

#include <input/algo/evaluate.hh>
#include <input/algo/simplify.hh>

/*
whole process as in gringo atm
1. apply #const statements (partially done)
2. unpool (done)
3. init theory
4. simplify (done for terms)
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

struct TermMap {};

//! Simplify a term.
struct SimplifyTerm {
    enum class Type {
        numeric,
        symbolic,
        tuple,
        any,
    };

    struct ResultFail {};
    struct ResultChanged {
        Type type;
        Term term;
    };
    struct ResultLinear {
        Term x;
        Number m;
        Number n;
    };
    using ResultSymbol = Symbol;
    using ResultUnchanged = Type;

    using Result = std::variant<ResultFail, ResultSymbol, ResultUnchanged, ResultChanged, ResultLinear>;

    using Projected = std::monostate;
    using ResultTupleChanged = std::vector<std::variant<Projected, Symbol, Term>>;
    struct ResultTupleUnchanged {};
    struct ResultTupleFail {};
    using ResultTuple = std::variant<ResultTupleFail, ResultTupleUnchanged, ResultTupleChanged>;

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
            mxn = TermBinary(loc, TermSymbol{loc, store.num(std::move(res.m))}, BinaryOperator::times, std::move(mxn));
        }
        if (!simplify || res.n != 0) {
            mxn = TermBinary(loc, std::move(mxn), BinaryOperator::plus, TermSymbol{loc, store.num(std::move(res.n))});
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
    auto simplify_tuple(SimplifyFlags flags, TupleVec const &tuple, bool &constant) const -> ResultTuple {
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
                    // TODO: error message
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
        for (auto const &arg : tuple) {
            if (!std::visit(simplify, arg)) {
                return ResultTupleFail{};
            }
            ++n;
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

    auto operator()(Term const &term, SimplifyFlags flags) const -> Result {
        return std::visit(*this, term, std::variant<SimplifyFlags>{flags});
    }

    auto operator()(auto const &term, SimplifyFlags flags) const -> Result = delete;

    auto operator()(TermSymbol const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(flags);
        return term.value;
    }

    auto operator()(TermVariable const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        // a variable can represent any term
        return Type::any;
    }

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
                        aux.emplace_back(TermVariable{term.loc, gen.new_name()}, term);
                        return ResultChanged{type, aux.back().first};
                    }
                    if (!constant) {
                        return type;
                    }
                    return store.fun(term.name, {}, false);
                }
                GRINGO_MATCH(res, ResultTupleChanged) {
                    if (!constant) {
                        auto fun =
                            TermFunction{term.loc, term.name,
                                         Util::make_vec<TupleVec>(args_term(tuple, std::move(res))), term.external};
                        if (term.external) {
                            aux.emplace_back(TermVariable{term.loc, gen.new_name()}, std::move(fun));
                            return ResultChanged{type, aux.back().first};
                        }
                        // Note: this is somewhat inefficient because the
                        // equality comparision recurses into the structure
                        return check_change(type, term, std::move(fun));
                    }
                    return store.fun(term.name, args_symbol(std::move(res)), false);
                }
            },
            simplify_tuple(flags, tuple, constant));
    }

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
                    return store.tup(args_symbol({}));
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
                    return store.tup(args_symbol(std::move(res)));
                }
                // the term evaluated to a symbol
            },
            simplify_tuple(flags, tuple, constant));
    }

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
                    // TODO: info message???
                    return ResultFail{};
                }
                return store.num(abs(*res.num()));
            }
            GRINGO_MATCH(res, ResultLinear) {
                TermVec pool;
                pool.emplace_back(linear_as_term(std::move(res)));
                return check_change(Type::numeric, term, TermAbs(term.loc, std::move(pool)));
            }
            // the argument did not change
            GRINGO_MATCH(res, ResultUnchanged) {
                if (res == Type::symbolic || res == Type::tuple) {
                    // TODO: info message???
                    return ResultFail{};
                }
                return ResultUnchanged{Type::numeric};
            }
            // the argument changed
            GRINGO_MATCH(res, ResultChanged) {
                // handle invalid terms
                if (res.type == Type::symbolic || res.type == Type::tuple) {
                    // TODO: info message
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

    auto operator()(TermUnary const &term, SimplifyFlags flags) const -> Result {
        // the term and nested terms are not projectable
        flags &= ~SimplifyFlags::projectable;

        auto simplify = [&term, this](auto &&res) -> Result {
            // evaluation of argument failed
            GRINGO_MATCH(res, ResultFail) { return ResultFail{}; }
            // the argument evaluated to a symbol
            GRINGO_MATCH(res, ResultSymbol) {
                // we can always evaluate constants
                auto opt_sym = evaluate(store, term.op, res);
                if (!opt_sym.has_value()) {
                    // TODO: info message???
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
            auto check_type = [&term](Type type) -> std::optional<Type> {
                if (type == Type::tuple || (term.op == UnaryOperator::invert && type == Type::symbolic)) {
                    // TODO: info message???
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
                    // TODO: error messages
                    return {};
                }
                auto name = gen.new_name();
                aux.emplace_back(TermVariable{term.loc, name},
                                 TermBinary{term.loc, result_as_term(term.lhs, std::move(res_lhs)),
                                            BinaryOperator::dots, result_as_term(term.rhs, std::move(res_rhs))});
                return ResultLinear{aux.back().first, Number{1}, Number{0}};
            };
            return std::visit(simplify, operator()(*term.lhs, flags), operator()(*term.rhs, flags));
        }
        auto simplify = [&, this](auto &&res_lhs, auto &&res_rhs) -> Result {
            // check arguments
            if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                // TODO: error messages
                return {};
            }

            // evaluate to symbol
            GRINGO_MATCH2(res_lhs, Symbol, res_rhs, Symbol) {
                auto res = evaluate(store, res_lhs, term.op, res_rhs);
                if (!res.has_value()) {
                    // TODO: info message???
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

    SymbolStore &store;
    NameGen &gen;
    AuxTermVec &aux;
};

//! Make a term matchable.
struct MakeMatchableTerm {
    using Result = std::optional<Term>;
    using ResultTuple = std::optional<TupleVec>;

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
                if (auto res_arg = operator()(arg, flags); res_arg.has_value() || res_tuple.has_value()) {
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

    auto operator()(Term const &term, SimplifyFlags flags) const -> Result {
        return std::visit(*this, term, std::variant<SimplifyFlags>{flags});
    }

    auto operator()(TermSymbol const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(term);
        static_cast<void>(flags);
        return std::nullopt;
    }

    auto operator()(TermFunction const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1);
        return handle_tuple(flags, term.pool.front()).transform([&term](auto &&args) {
            return TermFunction{term.loc, term.name, Util::make_vec<TupleVec>(std::move(args)), term.external};
        });
    }

    auto operator()(TermTuple const &term, SimplifyFlags flags) const -> Result {
        assert(term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front()));
        return handle_tuple(flags, std::get<TupleVec>(term.pool.front())).transform([&term](auto &&args) {
            return TermTuple{term.loc, Util::make_vec<TermTuple::Element>(std::move(args))};
        });
    }

    auto operator()(TermAbs const &term, SimplifyFlags flags) const -> Result {
        static_cast<void>(flags);
        auto name = gen.new_name();
        aux.emplace_back(TermVariable{term.loc, name}, term);
        return aux.back().first;
    }

    auto operator()(TermUnary const &term, SimplifyFlags flags) const -> Result {
        if (!test(flags, SimplifyFlags::unfailable) && term.op == UnaryOperator::negate) {
            return operator()(*term.rhs, flags).transform([&term](auto &&arg) -> Term {
                return TermUnary{term.loc, term.op, Util::construct_shared<Term>(std::forward<decltype(arg)>(arg))};
            });
        }
        auto name = gen.new_name();
        aux.emplace_back(TermVariable{term.loc, name}, term);
        return aux.back().first;
    }

    auto operator()(TermBinary const &term, SimplifyFlags flags) const -> Result {
        if (!test(flags, SimplifyFlags::unfailable) && is_linear(term)) {
            return std::nullopt;
        }
        auto name = gen.new_name();
        aux.emplace_back(TermVariable{term.loc, name}, term);
        return aux.back().first;
    }

    SymbolStore &store;
    NameGen &gen;
    AuxTermVec &aux;
};

} // namespace

/*
struct RewriteArithmetics : Transformer<RewriteArithmetics> {
    RewriteArithmetics(TermMap &map) : map{map} {}

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // ignore

    auto operator()(std::monostate const &x) const -> std::optional<std::monostate> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(String const &x) const -> std::optional<String> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(Relation const &x) const -> std::optional<Relation> {
        static_cast<void>(x);
        return std::nullopt;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermFunction const &term) const -> std::optional<Term> {
        return transform_construct<TermFunction>(term.loc, term.name, tr(term.pool), term.external);
    }

    auto operator()(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct<TermTuple>(term.loc, tr(term.pool));
    }

    auto operator()(TermAbs const &term) const -> std::optional<Term> {
        return transform_construct<TermAbs>(term.loc, tr(term.pool));
    }

    auto operator()(TermUnary const &term) const -> std::optional<Term> {
        return transform_construct<TermUnary>(term.loc, term.op, tr(term.rhs));
    }

    auto operator()(TermBinary const &term) const -> std::optional<Term> {
        return transform_construct<TermBinary>(term.loc, tr(term.lhs), term.op, tr(term.rhs));
    }

    // theory

    auto operator()(TheoryTerm const &term) const -> std::optional<TheoryTerm> { return std::visit(*this, term); }

    auto operator()(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermUnparsed>(term.loc, tr(term.elems));
    }

    auto operator()(TheoryTermSymbol const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TheoryTermVariable const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TheoryTermTuple const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermTuple>(term.loc, term.type, tr(term.elems));
    }

    auto operator()(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermFunction>(term.loc, term.name, tr(term.args));
    }

    // literal

    auto operator()(Literal const &lit) const { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit) const -> std::optional<Literal> {
        return transform_construct<LiteralRelation>(lit.loc, lit.sign, tr(lit.lhs), tr(lit.rhs));
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        return transform_construct<LiteralSymbolic>(lit.loc, lit.sign, tr(lit.term));
    }

    // conditional literal

    auto operator()(ConditionalLiteral const &lit) const -> std::optional<ConditionalLiteral> {
        return transform_construct<ConditionalLiteral>(lit.loc, tr(lit.lits), tr(lit.cond));
    }

    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const
        -> std::optional<std::conditional_t<Conjunctive, BodyLiteral, HeadLiteral>> {
        return transform_construct<Junction<Conjunctive>>(lit.loc, tr(lit.elems));
    }

    // set aggregate

    auto operator()(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        return transform_construct<SetAggregateElement>(tr(elem.lit), tr(elem.cond));
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteral> { return operator()(lit.lit); }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(lit.loc, tr(lit.lhs), tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(tr(elem.tuple), tr(elem.lit), tr(elem.cond));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.loc, tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(lit.loc, tr(lit.name), tr(lit.elems), tr(lit.rhs));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<BodyLiteral> { return operator()(lit.lit); }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.loc, lit.sign, tr(lit.lhs), tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        return transform_construct<BodyAggregate::Element>(tr(elem.tuple), tr(elem.cond));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.loc, lit.sign, tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.loc, lit.sign, tr(lit.name), tr(lit.elems), tr(lit.rhs));
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> std::optional<Statement> {
        return transform_construct<Rule>(stm.loc, tr(stm.head), tr(stm.body));
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementOptimize::Tuple const &elem) const -> std::optional<StatementOptimize::Tuple> {
        return transform_construct<StatementOptimize::Tuple>(tr(elem.weight), tr(elem.priority), tr(elem.terms));
    }

    auto operator()(StatementOptimize::Element const &elem) const -> std::optional<StatementOptimize::Element> {
        return transform_construct<StatementOptimize::Element>(tr(elem.first), tr(elem.second));
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementOptimize>(stm.loc, stm.type, tr(stm.elems));
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementWeakConstraint>(stm.loc, tr(stm.body), tr(stm.tuple));
    }

    auto operator()(StatementShow const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementShow>(stm.loc, tr(stm.term), tr(stm.body));
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementProject>(stm.loc, tr(stm.term), tr(stm.body));
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementExternal>(stm.loc, tr(stm.term), tr(stm.body), tr(stm.type));
    }

    auto operator()(StatementEdge::Edge const &edge) const -> std::optional<StatementEdge::Edge> {
        return transform_construct<StatementEdge::Edge>(tr(edge.u), tr(edge.v));
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.loc, tr(stm.edges), tr(stm.body));
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementHeuristic>(stm.loc, tr(stm.atom), tr(stm.body), tr(stm.type), tr(stm.prio),
                                                       tr(stm.mod));
    }

    auto operator()(StatementScript const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementConst const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(Comment const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    TermMap &map;
};

*/

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

[[nodiscard]] auto simplify(SimplifyFlags flags, SymbolStore &store, NameGen &gen, AuxTermVec &aux, Term const &term)
    -> std::variant<std::monostate, std::nullopt_t, Symbol, Term> {
    auto make_matchable = [&](auto &&term,
                              bool self = true) -> std::variant<std::monostate, std::nullopt_t, Symbol, Term> {
        if (test(flags, SimplifyFlags::matchable)) {
            if (auto ret = MakeMatchableTerm{store, gen, aux}(term, flags); ret.has_value()) {
                return ret.value();
            }
        }
        if (self) {
            return std::forward<decltype(term)>(term);
        }
        return std::nullopt;
    };
    auto simp = SimplifyTerm{store, gen, aux};
    return std::visit(
        [&](auto &&res) -> std::variant<std::monostate, std::nullopt_t, Symbol, Term> {
            GRINGO_MATCH(res, SimplifyTerm::ResultFail) { return {}; }
            GRINGO_MATCH(res, SimplifyTerm::ResultUnchanged) { return make_matchable(term, false); }
            GRINGO_MATCH(res, SimplifyTerm::ResultSymbol) { return res; }
            GRINGO_MATCH(res, SimplifyTerm::ResultChanged) { return make_matchable(res.term); }
            GRINGO_MATCH(res, SimplifyTerm::ResultLinear) {
                return make_matchable(simp.linear_as_term(std::move(res), false));
            }
        },
        simp(term, flags));
}

} // namespace Gringo::Input
