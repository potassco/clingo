#include <iostream>
#include <map>

#include <input/statement.hh>

namespace Gringo::Input {

namespace {

struct BuildDependency {

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

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
            depend.emplace_back(fun->name);
        }
    }

    void operator()(TermTuple const &term) const {
        for (auto const &term_or_tuple : term.pool) {
            std::visit(*this, term_or_tuple);
        }
    }

    void operator()(TermFunction const &term) const {
        if (!term.external && term.pool.size() == 1 && term.pool.front().empty()) {
            depend.emplace_back(term.name);
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

    std::vector<std::string> &depend;
};

} // namespace

auto evaluate_const(std::vector<StatementConst> const &stms) -> std::map<std::string, Symbol> {
    // build map
    std::map<std::string, StatementConst> map;
    for (auto const &stm : stms) {
        auto res = map.try_emplace(stm.name, stm);
        if (!res.second) {
            if (res.first->second.type < stm.type) {
                res.first->second = stm;
            } else {
                std::cerr << "constant already defined: " << stm.name << std::endl;
            }
        }
    }
    // build dependency
    for (auto const &[name, stm] : map) {
        std::vector<std::string> dep;
        BuildDependency bd{dep};
        bd(stm.value);
    }
    // check for cyclic dependencies
    // evaluate const directives
    throw std::logic_error("implement me!!!");
}

} // namespace Gringo::Input
