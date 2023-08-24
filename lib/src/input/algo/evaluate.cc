#include <unordered_map>

#include <util/algorithm.hh>

#include <input/algo/evaluate.hh>
#include <input/algo/print.hh>

#include "checked_math.hh"
#include "graph.hh"
#include "logger.hh"

namespace Gringo::Input {

namespace {

struct StateDep {
    StateDep(StatementConst stm) : stm{std::move(stm)} {}
    //! The const directive at hand.
    StatementConst stm;
    //! The constants that depend on this directive.
    std::vector<String> rev;
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
        if (term.value.type() == SymbolType::function) {
            add_(term.value.name());
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
    void add_(String const &name) const {
        if (auto it = map.find(name); it != map.end()) {
            dep.add_edge(id, it->second);
        }
    }

    //! A map from constant names to indices of const statements.
    std::unordered_map<String, size_t> &map;
    //! The dependency graph to build.
    Graph &dep;
    //! The id of the const statement at hand.
    size_t id;
};

auto num_to_sym(std::optional<int> num) -> std::optional<Symbol> {
    if (num.has_value()) {
        return SymbolStore::num(num.value());
    }
    return std::nullopt;
}

struct Evaluate {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<Symbol> = delete;

    // symbols

    [[nodiscard]] auto eval_args_(SymbolSpan args) const -> std::variant<bool, std::vector<Symbol>> {
        std::optional<std::vector<Symbol>> ret;
        size_t n = 0;
        for (auto sym : args) {
            auto arg_eval = operator()(sym);
            if (!arg_eval.has_value()) {
                return false;
            }
            if (arg_eval != sym) {
                ret = Util::copy_n(args, n);
            }
            if (ret.has_value()) {
                ret->emplace_back(std::move(arg_eval).value());
            }
            ++n;
        }
        if (ret.has_value()) {
            return std::move(ret).value();
        }
        return true;
    }

    auto operator()(Symbol const &sym) const -> std::optional<Symbol> {
        switch (sym.type()) {
            case SymbolType::inf:
            case SymbolType::sup:
            case SymbolType::string:
            case SymbolType::number: {
                break;
            }
            case SymbolType::function: {
                auto args = sym.args();
                if (args.empty()) {
                    auto it = map.find(sym.name());
                    if (it == map.end()) {
                        return sym;
                    }
                    auto rep = it->second;
                    if (rep.has_value() && sym.has_sign()) {
                        return evaluate(UnaryOperator::negate, rep.value());
                    }
                    return rep;
                }
                return std::visit(
                    [this, sym](auto res) -> std::optional<Symbol> {
                        if constexpr (GRINGO_IS_OF_TYPE(res, bool)) {
                            if (res) {
                                return sym;
                            }
                            return std::nullopt;
                        }
                        if constexpr (GRINGO_IS_OF_TYPE(res, std::vector<Symbol>)) {
                            return store.fun(sym.name(), res, sym.has_sign());
                        }
                    },
                    eval_args_(args));
            }
            case SymbolType::tuple: {
                return std::visit(
                    [this, sym](auto res) -> std::optional<Symbol> {
                        if constexpr (GRINGO_IS_OF_TYPE(res, bool)) {
                            if (res) {
                                return sym;
                            }
                            return std::nullopt;
                        }
                        if constexpr (GRINGO_IS_OF_TYPE(res, std::vector<Symbol>)) {
                            return store.tup(res);
                        }
                    },
                    eval_args_(sym.args()));
            }
        }
        return sym;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Symbol> { return std::visit(*this, term); }

    auto operator()(std::monostate const &x) const -> std::optional<Symbol> {
        static_cast<void>(x);
        GRINGO_REPORT_LOC(log, error, location(root)) << "projection is not permitted here\n";
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
        return {store.tup(args.value())};
    }

    auto operator()(TermVariable const &term) const -> std::optional<Symbol> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermSymbol const &term) const -> std::optional<Symbol> { return operator()(term.value); }

    auto operator()(TermTuple const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            return std::nullopt;
        }
        return std::visit(*this, term.pool.front());
    }

    auto operator()(TermFunction const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            GRINGO_REPORT_LOC(log, error, location(term)) << "pools are not permitted here\n";
            return std::nullopt;
        }
        if (term.external) {
            GRINGO_REPORT_LOC(log, error, location(term)) << "external functions are not permitted here\n";
            return std::nullopt;
        }
        if (term.pool.front().empty()) {
            if (auto it = map.find(term.name); it != map.end()) {
                return it->second;
            }
        }
        auto args = eval_(term.pool.front());
        if (!args.has_value()) {
            return std::nullopt;
        }
        return store.fun(term.name, args.value(), false);
    }

