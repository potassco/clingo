#include <deque>
#include <iostream>
#include <map>

#include <input/algo/evaluate.hh>

#include "checked_math.hh"

namespace Gringo::Input {

namespace {

struct EvaluateUnary {
    auto operator()(int val) const -> std::optional<Symbol> {
        // TODO: handle overflows + info
        switch (op) {
            case UnaryOperator::invert: {
                return Symbol{~val};
            }
            case UnaryOperator::negate: {
                break;
            }
        }
        return Symbol{-val};
    }
    auto operator()(Constant val) const -> std::optional<Symbol> {
        static_cast<void>(val);
        // TODO: info reporting
        std::cerr << "could not evaluate symbol" << std::endl;
        return std::nullopt;
    }
    auto operator()(QuotedString val) const -> std::optional<Symbol> {
        static_cast<void>(val);
        // TODO: info reporting
        std::cerr << "could not evaluate symbol" << std::endl;
        return std::nullopt;
    }
    auto operator()(Function val) const -> std::optional<Symbol> {
        switch (op) {
            case UnaryOperator::invert: {
                break;
            }
            case UnaryOperator::negate: {
                if (!val.name.empty()) {
                    return Function{val.name, val.args, !val.has_sign};
                }
                break;
            }
        }
        // TODO: info reporting
        std::cerr << "could not evaluate symbol" << std::endl;
        return std::nullopt;
    }
    UnaryOperator op;
};

struct EvaluateBinary {
    auto operator()(int lhs, int rhs) const -> std::optional<Symbol> {
        // TODO: standard mathematical operators + handle overflows (empty pools) + info
        switch (op) {
            case BinaryOperator::dots: {
                break;
            }
            case BinaryOperator::xor_: {
                return Symbol{lhs ^ rhs};
            }
            case BinaryOperator::or_: {
                return Symbol{lhs | rhs};
            }
            case BinaryOperator::and_: {
                return Symbol{lhs & rhs};
            }
            case BinaryOperator::plus: {
                return check_add(lhs, rhs);
            }
            case BinaryOperator::minus: {
                return check_sub(lhs, rhs);
            }
            case BinaryOperator::times: {
                return Symbol{lhs * rhs};
            }
            case BinaryOperator::div: {
                return check_div(lhs, rhs);
            }
            case BinaryOperator::mod: {
                return Symbol{lhs % rhs};
            }
            case BinaryOperator::pow: {
                return Symbol{static_cast<int>(std::pow(lhs, rhs))};
            }
        }
        // TODO: handle syntax error
        std::cerr << "intervals cannot be used here" << std::endl;
        return std::nullopt;
    }
    template <class L, class R> auto operator()(L const &lhs, R const &rhs) const -> std::optional<Symbol> {
        static_cast<void>(lhs);
        static_cast<void>(rhs);
        // TODO: handle info
        std::cerr << "could not evaluate symbol" << std::endl;
        return std::nullopt;
    }
    BinaryOperator op;
};

struct StateDep {
    StateDep(StatementConst stm) : stm{std::move(stm)} {}
    //! The const directive at hand.
    StatementConst stm;
    //! The constants that depend on this directive.
    std::vector<std::string> rev;
    //! The number of constants this directive depends on.
    size_t dep = 0;
    //! A generation counter to ensure that values are only inserted once into rev.
    size_t gen = 0;
};

struct BuildDep {

    // protect ourselves -> no unintended overloads

    template <class T> void operator()(T const &x) const = delete;

    // term

    void operator()(Term const &term) const { std::visit(*this, term); }

    void operator()(std::monostate const &x) const { static_cast<void>(x); }

    void operator()(TupleElem const &elem) const { std::visit(*this, elem); };

    void operator()(TupleVec const &vec) const {
        for (auto const &elem : vec) {
            std::visit(*this, elem);
        }
    }

    void operator()(TermVariable const &term) const { static_cast<void>(term); }

