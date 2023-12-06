#include <util/algorithm.hh>
#include <util/checked_math.hh>

#include <input/algo/evaluate.hh>
#include <input/algo/print.hh>

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

    void operator()(Projection const &pro) const { static_cast<void>(pro); }

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
    Util::unordered_map<String, size_t> &map;
    //! The dependency graph to build.
    Graph &dep;
    //! The id of the const statement at hand.
    size_t id;
};

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

    auto operator()(Symbol sym) const -> std::optional<Symbol> {
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
                    auto [type, rep] = it->second;
                    if (sym.has_sign()) {
                        return evaluate(store, UnaryOperator::negate, rep);
                    }
                    return rep;
                }
                return std::visit(
                    [this, sym](auto res) -> std::optional<Symbol> {
                        GRINGO_MATCH(res, bool) {
                            if (res) {
                                return sym;
                            }
                            return std::nullopt;
                        }
                        GRINGO_MATCH(res, std::vector<Symbol>) { return store.fun(sym.name(), res, sym.has_sign()); }
                    },
                    eval_args_(args));
            }
            case SymbolType::tuple: {
                return std::visit(
                    [this, sym](auto res) -> std::optional<Symbol> {
                        GRINGO_MATCH(res, bool) {
                            if (res) {
                                return sym;
                            }
                            return std::nullopt;
                        }
                        GRINGO_MATCH(res, std::vector<Symbol>) { return store.tup(res); }
                    },
                    eval_args_(sym.args()));
            }
        }
        return sym;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Symbol> { return std::visit(*this, term); }

    auto operator()(Projection const &pro) const -> std::optional<Symbol> {
        static_cast<void>(pro);
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
        if (term.pool.size() != 1 || term.external) {
            return std::nullopt;
        }
        if (term.pool.front().empty()) {
            if (auto it = map.find(term.name); it != map.end()) {
                return it->second.second;
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
            return std::nullopt;
        }
        auto val = operator()(term.pool.front());
        if (!val.has_value()) {
            return std::nullopt;
        }
        if (val->type() == SymbolType::number) {
            val = store.num(abs(*val->num()));
        } else {
            val = std::nullopt;
        }
        if (!val.has_value()) {
            GRINGO_REPORT_LOC(log, error, location(term)) << "operation undefined:\n"
                                                          << "  |" << val.value() << "|\n"
                                                          << location(root) << ": note: operation appears in:\n"
                                                          << "  " << root << "\n";
            return std::nullopt;
        }
        return val;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        auto rhs = operator()(*term.rhs);
        if (!rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(store, term.op, rhs.value());
        if (!res.has_value()) {
            auto const *lp = "";
            auto const *rp = "";
            if (rhs->has_sign()) {
                lp = "(";
                rp = ")";
            }
            GRINGO_REPORT_LOC(log, error, location(term)) << "operation undefined:\n"
                                                          << "  " << term.op << lp << rhs.value() << rp << "\n"
                                                          << location(root) << ": note: operation appears in:\n"
                                                          << "  " << root << "\n";
        }
        return res;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        auto lhs = operator()(*term.lhs);
        auto rhs = operator()(*term.rhs);
        if (term.op == BinaryOperator::dots || !lhs.has_value() || !rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(store, lhs.value(), term.op, rhs.value());
        if (!res.has_value()) {
            auto const *lp = "";
            auto const *rp = "";
            if (rhs->has_sign()) {
                lp = "(";
                rp = ")";
            }
            GRINGO_REPORT_LOC(log, error, location(term))
                << "operation undefined:\n"
                << "  " << lhs.value() << term.op << lp << rhs.value() << rp << "\n"
                << location(root) << ": note: operation appears in:\n"
                << "  " << root << "\n";
        }
        return res;
    }

    Logger &log;
    SymbolStore &store;
    ConstMap const &map;
    StatementConst const &root;
};

auto evaluate(Logger &log, SymbolStore &store, ConstMap const &map, StatementConst const &stm)
    -> std::optional<Symbol> {
    return std::visit(Evaluate{log, store, map, stm}, stm.value);
}

} // namespace

auto evaluate(SymbolStore &store, UnaryOperator op, Symbol rhs) -> std::optional<Symbol> {
    if (op == UnaryOperator::negate) {
        if (rhs.type() == SymbolType::number) {
            return store.num(-*rhs.num());
        }
        return rhs.flip_classical_sign();
    }
    if (rhs.type() == SymbolType::number) {
        return store.num(~*rhs.num());
    }
    return std::nullopt;
}

auto evaluate(Symbol lhs, Relation rel, Symbol rhs) -> bool {
    switch (rel) {
        case Relation::equal: {
            return lhs == rhs;
        }
        case Relation::inequal: {
            return lhs != rhs;
        }
        case Relation::less: {
            return lhs < rhs;
        }
        case Relation::less_equal: {
            return lhs <= rhs;
        }
        case Relation::greater: {
            return lhs > rhs;
        }
        case Relation::greater_equal: {
            break;
        }
    }
    return lhs >= rhs;
}

auto evaluate(SymbolStore &store, Symbol lhs, BinaryOperator op, Symbol rhs) -> std::optional<Symbol> {
    if (lhs.type() != SymbolType::number || rhs.type() != SymbolType::number) {
        return std::nullopt;
    }
    switch (op) {
        case BinaryOperator::dots: {
            break;
        }
        case BinaryOperator::xor_: {
            return store.num(*lhs.num() ^ *rhs.num());
        }
        case BinaryOperator::or_: {
            return store.num(*lhs.num() | *rhs.num());
        }
        case BinaryOperator::and_: {
            return store.num(*lhs.num() & *rhs.num());
        }
        case BinaryOperator::plus: {
            return store.num(*lhs.num() + *rhs.num());
        }
        case BinaryOperator::minus: {
            return store.num(*lhs.num() - *rhs.num());
        }
        case BinaryOperator::times: {
            return store.num(*lhs.num() * *rhs.num());
        }
        case BinaryOperator::div: {
            if (*rhs.num() == 0) {
                return std::nullopt;
            }
            return store.num(*lhs.num() / *rhs.num());
        }
        case BinaryOperator::mod: {
            if (*rhs.num() == 0) {
                return std::nullopt;
            }
            return store.num(*lhs.num() % *rhs.num());
        }
        case BinaryOperator::pow: {
            if (*rhs.num() < 0) {
                return std::nullopt;
            }
            return store.num(pow(*lhs.num(), *rhs.num()));
        }
    }
    throw std::runtime_error("cannot evaluate intervals");
}

void evaluate_const(Logger &log, SymbolStore &store, std::vector<StatementConst> const &stms, ConstMap &res) {
    // build map
    Util::unordered_map<String, size_t> map;
    size_t id_stm = 0;
    for (auto const &stm_a : stms) {
        auto res = map.try_emplace(stm_a.name, id_stm);
        if (!res.second) {
            auto const &stm_b = stms[res.first->second];
            if (stm_b.type < stm_a.type) {
                res.first.value() = id_stm;
            } else if (stm_b.type == stm_a.type) {
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
    for (const auto &[name, id_stm] : map) {
        BuildDep{map, dep, id_stm}(stms[id_stm].value);
    }
    // evaluate const statements
    dep.tarjan([&log, &store, &stms, &map, &res](auto const &scc) {
        if (scc.size() == 1) {
            auto const &stm = stms[scc.front()];
            if (map[stm.name] == scc.front()) {
                if (auto value = evaluate(log, store, res, stm); value) {
                    auto [it, ins] = res.try_emplace(stm.name, stm, *value);
                    if (!ins) {
                        if (it->second.first.type < stm.type) {
                            it.value() = std::make_pair(stm, *value);
                        } else if (it->second.first.type == stm.type) {
                            GRINGO_REPORT_LOC(log, error, location(stm))
                                << "redefinition of constant:\n"
                                << "  " << stm << "\n"
                                << location(it->second.first) << ": note: redefinition of constant:\n"
                                << "  " << it->second.first << "\n";
                        }
                    }
                }
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
            }
            GRINGO_REPORT_STR(log, error, oss.str());
        }
    });
}

} // namespace Gringo::Input
