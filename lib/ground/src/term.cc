#include <gringo/ground/term.hh>

namespace Gringo::Ground {

auto TermSymbol::match([[maybe_unused]] SymbolStore &store, Symbol sym, [[maybe_unused]] Assignment &ass) const
    -> bool {
    return sym == sym_;
}

auto TermSymbol::eval([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass) const
    -> std::optional<Symbol> {
    return sym_;
}

auto TermVariable::match([[maybe_unused]] SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    if (ass[var_]) {
        return ass[var_] == sym;
    }
    ass[var_] = sym;
    return true;
}

auto TermVariable::eval([[maybe_unused]] SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    return ass[var_];
}

auto TermLinear::match([[maybe_unused]] SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    if (sym.type() != SymbolType::number) {
        return false;
    }
    if (ass[var_]) {
        // m * x + n == s
        if (ass[var_]->type() != SymbolType::number) {
            return false;
        }
        return m_ * ass[var_]->num() + n_ == sym.num();
    }
    // x == (s - n) / m
    auto sn = *sym.num() - n_;
    if (sn % m_ == 0) {
        ass[var_] = store.num(std::move(sn) / m_);
        return true;
    }
    return false;
}

auto TermLinear::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    if (ass[var_] && ass[var_]->type() == SymbolType::number) {
        return store.num(m_ * ass[var_]->num() + n_);
    }
    return std::nullopt;
}

[[nodiscard]] auto TermUnary::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    if (op_ == UnaryOperator::minus) {
        if (sym.type() == SymbolType::function) {
            return rhs_->match(store, *sym.flip_classical_sign(), ass);
        }
        if (sym.type() == SymbolType::number) {
            return rhs_->match(store, store.num(-*sym.num()), ass);
        }
        return false;
    }
    return sym.type() == SymbolType::number && eval(store, ass) == sym;
}

auto TermUnary::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    if (auto rhs = rhs_->eval(store, ass); rhs) {
        switch (op_) {
            case UnaryOperator::minus: {
                if (rhs->type() == SymbolType::number) {
                    return store.num(-*rhs->num());
                }
                return rhs->flip_classical_sign();
            }
            case UnaryOperator::invert: {
                if (rhs->type() == SymbolType::number) {
                    return store.num(~*rhs->num());
                }
                break;
            }
            case UnaryOperator::abs: {
                if (rhs->type() == SymbolType::number) {
                    return store.num(abs(*rhs->num()));
                }
                break;
            }
        }
    }
    return std::nullopt;
}

[[nodiscard]] auto TermBinary::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    return sym.type() == SymbolType::number && eval(store, ass) == sym;
}

[[nodiscard]] auto TermBinary::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    if (auto lhs = lhs_->eval(store, ass); lhs && lhs->type() == SymbolType::number) {
        if (auto rhs = rhs_->eval(store, ass); rhs && rhs->type() == SymbolType::number) {
            switch (op_) {
                case BinaryOperator::and_: {
                    return store.num(*lhs->num() & *rhs->num());
                }
                case BinaryOperator::div: {
                    if (*rhs->num() != 0) {
                        return store.num(*lhs->num() / *rhs->num());
                    }
                    break;
                }
                case BinaryOperator::minus: {
                    return store.num(*lhs->num() - *rhs->num());
                }
                case BinaryOperator::mod: {
                    if (*rhs->num() != 0) {
                        return store.num(*lhs->num() % *rhs->num());
                    }
                    break;
                }
                case BinaryOperator::or_: {
                    return store.num(*lhs->num() | *rhs->num());
                }
                case BinaryOperator::plus: {
                    return store.num(*lhs->num() + *rhs->num());
                }
                case BinaryOperator::pow: {
                    if (*rhs->num() >= 0) {
                        return store.num(pow(*lhs->num(), *rhs->num()));
                    }
                    break;
                }
                case BinaryOperator::times: {
                    return store.num(*lhs->num() * *rhs->num());
                }
                case BinaryOperator::xor_: {
                    return store.num(*lhs->num() ^ *rhs->num());
                }
            }
        }
    }
    return std::nullopt;
}

[[nodiscard]] auto TermTuple::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    // TODO: match the arguments
    static_cast<void>(store);
    static_cast<void>(sym);
    static_cast<void>(ass);
    throw std::logic_error("implement me!!!");
}

[[nodiscard]] auto TermTuple::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    static_cast<void>(store);
    static_cast<void>(ass);
    throw std::logic_error("implement me!!!");
}

[[nodiscard]] auto TermFunction::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    // TODO: match the arguments
    static_cast<void>(store);
    static_cast<void>(sym);
    static_cast<void>(ass);
    throw std::logic_error("implement me!!!");
}

[[nodiscard]] auto TermFunction::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    static_cast<void>(store);
    static_cast<void>(ass);
    throw std::logic_error("implement me!!!");
}

} // namespace Gringo::Ground
