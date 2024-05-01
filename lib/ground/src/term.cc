#include <gringo/ground/term.hh>

#include <gringo/util/optional.hh>

#include <typeindex>

namespace Gringo::Ground {

namespace {

auto eval_args(SymbolStore &store, Assignment const &ass, UTermVec const &args) -> std::optional<SymbolVec> {
    SymbolVec res;
    res.reserve(args.size());
    for (auto const &arg : args) {
        if (auto sym = arg->eval(store, ass); sym) {
            res.emplace_back(*sym);
        } else {
            return std::nullopt;
        }
    }
    return res;
}

auto match_args(SymbolStore &store, Assignment &ass, UTermVec const &term_args, SymbolSpan sym_args) -> bool {
    if (term_args.size() != sym_args.size()) {
        return false;
    }
    auto it = sym_args.begin();
    for (auto const &arg : term_args) {
        if (!arg->match(store, *it++, ass)) {
            return false;
        }
    }
    return true;
}

auto rename_args(UTermVec const &args, SymbolStore &store, RenameMode mode, size_t *vars) -> UTermVec {
    auto res = UTermVec{};
    res.reserve(args.size());
    for (auto const &arg : args) {
        if (auto rep = arg->rename(store, mode, nullptr, vars); rep != nullptr) {
            res.emplace_back(std::move(rep));
        }
    }
    res.shrink_to_fit();
    return res;
}

} // namespace

// TermProjection

auto TermProjection::score([[maybe_unused]] double size,
                           [[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return 0;
}

auto TermProjection::match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Symbol sym,
                           [[maybe_unused]] Assignment &ass) const -> bool {
    return true;
}

auto TermProjection::eval(SymbolStore &store, [[maybe_unused]] Assignment const &ass) const -> std::optional<Symbol> {
    // Note: this is a sentinel symbol intended for text output
    return store.fun(store.string("*"), {}, false);
}

auto TermProjection::rename([[maybe_unused]] SymbolStore &store, RenameMode mode, [[maybe_unused]] String const *name,
                            size_t *vars) const -> UTerm {
    assert(name == nullptr);
    if (mode == RenameMode::drop_projection) {
        return nullptr;
    }
    if (mode == RenameMode::rename_projection && vars != nullptr) {
        return std::make_unique<TermVariable>((*vars)++);
    }
    return std::make_unique<TermProjection>();
}

auto TermProjection::rename([[maybe_unused]] Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermProjection>();
}

void TermProjection::vars([[maybe_unused]] VariableSet &vars, [[maybe_unused]] bool provide) const {}

void TermProjection::print(std::ostream &out) const { out << "*"; }

auto TermProjection::hash() const -> size_t { return Util::value_hash_record<TermProjection>(); }

auto TermProjection::equal_to([[maybe_unused]] Term const &other) const -> bool {
    return dynamic_cast<TermProjection const *>(&other) != nullptr;
}

auto TermProjection::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermProjection const *>(&other); x != nullptr) {
        return 0 <=> 0;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermSymbol

auto TermSymbol::match([[maybe_unused]] SymbolStore &store, Symbol sym,
                       [[maybe_unused]] Assignment &ass) const -> bool {
    return sym == sym_;
}

auto TermSymbol::score([[maybe_unused]] double size, [[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return 0;
}

auto TermSymbol::eval([[maybe_unused]] SymbolStore &store,
                      [[maybe_unused]] Assignment const &ass) const -> std::optional<Symbol> {
    return sym_;
}

auto TermSymbol::rename([[maybe_unused]] SymbolStore &store, [[maybe_unused]] RenameMode mode, String const *name,
                        [[maybe_unused]] size_t *vars) const -> UTerm {
    if (name != nullptr && sym_.type() == SymbolType::function) {
        return std::make_unique<TermSymbol>(store.fun(*name, sym_.args(), sym_.has_classical_sign()));
    }
    if (mode == RenameMode::rename_vars && vars != nullptr) {
        return std::make_unique<TermVariable>((*vars)++);
    }
    return std::make_unique<TermSymbol>(sym_);
}

auto TermSymbol::rename([[maybe_unused]] Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermSymbol>(sym_);
}

void TermSymbol::vars([[maybe_unused]] VariableSet &vars, [[maybe_unused]] bool provide) const {}

void TermSymbol::print(std::ostream &out) const { out << sym_; }

auto TermSymbol::hash() const -> size_t { return Util::value_hash_record<TermSymbol>(sym_); }

auto TermSymbol::equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermSymbol const *>(&other);
    return x != nullptr && sym_ == x->sym_;
}

auto TermSymbol::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermSymbol const *>(&other); x != nullptr) {
        return sym_ <=> x->sym_;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermVariable

auto TermVariable::score(double size, std::vector<bool> const &bound) const -> double {
    return bound[var_] ? 0.0 : size;
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

auto TermVariable::rename([[maybe_unused]] SymbolStore &store, RenameMode mode, [[maybe_unused]] String const *name,
                          size_t *vars) const -> UTerm {
    assert(name == nullptr);
    return std::make_unique<TermVariable>(mode == RenameMode::rename_vars && vars != nullptr ? (*vars)++ : var_);
}

auto TermVariable::rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermVariable>(vars.try_emplace(var_, vars.size()).first.value());
}

void TermVariable::vars(VariableSet &vars, [[maybe_unused]] bool provide) const { vars.emplace(var_); }

void TermVariable::print(std::ostream &out) const { out << "X_" << var_; }

auto TermVariable::hash() const -> size_t { return Util::value_hash_record<TermSymbol>(var_); }

auto TermVariable::equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermVariable const *>(&other);
    return x != nullptr && var_ == x->var_;
}

auto TermVariable::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermVariable const *>(&other); x != nullptr) {
        return var_ <=> x->var_;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermLinear

auto TermLinear::score(double size, std::vector<bool> const &bound) const -> double { return bound[var_] ? 0.0 : size; }

auto TermLinear::match([[maybe_unused]] SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    if (sym.type() != SymbolType::number) {
        return false;
    }
    if (auto var = ass[var_]; var) {
        // m * x + n == s
        if (var->type() != SymbolType::number) {
            return false;
        }
        return m_ * var->num() + n_ == sym.num();
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
    if (auto var = ass[var_]; var && var->type() == SymbolType::number) {
        return store.num(m_ * var->num() + n_);
    }
    return std::nullopt;
}

auto TermLinear::rename([[maybe_unused]] SymbolStore &store, RenameMode mode, [[maybe_unused]] String const *name,
                        size_t *vars) const -> UTerm {
    assert(name == nullptr);
    return std::make_unique<TermLinear>(m_, mode == RenameMode::rename_vars && vars != nullptr ? (*vars)++ : var_, n_);
}

auto TermLinear::rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermLinear>(m_, vars.try_emplace(var_, vars.size()).first.value(), n_);
}

void TermLinear::vars(VariableSet &vars, [[maybe_unused]] bool provide) const { vars.emplace(var_); }

void TermLinear::print(std::ostream &out) const { out << "(" << m_ << "*X_" << var_ << "+" << n_ << ")"; }

auto TermLinear::hash() const -> size_t { return Util::value_hash_record<TermSymbol>(var_, m_, n_); }

auto TermLinear::equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermLinear const *>(&other);
    return x != nullptr && std::tie(var_, m_, n_) == std::tie(x->var_, x->m_, x->n_);
}

auto TermLinear::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermLinear const *>(&other); x != nullptr) {
        return std::tie(var_, m_, n_) <=> std::tie(x->var_, x->m_, x->n_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermUnary

auto TermUnary::score(double size, std::vector<bool> const &bound) const -> double {
    return op_ == UnaryOperator::minus ? rhs_->score(size, bound) : 0.0;
}

auto TermUnary::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    if (op_ == UnaryOperator::minus) {
        if (sym.type() == SymbolType::function) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
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

auto TermUnary::rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const -> UTerm {
    assert(name == nullptr);
    return std::make_unique<TermUnary>(op_, rhs_->rename(store, mode, name, vars));
}

auto TermUnary::rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermUnary>(op_, rhs_->rename(vars));
}

void TermUnary::vars(VariableSet &vars, bool provide) const {
    if (op_ == UnaryOperator::minus || !provide) {
        rhs_->vars(vars, provide);
    }
}

void TermUnary::print(std::ostream &out) const {
    out << "(";
    switch (op_) {
        case UnaryOperator::abs: {
            out << "|";
            break;
        }
        case UnaryOperator::invert: {
            out << "~";
            break;
        }
        case UnaryOperator::minus: {
            out << "-";
            break;
        }
    }
    rhs_->print(out);
    if (op_ == UnaryOperator::abs) {
        out << "|";
    }
    out << ")";
}

auto TermUnary::hash() const -> size_t { return Util::value_hash_record<TermUnary>(op_, *rhs_); }

auto TermUnary::equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermUnary const *>(&other);
    return x != nullptr && std::tie(op_, *rhs_) == std::tie(x->op_, *x->rhs_);
}

auto TermUnary::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermUnary const *>(&other); x != nullptr) {
        return std::tie(op_, *rhs_) <=> std::tie(x->op_, *x->rhs_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermBinary

auto TermBinary::score([[maybe_unused]] double size, [[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return 0;
}

auto TermBinary::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    return sym.type() == SymbolType::number && eval(store, ass) == sym;
}

auto TermBinary::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
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

auto TermBinary::rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const -> UTerm {
    assert(name == nullptr);
    auto lhs = lhs_->rename(store, mode, name, vars);
    return std::make_unique<TermBinary>(std::move(lhs), op_, rhs_->rename(store, mode, name, vars));
}

auto TermBinary::rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    auto lhs = lhs_->rename(vars);
    auto rhs = rhs_->rename(vars);
    return std::make_unique<TermBinary>(std::move(lhs), op_, std::move(rhs));
}

void TermBinary::vars(VariableSet &vars, bool provide) const {
    if (!provide) {
        lhs_->vars(vars, provide);
        rhs_->vars(vars, provide);
    }
}

void TermBinary::print(std::ostream &out) const {
    out << "(";
    lhs_->print(out);
    switch (op_) {
        case BinaryOperator::and_: {
            out << "&";
            break;
        }
        case BinaryOperator::div: {
            out << "/";
            break;
        }
        case BinaryOperator::minus: {
            out << "-";
            break;
        }
        case BinaryOperator::mod: {
            out << "%";
            break;
        }
        case BinaryOperator::or_: {
            out << "|";
            break;
        }
        case BinaryOperator::plus: {
            out << "+";
            break;
        }
        case BinaryOperator::pow: {
            out << "^";
            break;
        }
        case BinaryOperator::times: {
            out << "*";
            break;
        }
        case BinaryOperator::xor_: {
            out << "^";
            break;
        }
    }
    rhs_->print(out);
    out << ")";
}

auto TermBinary::hash() const -> size_t { return Util::value_hash_record<TermBinary>(op_, *rhs_); }

auto TermBinary::equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermBinary const *>(&other);
    return x != nullptr && std::tie(op_, *rhs_) == std::tie(x->op_, *x->rhs_);
}

auto TermBinary::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermBinary const *>(&other); x != nullptr) {
        return std::tie(op_, *rhs_) <=> std::tie(x->op_, *x->rhs_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermTuple

auto TermTuple::score(double size, std::vector<bool> const &bound) const -> double {
    double ret = 0.0;
    if (!args_.empty()) {
        auto len = static_cast<double>(args_.size());
        // NOLINTNEXTLINE(readability-magic-numbers)
        double root = std::max(1.0, std::pow(size, 1.0 / len));
        for (const auto &x : args_) {
            ret += x->score(root, bound);
        }
        ret /= len;
    }
    return ret;
}

auto TermTuple::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    return sym.type() == SymbolType::tuple && match_args(store, ass, args_, sym.args());
}

auto TermTuple::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    return Util::transform(eval_args(store, ass, args_),
                           [&store](SymbolVec args) { return store.tup(std::move(args)); });
}

