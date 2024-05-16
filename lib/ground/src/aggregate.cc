#include <gringo/ground/aggregate.hh>

#include <iostream>
#include <typeindex>

namespace Gringo::Ground {

// LitCondLit

auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream & {
    switch (type) {
        case LitCondLitType::empty: {
            out << "empty";
            break;
        }
        case LitCondLitType::premise: {
            out << "premise";
            break;
        }
        case LitCondLitType::conclusion: {
            out << "conclusion";
            break;
        }
        case LitCondLitType::lit: {
            out << "condlit";
            break;
        }
    }
    return out;
}

void LitCondLit::vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        base_->vars(vars, type_ != LitCondLitType::empty);
    }
}

auto LitCondLit::domain(bool domain) const -> bool {
    std::cerr << "TODO: cond lit " << type_ << " check if base is domain\n";
    return domain;
}

auto LitCondLit::recursive() const -> bool {
    std::cerr << "TODO: cond lit " << type_ << " check if recursive via some index\n";
    return false;
}

auto LitCondLit::matcher(MatcherType type,
                         std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    static_cast<void>(type);
    static_cast<void>(bound);
    // TODO: this should look a bit like a predicate matcher
    throw std::runtime_error("return a matcher!!!");
}

auto LitCondLit::score(std::vector<bool> const &bound) const -> double {
    static_cast<void>(bound);
    std::cerr << "TODO: cond lit " << type_ << " compute proper score or return something very small\n";
    return 0;
}

void LitCondLit::print(std::ostream &out) const { out << "cond lit " << type_; }

auto LitCondLit::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    static_cast<void>(store);
    static_cast<void>(ass);
    static_cast<void>(out);
    std::cerr << "TODO: cond lit " << type_ << " output something\n";
    return false;
}

auto LitCondLit::copy() const -> ULit { return std::make_unique<LitCondLit>(type_, *base_, index_); }

auto LitCondLit::hash() const -> size_t {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return Util::value_hash_record<LitCondLit>(type_, reinterpret_cast<uintptr_t>(base_));
}

auto LitCondLit::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitCondLit const *>(&other);
    return x != nullptr && std::tie(type_, base_) == std::tie(x->type_, x->base_);
}

auto LitCondLit::compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitCondLit const *>(&other); x != nullptr) {
        return std::tie(type_, base_) <=> std::tie(x->type_, x->base_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// StmCondLit

auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream & {
    switch (type) {
        case StmCondLitType::empty: {
            out << "empty";
            break;
        }
        case StmCondLitType::premise: {
            out << "premise";
            break;
        }
        case StmCondLitType::conclusion: {
            out << "conclusion";
            break;
        }
    }
    return out;
}

void StmCondLit::print(std::ostream &out) const { out << "TOOD: cond lit" << type_ << "."; }

auto StmCondLit::body() const -> ULitVec const & { return body_; }

auto StmCondLit::important() const -> VariableSet { return base_->vars(type_ != StmCondLitType::empty); }

void StmCondLit::init(size_t gen) { std::cerr << "init cond lit " << type_ << "with generation: " << gen << "\n"; }

void StmCondLit::report([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass) {
    std::cerr << "report cond lit " << type_ << "\n";
}

void StmCondLit::propagate([[maybe_unused]] Queue &queue) { std::cerr << "propagate cond lit " << type_ << "\n"; }

auto StmCondLit::priority() const -> size_t { return prio_; }

} // namespace Gringo::Ground
