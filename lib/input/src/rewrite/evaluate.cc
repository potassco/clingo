#include <clingo/input/print.hh>

#include <clingo/input/rewrite/evaluate.hh>

#include <clingo/core/logger.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/checked_math.hh>
#include <clingo/util/graph.hh>
#include <clingo/util/type_traits.hh>

namespace CppClingo::Input {

namespace {

struct StateDep {
    StateDep(StmConst stm) : stm{std::move(stm)} {}
    //! The const directive at hand.
    StmConst stm;
    //! The constants that depend on this directive.
    std::vector<String> rev;
    //! The number of constants this directive depends on.
    size_t dep = 0;
    //! A generation counter to ensure that values are only inserted once into rev.
    size_t gen = 0;
};

class BuildDep {
  public:
    BuildDep(Util::ordered_map<String, size_t> &map, Util::Graph &dep, size_t id) : map_{&map}, dep_{&dep}, id_{id} {}

    // protect ourselves -> no unintended overloads

    template <class T> void operator()(T const &x) const = delete;

    // term

    void operator()(Term const &term) const { std::visit(*this, term); }

    void operator()([[maybe_unused]] TermFormatString const &term) const {}

    void operator()([[maybe_unused]] Projection const &pro) const {}

    void operator()(Argument const &elem) const { std::visit(*this, elem); };

    void operator()(ArgumentTuple const &tuple) const {
        std::for_each(tuple.elems().begin(), tuple.elems().end(), *this);
    }

    void operator()([[maybe_unused]] TermVariable const &term) const {}

    void operator()(TermSymbol const &term) const {
        if (term.value().type() == SymbolType::function) {
            add_(term.value().name());
        }
    }

    void operator()(TupleElement const &elem) const { std::visit(*this, elem); }

    void operator()(TermTuple const &term) const { std::for_each(term.pool().begin(), term.pool().end(), *this); }

    void operator()(TermFunction const &term) const {
        if (!term.external() && term.pool().size() == 1 && term.pool().front().elems().empty()) {
            add_(term.name());
        }
        std::for_each(term.pool().begin(), term.pool().end(), *this);
    }

    void operator()(TermAbs const &term) const { std::for_each(term.pool().begin(), term.pool().end(), *this); }

    void operator()(TermUnary const &term) const { operator()(*term.rhs()); }

    void operator()(TermBinary const &term) const {
        operator()(*term.lhs());
        operator()(*term.rhs());
    }

    //! Add a dependency to the graph.
    void add_(String const &name) const {
        if (auto it = map_->find(name); it != map_->end()) {
            dep_->add_edge(id_, it->second);
        }
    }

