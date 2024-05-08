#include <gringo/ground/aggregate.hh>

#include <iostream>

namespace Gringo::Ground {

void StmCondLitEmpty::print(std::ostream &out) const { out << "TOOD: empty cond lit"; }

auto StmCondLitEmpty::body() const -> ULitVec const & { return body_; }

auto StmCondLitEmpty::important() const -> VariableSet { return {base_->global.begin(), base_->global.end()}; }

void StmCondLitEmpty::init(size_t gen) { std::cerr << "init cond lit with generation: " << gen << "\n"; }

void StmCondLitEmpty::report([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass) {
    std::cerr << "report cond lit\n";
}

void StmCondLitEmpty::propagate([[maybe_unused]] Queue &queue) { std::cerr << "propagate cond lit\n"; }

auto StmCondLitEmpty::priority() const -> size_t { return prio_; }

} // namespace Gringo::Ground