    void operator()(TermSymbol const &term) const {
        auto const *fun = std::get_if<Function>(&term.value);
        if (fun != nullptr && fun->args.empty()) {
            add_(fun->name);
        }
    }

    void operator()(TermTuple const &term) const {
        for (auto const &term_or_tuple : term.pool) {
            std::visit(*this, term_or_tuple);
        }
    }

    void operator()(TermFunction const &term) const {
        if (!term.external && term.pool.size() == 1 && term.pool.front().empty()) {
            add_(term.name);
        }
        for (auto const &tuple : term.pool) {
            operator()(tuple);
        }
    }

    void operator()(TermAbs const &term) const {
        for (auto const &arg : term.pool) {
            operator()(arg);
        }
    }

    void operator()(TermUnary const &term) const { operator()(*term.rhs); }

    void operator()(TermBinary const &term) const {
        operator()(*term.lhs);
        operator()(*term.rhs);
    }

    //! Add a dependency to the map.
    void add_(std::string const &name) const {
        if (auto it = depend.find(name); it != depend.end() && it->second.gen < gen) {
            it->second.gen = gen;
            ++dep;
            it->second.rev.emplace_back(name);
        }
    }

    //! The dependency map.
    std::map<std::string, StateDep> &depend;
    //! The name of the constant at hand.
    std::optional<std::string> name;
    //! The dependency count of the constant at hand.
    size_t &dep;
    //! Generation of last name inserted.
    size_t gen;
};

struct Evaluate {

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<Symbol> = delete;

    // symbols

    auto operator()(Symbol const &sym) const -> std::optional<Symbol> { return std::visit(*this, sym); }

    auto operator()(int const &val) const -> std::optional<Symbol> { return Symbol{val}; }

    auto operator()(Constant const &val) const -> std::optional<Symbol> { return Symbol{val}; }

    auto operator()(QuotedString const &val) const -> std::optional<Symbol> { return Symbol{val}; }

    auto operator()(Function const &val) const -> std::optional<Symbol> {
        if (val.args.empty()) {
            return Symbol{val};
        }
        auto it = map.find(val.name);
        if (it == map.end()) {
            return Symbol{val};
        }
        auto const &rep = it->second;
        if (rep.has_value() && val.has_sign) {
            return evaluate(UnaryOperator::negate, rep.value());
        }
        return rep;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Symbol> { return std::visit(*this, term); }

    auto operator()(std::monostate const &x) const -> std::optional<Symbol> {
        // TODO: handle syntax error
        std::cerr << "projection is not permitted here" << std::endl;
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(TupleElem const &elem) const -> std::optional<Symbol> { return std::visit(*this, elem); };

    [[nodiscard]] auto eval_(TupleVec const &vec) const -> std::optional<std::vector<Symbol>> {
        std::vector<Symbol> args;
        args.reserve(vec.size());
        for (auto const &elem : vec) {
            auto res = operator()(elem);
            if (!res.has_value()) {
                return std::nullopt;
            }
            args.emplace_back(std::move(res).value());
        }
        return {std::move(args)};
    }

    auto operator()(TupleVec const &vec) const -> std::optional<Symbol> {
        auto args = eval_(vec);
        if (!args.has_value()) {
            return std::nullopt;
        }
        return {Function{"", std::move(args).value()}};
    }

    auto operator()(TermVariable const &term) const -> std::optional<Symbol> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermSymbol const &term) const -> std::optional<Symbol> {
        auto const *fun = std::get_if<Function>(&term.value);
        if (fun != nullptr && fun->args.empty()) {
            if (auto it = map.find(fun->name); it != map.end()) {
                auto const &val = it->second;
                if (val.has_value() && fun->has_sign) {
                    return evaluate(UnaryOperator::negate, val.value());
                }
                return val;
            }
        }
        // also arguments of functions have to be evaluated
        return operator()(term.value);
    }

    auto operator()(TermTuple const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            return std::nullopt;
        }
        return std::visit(*this, term.pool.front());
    }