auto TermTuple::rename(SymbolStore &store, RenameMode mode, [[maybe_unused]] String const *name,
                       size_t *vars) const -> UTerm {
    assert(name == nullptr);
    return std::make_unique<TermTuple>(rename_args(args_, store, mode, vars));
}

auto TermTuple::rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    auto args = UTermVec{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->rename(vars));
    }
    return std::make_unique<TermTuple>(std::move(args));
}

void TermTuple::vars(VariableSet &vars, bool provide) const {
    for (auto const &arg : args_) {
        arg->vars(vars, provide);
    }
}

void TermTuple::print(std::ostream &out) const {
    out << "(";
    auto n = args_.size();
    if (args_.size() == 1) {
        ++n;
    }
    for (auto const &arg : args_) {
        arg->print(out);
        if (--n; n > 0) {
            out << ",";
        }
    }
    out << ")";
}

auto TermTuple::hash() const -> size_t { return Util::value_hash_record<TermTuple>(args_); }

auto TermTuple::equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermTuple const *>(&other);
    return x != nullptr && std::equal(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                                      [](auto const &a, auto const &b) { return *a == *b; });
}

auto TermTuple::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermTuple const *>(&other); x != nullptr) {
        return std::lexicographical_compare_three_way(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                                                      [](auto const &a, auto const &b) { return *a <=> *b; });
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermFunction

auto TermFunction::score(double size, std::vector<bool> const &bound) const -> double {
    double ret = 0.0;
    if (!args_.empty()) {
        auto len = static_cast<double>(args_.size());
        // NOLINTNEXTLINE(readability-magic-numbers)
        double root = std::max(1.0, std::pow(size / 2.0, 1.0 / len));
        for (const auto &x : args_) {
            ret += x->score(root, bound);
        }
        ret /= len;
    }
    return ret;
}

auto TermFunction::match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
    return sym.type() == SymbolType::function && !sym.has_classical_sign() && sym.name() == name_ &&
           match_args(store, ass, args_, sym.args());
}