    auto operator()(TermAbs const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            GRINGO_REPORT_LOC(log, error, location(term)) << "pools are not permitted here\n";
            return std::nullopt;
        }
        auto val = operator()(term.pool.front());
        if (!val.has_value()) {
            return std::nullopt;
        }
        if (val->type() == SymbolType::number) {
            val = num_to_sym(check_abs(val->num()));
        } else {
            val = std::nullopt;
        }
        if (!val.has_value()) {
            GRINGO_REPORT_LOC(log, info_operation_undefined, location(term)) << "operation undefined:\n"
                                                                             << "  |" << val.value() << "|\n";
            return std::nullopt;
        }
        return val;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        auto rhs = operator()(*term.rhs);
        if (!rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(term.op, rhs.value());
        if (!res.has_value()) {
            auto const *lp = "";
            auto const *rp = "";
            if (rhs->has_sign()) {
                lp = "(";
                rp = ")";
            }
            GRINGO_REPORT_LOC(log, info_operation_undefined, location(term))
                << "operation undefined:\n"
                << "  " << term.op << lp << rhs.value() << rp << "\n";
        }
        return res;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        auto lhs = operator()(*term.lhs);
        auto rhs = operator()(*term.rhs);
        if (term.op == BinaryOperator::dots) {
            GRINGO_REPORT_LOC(log, error, location(term)) << "intervals are not permitted here\n";
            return std::nullopt;
        }
        if (!lhs.has_value() || !rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(lhs.value(), term.op, rhs.value());
        if (!res.has_value()) {
            auto const *lp = "";
            auto const *rp = "";
            if (rhs->has_sign()) {
                lp = "(";
                rp = ")";
            }
            GRINGO_REPORT_LOC(log, info_operation_undefined, location(term))
                << "operation undefined:\n"
                << "  " << lhs.value() << term.op << lp << rhs.value() << rp << "\n";
        }
        return res;
    }

    Logger &log;
    SymbolStore &store;
    std::unordered_map<String, std::optional<Symbol>> const &map;
    Term const &root;
};

} // namespace

auto evaluate(UnaryOperator op, Symbol const &rhs) -> std::optional<Symbol> {
    if (op == UnaryOperator::negate) {
        return rhs.flip_sign();
    }
    if (rhs.type() == SymbolType::number) {
        return SymbolStore::num(~rhs.num());
    }
    return std::nullopt;
}

auto evaluate(Symbol const &lhs, BinaryOperator op, Symbol const &rhs) -> std::optional<Symbol> {
    // Note that the bitwise binary operations on signed integers became
    // well-defined with C++20. Even though this library also supports
    // C++17, we rely on two's complement for integers.
    if (lhs.type() != SymbolType::number || rhs.type() != SymbolType::number) {
        return std::nullopt;
    }
    switch (op) {
        case BinaryOperator::dots: {
            break;
        }
        case BinaryOperator::xor_: {
            return SymbolStore::num(lhs.num() ^ rhs.num());
        }
        case BinaryOperator::or_: {
            return SymbolStore::num(lhs.num() | rhs.num());
        }
        case BinaryOperator::and_: {
            return SymbolStore::num(lhs.num() & rhs.num());
        }
        case BinaryOperator::plus: {
            return num_to_sym(check_add(lhs.num(), rhs.num()));
        }
        case BinaryOperator::minus: {
            return num_to_sym(check_sub(lhs.num(), rhs.num()));
        }
        case BinaryOperator::times: {
            return num_to_sym(check_mul(lhs.num(), rhs.num()));
        }
        case BinaryOperator::div: {
            return num_to_sym(check_div(lhs.num(), rhs.num()));
        }
        case BinaryOperator::mod: {
            return num_to_sym(check_mod(lhs.num(), rhs.num()));
        }
        case BinaryOperator::pow: {
            return num_to_sym(check_pow(lhs.num(), rhs.num()));
        }
    }
    throw std::runtime_error("cannot evaluate intervals");
}

auto evaluate(Logger &log, SymbolStore &store, std::unordered_map<String, std::optional<Symbol>> const &map,
              Term const &term) -> std::optional<Symbol> {
    return std::visit(Evaluate{log, store, map, term}, term);
}

auto evaluate_const(Logger &log, SymbolStore &store, std::vector<StatementConst> const &stms)
    -> std::unordered_map<String, std::optional<Symbol>> {
    // build map
    std::unordered_map<String, size_t> map;
    size_t id_stm = 0;
    for (auto const &stm_a : stms) {
        auto res = map.try_emplace(stm_a.name, id_stm);
        if (!res.second) {
            auto const &stm_b = stms[res.first->second];
            if (stm_b.type < stm_a.type) {
                res.first->second = id_stm;
            } else {
                GRINGO_REPORT_LOC(log, error, location(stm_a))
                    << "redefinition of constant:\n"
                    << "  " << stm_a << "\n"
                    << location(stm_b) << ": note: redefinition of constant:\n"
                    << "  " << stm_b << "\n";
            }
        }
        ++id_stm;
    }
    // build dependency graph
    Graph dep;
    dep.ensure_size(id_stm);
    for (auto &[name, id_stm] : map) {
        BuildDep{map, dep, id_stm}(stms[id_stm].value);
    }
    // evaluate const statements
    std::unordered_map<String, std::optional<Symbol>> res;
    dep.tarjan([&log, &store, &stms, &map, &res](auto const &scc) {
        if (scc.size() == 1) {
            auto const &stm = stms[scc.front()];
            if (map[stm.name] == scc.front()) {
                res.emplace(stm.name, evaluate(log, store, res, stm.value));
            }
        } else {
            bool first = true;
            std::ostringstream oss;
            for (auto id_stm : scc) {
                auto &stm = stms[id_stm];
                oss << location(stm);
                if (first) {
                    oss << ": " << log.message_prefix(MessageCode::error) << ": cyclic constant definition:\n";
                    first = false;
                } else {
                    oss << ": note: cyclic constant definition:\n";
                }
                oss << "  " << stms[id_stm] << "\n";
                if (map[stm.name] == scc.front()) {
                    res.emplace(stm.name, std::nullopt);
                }
            }
            GRINGO_REPORT_STR(log, error, oss.str());
        }
    });
    return res;
}

} // namespace Gringo::Input
