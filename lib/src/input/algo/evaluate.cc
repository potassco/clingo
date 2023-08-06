#include <deque>
#include <iostream>
#include <map>

#include <input/algo/evaluate.hh>

namespace Gringo::Input {

namespace {

// TODO: add logger for error reporting!!!

struct EvaluateUnary {
    auto operator()(int val) const -> std::optional<Symbol> {
        // TODO: using an optional allows for nice overflow checking
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
        std::cerr << "could not evaluate symbol" << std::endl;
        return std::nullopt;
    }
    auto operator()(QuotedString val) const -> std::optional<Symbol> {
        static_cast<void>(val);
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
                    return Function{val.name, val.args, val.has_sign};
                }
                break;
            }
        }
        std::cerr << "could not evaluate symbol" << std::endl;
        return std::nullopt;
    }
    UnaryOperator op;
};

struct EvaluateBinary {
    auto operator()(int lhs, int rhs) const -> std::optional<Symbol> {
        // TODO: using an optional allows for nice overflow checking
        switch (op) {
            case BinaryOperator::plus: {
                return Symbol{lhs + rhs};
            }
            default: {
                throw std::logic_error("implement me!!!");
            }
        }
    }
    template <class L, class R> auto operator()(L const &lhs, R const &rhs) const -> std::optional<Symbol> {
        static_cast<void>(lhs);
        static_cast<void>(rhs);
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

    // term

    auto operator()(Term const &term) const -> std::optional<Symbol> { return std::visit(*this, term); }

    auto operator()(std::monostate const &x) const -> std::optional<Symbol> {
        std::cerr << "projection is not permitted here" << std::endl;
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(TupleElem const &elem) const -> std::optional<Symbol> { return std::visit(*this, elem); };

    [[nodiscard]] auto eval_(TupleVec const &vec) const -> std::optional<std::vector<Symbol>> {
        std::vector<Symbol> args;
        args.reserve(vec.size());
        for (auto const &elem : vec) {
            auto res = std::visit(*this, elem);
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
                    throw std::logic_error("invert value");
                }
                return val;
            }
        }
        return term.value;
    }

    auto operator()(TermTuple const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            return std::nullopt;
        }
        return std::visit(*this, term.pool.front());
    }

    auto operator()(TermFunction const &term) const -> std::optional<Symbol> {
        if (!term.external && term.pool.size() == 1 && term.pool.front().empty()) {
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
        static_cast<void>(term);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        static_cast<void>(term);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        static_cast<void>(term);
        throw std::logic_error("implement me!!!");
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