    auto operator()(TermFunction const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            std::cerr << "pools are not permitted here" << std::endl;
            return std::nullopt;
        }
        if (!term.external && term.pool.front().empty()) {
            if (auto it = map.find(term.name); it != map.end()) {
                return it->second;
            }
        }
        auto args = eval_(term.pool.front());
        if (!args.has_value()) {
            return std::nullopt;
        }
        return Function{term.name, std::move(args).value()};
    }

    auto operator()(TermAbs const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            std::cerr << "pools are not permitted here" << std::endl;
            return std::nullopt;
        }
        auto val = operator()(term.pool.front());
        if (!val.has_value()) {
            return std::nullopt;
        }
        auto const *value = std::get_if<int>(&val.value());
        if (value == nullptr) {
            std::cerr << "can only compute absolute of integers" << std::endl;
            return std::nullopt;
        }
        return Symbol{std::abs(*value)};
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        auto rhs = operator()(*term.rhs);
        if (!rhs.has_value()) {
            return std::nullopt;
        }
        return evaluate(term.op, rhs.value());
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        auto lhs = operator()(*term.lhs);
        auto rhs = operator()(*term.rhs);
        if (!lhs.has_value() || !rhs.has_value()) {
            return std::nullopt;
        }
        return evaluate(lhs.value(), term.op, rhs.value());
    }

    std::map<std::string, std::optional<Symbol>> const &map;
};

} // namespace

auto evaluate(UnaryOperator op, Symbol const &rhs) -> std::optional<Symbol> {
    return std::visit(EvaluateUnary{op}, rhs);
}

auto evaluate(Symbol const &lhs, BinaryOperator op, Symbol const &rhs) -> std::optional<Symbol> {
    return std::visit(EvaluateBinary{op}, lhs, rhs);
}

auto evaluate(std::map<std::string, std::optional<Symbol>> const &map, Term const &term) -> std::optional<Symbol> {
    return std::visit(Evaluate{map}, term);
}

auto evaluate_const(std::vector<StatementConst> const &stms) -> std::map<std::string, std::optional<Symbol>> {
    // build map
    std::map<std::string, StateDep> map;
    for (auto const &stm : stms) {
        auto res = map.try_emplace(stm.name, stm);
        if (!res.second) {
            if (res.first->second.stm.type < stm.type) {
                res.first->second.stm = stm;
            } else {
                // TODO: handle info
                std::cerr << "constant already defined: " << stm.name << std::endl;
            }
        }
    }
    // build dependency graph and initialize counters
    size_t gen = 0;
    for (auto &[name, state] : map) {
        ++gen;
        BuildDep{map, state.stm.name, state.dep, gen}(state.stm.value);
    }
    // initialize queue
    std::deque<std::map<std::string, StateDep>::iterator> todo;
    for (auto it = map.begin(); it != map.end();) {
        if (it->second.dep == 0) {
            todo.emplace_back(it);
        } else {
            ++it;
        }
    }
    // process queue and evaluate
    std::map<std::string, std::optional<Symbol>> res;
    while (!todo.empty()) {
        auto it = todo.front();
        todo.pop_front();
        for (auto const &name : it->second.rev) {
            auto &dep = map.find(name)->second.dep;
            if (--dep; dep == 0) {
                todo.emplace_back(map.find(name));
            }
        }
        res.emplace(it->second.stm.name, evaluate(res, it->second.stm.value));
        map.erase(it);
    }
    // report errors
    if (!map.empty()) {
        std::cerr << "Some constants could not be evaluated because they depend cyclicly on each other.\n"
                  << "To report them nicely, it is easiest to compute strongly connected components.\n"
                  << "Clingo's graph class should be ported for this.\n"
                  << "Furthermore, errors during term evaluation have to be reported properly." << std::endl;
    }
    return res;
}

} // namespace Gringo::Input