  private:
    //! A map from constant names to indices of const statements.
    Util::ordered_map<String, size_t> *map_;
    //! The dependency graph to build.
    Util::Graph *dep_;
    //! The id of the const statement at hand.
    size_t id_;
};

class Evaluate {
  public:
    Evaluate(Logger &log, SymbolStore &store, ConstMap const &map, StmConst const *root)
        : log_{&log}, store_{&store}, map_{&map}, root_{root} {}

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
                ret->emplace_back(arg_eval.value());
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
                    auto it = map_->find(sym.name());
                    if (it == map_->end()) {
                        return sym;
                    }
                    auto [type, rep] = it->second;
                    if (sym.has_sign()) {
                        return evaluate(*store_, UnaryOperator::minus, *rep);
                    }
                    return *rep;
                }
                return std::visit(
                    [this, sym]<class T>(T res) -> std::optional<Symbol> {
                        if constexpr (std::is_same_v<T, bool>) {
                            if (res) {
                                return sym;
                            }
                            return std::nullopt;
                        }
                        if constexpr (std::is_same_v<T, std::vector<Symbol>>) {
                            return store_->fun_ref(sym.name(), res, sym.has_sign());
                        }
                    },
                    eval_args_(args));
            }
            case SymbolType::tuple: {
                return std::visit(
                    [this, sym]<class T>(T res) -> std::optional<Symbol> {
                        if constexpr (std::is_same_v<T, bool>) {
                            if (res) {
                                return sym;
                            }
                            return std::nullopt;
                        }
                        if constexpr (std::is_same_v<T, std::vector<Symbol>>) {
                            return store_->tup_ref(res);
                        }
                    },
                    eval_args_(sym.args()));
            }
        }
        return sym;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Symbol> { return std::visit(*this, term); }

    auto operator()(TermFormatString const &term) const -> std::optional<Symbol> {
        auto res = std::string{};
        for (auto const &field : term.elems()) {
            if (!std::visit(
                    [&res]<class T>(T const &x) {
                        if constexpr (Util::is_among_v<T, FormatFieldLiteral>) {
                            res += x.value().view();
                            return true;
                        } else if constexpr (std::is_same_v<T, FormatFieldExpression>) {
                            return false;
                        } else {
                            static_assert(Util::is_among_v<T, void>);
                        }
                    },
                    field)) {
                return std::nullopt;
            }
        }
        return SymbolStore::str_ref(store_->string_ref(res));
    }

    auto operator()([[maybe_unused]] Projection const &pro) const -> std::optional<Symbol> { return std::nullopt; }

    auto operator()(Argument const &elem) const -> std::optional<Symbol> { return std::visit(*this, elem); };

    [[nodiscard]] auto eval_(ArgumentTuple const &tuple) const -> std::optional<std::vector<Symbol>> {
        std::vector<Symbol> args;
        args.reserve(tuple.elems().size());
        for (auto const &elem : tuple.elems()) {
            auto res = operator()(elem);
            if (!res.has_value()) {
                return std::nullopt;
            }
            args.emplace_back(res.value());
        }
        return {std::move(args)};
    }

    auto operator()(ArgumentTuple const &tuple) const -> std::optional<Symbol> {
        auto args = eval_(tuple);
        if (!args.has_value()) {
            return std::nullopt;
        }
        return {store_->tup_ref(args.value())};
    }

    auto operator()([[maybe_unused]] TermVariable const &term) const -> std::optional<Symbol> { return std::nullopt; }

    auto operator()(TermSymbol const &term) const -> std::optional<Symbol> { return operator()(term.value()); }

    auto operator()(TermTuple const &term) const -> std::optional<Symbol> {
        if (term.pool().size() != 1) {
            return std::nullopt;
        }
        return std::visit(*this, term.pool().front());
    }

    auto operator()(TermFunction const &term) const -> std::optional<Symbol> {
        if (term.pool().size() != 1 || term.external()) {
            return std::nullopt;
        }
        if (term.pool().front().elems().empty()) {
            if (auto it = map_->find(term.name()); it != map_->end()) {
                return *it->second.second;
            }
        }
        auto args = eval_(term.pool().front());
        if (!args.has_value()) {
            return std::nullopt;
        }
        return store_->fun_ref(term.name(), args.value(), false);
    }

    struct ErrorContext {
        friend auto operator<<(std::ostream &out, ErrorContext const &ctx) -> std::ostream & {
            if (ctx.root != nullptr) {
                out << location(*ctx.root) << ": note: operation appears in:\n"
                    << "  " << *ctx.root << "\n";
            }
            return out;
        }
        StmConst const *root;
    };

    auto operator()(TermAbs const &term) const -> std::optional<Symbol> {
        if (term.pool().size() != 1) {
            return std::nullopt;
        }
        auto val = operator()(term.pool().front());
        if (!val.has_value()) {
            return std::nullopt;
        }
        if (val->type() == SymbolType::number) {
            val = store_->num_ref(abs(val->num()));
        } else {
            val = std::nullopt;
        }
        if (!val.has_value()) {
            CLINGO_REPORT_LOC(*log_, info_operation_undefined, location(term)) << "operation undefined:\n"
                                                                               << "  |" << val.value() << "|\n"
                                                                               << ErrorContext{root_};
            return std::nullopt;
        }
        return val;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        auto rhs = operator()(*term.rhs());
        if (!rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(*store_, term.op(), rhs.value());
        if (!res.has_value()) {
            auto const *lp = "";
            auto const *rp = "";
            if (rhs->has_sign()) {
                lp = "(";
                rp = ")";
            }
            CLINGO_REPORT_LOC(*log_, info_operation_undefined, location(term))
                << "operation undefined:\n"
                << "  " << term.op() << lp << rhs.value() << rp << "\n"
                << ErrorContext{root_};
        }
        return res;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        auto lhs = operator()(*term.lhs());
        auto rhs = operator()(*term.rhs());
        if (term.op() == BinaryOperator::dots || !lhs.has_value() || !rhs.has_value()) {
            return std::nullopt;
        }
        auto res = evaluate(*store_, lhs.value(), term.op(), rhs.value());
        if (!res.has_value()) {
            auto const *lp = "";
            auto const *rp = "";
            if (rhs->has_sign()) {
                lp = "(";
                rp = ")";
            }
            CLINGO_REPORT_LOC(*log_, info_operation_undefined, location(term))
                << "operation undefined:\n"
                << "  " << lhs.value() << term.op() << lp << rhs.value() << rp << "\n"
                << ErrorContext{root_};
        }
        return res;
    }

  private:
    Logger *log_;
    SymbolStore *store_;
    ConstMap const *map_;
    StmConst const *root_;
};

