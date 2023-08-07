#include <deque>
#include <iostream>
#include <map>

#include <input/algo/evaluate.hh>
#include <input/algo/print.hh>

#include "checked_math.hh"
#include "graph.hh"

namespace Gringo::Input {

namespace {

struct EvaluateUnary {
    auto operator()(int val) const -> std::optional<Symbol> {
        switch (op) {
            case UnaryOperator::invert: {
                return Symbol{~val};
            }
            case UnaryOperator::negate: {
                break;
            }
        }
        return check_neg(val);
    }
    auto operator()(Constant val) const -> std::optional<Symbol> {
        static_cast<void>(val);
        return std::nullopt;
    }
    auto operator()(QuotedString val) const -> std::optional<Symbol> {
        static_cast<void>(val);
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
        return std::nullopt;
    }
    UnaryOperator op;
};

struct EvaluateBinary {
    auto operator()(int lhs, int rhs) const -> std::optional<Symbol> {
        // Note that the bitwise binary operations on signed integers became
        // well-defined with C++20. Even though this library also supports
        // C++17, we rely on two's complement for integers.
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
                return check_mul(lhs, rhs);
            }
            case BinaryOperator::div: {
                return check_div(lhs, rhs);
            }
            case BinaryOperator::mod: {
                return check_mod(lhs, rhs);
            }
            case BinaryOperator::pow: {
                return check_pow(lhs, rhs);
            }
        }
        throw std::runtime_error("cannot evaluate intervals");
    }
    template <class L, class R> auto operator()(L const &lhs, R const &rhs) const -> std::optional<Symbol> {
        static_cast<void>(lhs);
        static_cast<void>(rhs);
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

    //! Add a dependency to the graph.
    void add_(std::string const &name) const {
        if (auto it = map.find(name); it != map.end()) {
            // TODO: check if the direction is correct
            dep.add_edge(it->second, id);
        }
    }

    //! A map from constant names to indices of const statements.
    std::map<std::string, size_t> &map;
    //! The dependency graph to build.
    Graph &dep;
    //! The id of the const statement at hand.
    size_t id;
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
        if (!val.args.empty()) {
            // Note: this implementation is a bit unfortunate because, ideally,
            // the symbol would only be reconstructed if there are actual
            // changes. This might be noticeable performance-wise because
            // evaluation is a very common operation.
            // However, this is nothing that should be addressed right away.
            // The required code also depends on the eventual symbol
            // implementation that might not even use variants.
            std::vector<Symbol> args;
            args.reserve(val.args.size());
            for (auto const &arg : val.args) {
                auto val = operator()(arg);
                if (!val.has_value()) {
                    return std::nullopt;
                }
                args.emplace_back(val.value());
            }
            return Function{val.name, std::move(args), val.has_sign};
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
        // TODO: proper error handling
        std::cerr << "error: projection is not permitted here" << std::endl;
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
            // TODO: proper error handling
            std::cerr << "error: pools are not permitted here" << std::endl;
            return std::nullopt;
        }
        auto val = operator()(term.pool.front());
        if (!val.has_value()) {
            return std::nullopt;
        }
        auto const *value = std::get_if<int>(&val.value());
        if (value == nullptr) {
            // TODO: proper reporting
            std::cerr << "info: could not evaluate absolute " << Term{term} << " with " << term.pool.front() << "="
                      << val.value() << std::endl;
            return std::nullopt;
        }
        return Symbol{std::abs(*value)};
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        auto rhs = operator()(*term.rhs);
        if (!rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(term.op, rhs.value());
        if (!res.has_value()) {
            // TODO: proper reporting
            std::cerr << "info: could not evaluate unary operation " << Term{term} << " with " << *term.rhs << "="
                      << rhs.value() << std::endl;
        }
        return res;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        auto lhs = operator()(*term.lhs);
        auto rhs = operator()(*term.rhs);
        if (term.op == BinaryOperator::dots) {
            // TODO: proper error handling
            std::cerr << "error: intervals are not permitted here" << std::endl;
            return std::nullopt;
        }
        if (!lhs.has_value() || !rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(lhs.value(), term.op, rhs.value());
        if (!res.has_value()) {
            // TODO: proper reporting
            std::cerr << "info: could not evaluate binary operation " << Term{term} << " with " << *term.lhs << "="
                      << lhs.value() << " and " << *term.rhs << "=" << rhs.value() << std::endl;
        }
        return res;
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
    std::map<std::string, size_t> map;
    size_t id_stm = 0;
    for (auto const &stm_a : stms) {
        auto res = map.try_emplace(stm_a.name, id_stm);
        if (!res.second) {
            auto const &stm_b = stms[res.first->second];
            if (stm_b.type < stm_a.type) {
                res.first->second = id_stm;
            } else {
                // TODO: proper reporting
                std::cerr << "info: constant already defined: " << stm_a.name << std::endl;
            }
        }
        ++id_stm;
    }
    // build dependency graph
    Graph dep;
    for (auto &[name, id_stm] : map) {
        BuildDep{map, dep, id_stm}(stms[id_stm].value);
    }
    // evaluate const statements
    std::map<std::string, std::optional<Symbol>> res;
    dep.tarjan([&stms, &res](auto const &scc) {
        if (scc.size() == 1) {
            auto const &stm = stms[scc.front()];
            res.emplace(stm.name, evaluate(res, stm.value));
        } else {
            for (auto id_stm : scc) {
                res.emplace(stms[id_stm].name, std::nullopt);
            }
            // TODO: proper reporting
            std::cerr << "error: the following const statements depend cyclically on each other:";
            for (auto id_stm : scc) {
                std::cerr << "\n  " << stms[id_stm];
            }
            std::cerr << std::endl;
        }
    });
    return res;
}

} // namespace Gringo::Input