auto TermFunction::eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
    return Util::transform(eval_args(store, ass, args_),
                           [&store, this](SymbolVec args) { return store.fun(name_, std::move(args), false); });
}

auto TermFunction::rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const -> UTerm {
    return std::make_unique<TermFunction>(name != nullptr ? *name : name_, rename_args(args_, store, mode, vars));
}

auto TermFunction::rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    auto args = UTermVec{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->rename(vars));
    }
    return std::make_unique<TermFunction>(name_, std::move(args));
}

void TermFunction::vars(VariableSet &vars, bool provide) const {
    for (auto const &arg : args_) {
        arg->vars(vars, provide);
    }
}

void TermFunction::print(std::ostream &out) const {
    out << name_;
    if (auto n = args_.size(); n >= 1) {
        out << "(";
        for (auto const &arg : args_) {
            arg->print(out);
            if (--n; n > 0) {
                out << ",";
            }
        }
        out << ")";
    }
}

auto TermFunction::hash() const -> size_t { return Util::value_hash_record<TermFunction>(name_, args_); }

auto TermFunction::equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermFunction const *>(&other);
    return x != nullptr && name_ == x->name_ &&
           std::equal(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                      [](auto const &a, auto const &b) { return *a == *b; });
}

auto TermFunction::compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermFunction const *>(&other); x != nullptr) {
        if (auto n = name_ <=> x->name_; std::is_neq(n)) {
            return n;
        }
        return std::lexicographical_compare_three_way(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                                                      [](auto const &a, auto const &b) { return *a <=> *b; });
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

} // namespace Gringo::Ground