[[maybe_unused]] auto evaluate(Logger &log, SymbolStore &store, ConstMap const &map, StmConst const &stm)
    -> std::optional<Symbol> {
    return std::visit(Evaluate{log, store, map, &stm}, stm.value());
}

} // namespace

auto evaluate(SymbolStore &store, UnaryOperator op, Symbol rhs) -> std::optional<Symbol> {
    if (op == UnaryOperator::minus) {
        if (rhs.type() == SymbolType::number) {
            return store.num_ref(-rhs.num());
        }
        return rhs.flip_classical_sign();
    }
    if (rhs.type() == SymbolType::number) {
        return store.num_ref(~rhs.num());
    }
    return std::nullopt;
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
            return store.num_ref(lhs.num() ^ rhs.num());
        }
        case BinaryOperator::or_: {
            return store.num_ref(lhs.num() | rhs.num());
        }
        case BinaryOperator::and_: {
            return store.num_ref(lhs.num() & rhs.num());
        }
        case BinaryOperator::plus: {
            return store.num_ref(lhs.num() + rhs.num());
        }
        case BinaryOperator::minus: {
            return store.num_ref(lhs.num() - rhs.num());
        }
        case BinaryOperator::times: {
            return store.num_ref(lhs.num() * rhs.num());
        }
        case BinaryOperator::div: {
            if (rhs.num() == 0) {
                return std::nullopt;
            }
            return store.num_ref(lhs.num() / rhs.num());
        }
        case BinaryOperator::mod: {
            if (rhs.num() == 0) {
                return std::nullopt;
            }
            return store.num_ref(lhs.num() % rhs.num());
        }
        case BinaryOperator::pow: {
            if (rhs.num() < 0) {
                return std::nullopt;
            }
            return store.num_ref(pow(lhs.num(), rhs.num()));
        }
    }
    throw std::runtime_error("cannot evaluate intervals");
}

void evaluate_const(Logger &log, SymbolStore &store, std::vector<StmConst> const &stms, ConstMap &res) {
    bool ret = true;
    // build map
    Util::ordered_map<String, size_t> map;
    size_t id_stm = 0;
    for (auto const &stm_a : stms) {
        auto res = map.try_emplace(stm_a.name(), id_stm);
        if (!res.second) {
            auto const &stm_b = stms[res.first->second];
            if (stm_b.type() < stm_a.type()) {
                res.first.value() = id_stm;
            } else if (stm_b.type() == stm_a.type()) {
                ret = false;
                CLINGO_REPORT_LOC(log, error, location(stm_a))
                    << "redefinition of constant:\n"
                    << "  " << stm_a << "\n"
                    << location(stm_b) << ": note: redefinition of constant:\n"
                    << "  " << stm_b << "\n";
            }
        }
        ++id_stm;
    }
    // build dependency graph
    Util::Graph dep;
    dep.ensure_size(id_stm);
    for (const auto &[name, id_stm] : map) {
        BuildDep{map, dep, id_stm}(stms[id_stm].value());
    }
    // evaluate const statements
    dep.tarjan([&log, &store, &stms, &map, &res, &ret](auto const &scc) {
        if (scc.size() == 1) {
            auto const &stm = stms[scc.front()];
            if (map[stm.name()] == scc.front()) {
                if (auto value = evaluate(log, store, res, stm); value) {
                    auto [it, ins] = res.try_emplace(SharedString(stm.name()), stm, *value);
                    if (!ins) {
                        if (it->second.first.type() < stm.type()) {
                            it.value() = std::pair(stm, SharedSymbol(*value));
                        } else if (it->second.first.type() == stm.type()) {
                            CLINGO_REPORT_LOC(log, error, location(stm))
                                << "redefinition of constant:\n"
                                << "  " << stm << "\n"
                                << location(it->second.first) << ": note: redefinition of constant:\n"
                                << "  " << it->second.first << "\n";
                            ret = false;
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
            CLINGO_REPORT_STR(log, error, oss.str());
            ret = false;
        }
    });
    if (!ret) {
        throw std::runtime_error("evaluation of const statements failed");
    }
}

auto evaluate(Logger &log, SymbolStore &store, ConstMap const &map, Term const &term) -> std::optional<Symbol> {
    return std::visit(Evaluate{log, store, map, nullptr}, term);
}

} // namespace CppClingo::Input
