#include <gringo/ground/aggregate.hh>

#include <iostream>

namespace Gringo::Ground {

// StmCondLitEmpty

void StmCondLitEmpty::print(std::ostream &out) const { out << "TOOD: empty cond lit"; }

auto StmCondLitEmpty::body() const -> ULitVec const & { return body_; }

auto StmCondLitEmpty::important() const -> VariableSet { return {base_->global.begin(), base_->global.end()}; }

void StmCondLitEmpty::init(size_t gen) { std::cerr << "init empty cond lit with generation: " << gen << "\n"; }

void StmCondLitEmpty::report([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass) {
    std::cerr << "report cond lit empty\n";
}

void StmCondLitEmpty::propagate([[maybe_unused]] Queue &queue) { std::cerr << "propagate cond lit empty\n"; }

auto StmCondLitEmpty::priority() const -> size_t { return prio_; }

// StmCondLitPremise

void StmCondLitPremise::print(std::ostream &out) const { out << "TOOD: cond lit premise"; }

auto StmCondLitPremise::body() const -> ULitVec const & { return body_; }

auto StmCondLitPremise::important() const -> VariableSet {
    VariableSet res;
    res.reserve(base_->global.size() + base_->local.size());
    res.insert(base_->local.begin(), base_->local.end());
    res.insert(base_->global.begin(), base_->global.end());
    return res;
}

void StmCondLitPremise::init(size_t gen) { std::cerr << "init cond lit with premise generation: " << gen << "\n"; }

void StmCondLitPremise::report([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass) {
    std::cerr << "report cond lit premise\n";
}

void StmCondLitPremise::propagate([[maybe_unused]] Queue &queue) { std::cerr << "propagate cond lit premise\n"; }

auto StmCondLitPremise::priority() const -> size_t { return prio_; }

} // namespace Gringo::Ground
