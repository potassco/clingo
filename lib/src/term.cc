#include <cmath>
#include <optional>
#include <sstream>
#include <tuple>
#include <utility>

#include <util/print.hh>

#include <term.hh>

#include "unpool.hh"

////////// Term //////////

auto operator<<(std::ostream &out, TermType type) -> std::ostream & {
    out << "TODO: type";
    return out;
}

auto operator<<(std::ostream &out, Attribute attr) -> std::ostream & {
    out << "TODO: attr";
    return out;
}

[[nodiscard]] auto Term::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Term const &ast) -> std::ostream & {
    ast.print(out);
    return out;
}

auto Term::unpool() -> STermVec {
    STermVec terms;
    PoolTerm pool{terms};
    unpool(pool);
    return terms;
}

[[nodiscard]] auto Term::get_int(Attribute attr) -> int & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

[[nodiscard]] auto Term::get_ast(Attribute attr) -> STerm & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

[[nodiscard]] auto Term::get_ast_vec(Attribute attr) -> STermVec & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

[[nodiscard]] auto Term::get_ast_vec_vec(Attribute attr) -> STermVecVec & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

[[nodiscard]] auto Term::check_type(TermCheckType type, CheckTypeResult *res) const -> bool { return false; }

////////// TermSymbol //////////

void TermSymbol::print(std::ostream &out) const { out << value_; }

[[nodiscard]] auto TermSymbol::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    return visit_variant(
        value_,
        [&](int value) {
            if (type == TermCheckType::pos_number && value >= 0) {
                if (res != nullptr) {
                    res->pos_number = value;
                }
                return true;
            }
            return false;
        },
        [&](Function const &value) {
            if (type == TermCheckType::atom) {
                return true;
            }
            if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) &&
                !value.name.empty() && value.args.empty()) {
                if (res != nullptr) {
                    res->identifier = value.name;
                }
                return true;
            }
            return false;
        },
        [&](auto &&value) { return false; });
}

void TermSymbol::unpool(PoolTerm &pool) { pool.append(this); }

auto TermSymbol::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermSymbol const *>(&other);
    return d != nullptr && value_equal(value_, d->value_);
}

auto TermSymbol::hash() const -> size_t { return value_hash(typeid(TermSymbol), value_); }

void TermSymbol::variables(VariableSet &vars, VariableSelectMode mode) const {
    static_cast<void>(vars);
    static_cast<void>(mode);
}

[[nodiscard]] auto TermSymbol::type() const -> TermType { return TermType::TermSymbol; }

[[nodiscard]] auto TermSymbol::get_int(Attribute attr) -> int & {
    switch (attr) {
        case Attribute::Value: {
            return reinterpret_cast<int &>(value_);
        }
        default: {
            return Term::get_int(attr);
        }
    }
}

////////// TermTuple //////////

void TermTuple::print(std::ostream &out) const {
    if (pool_.size() == 1 && std::holds_alternative<STerm>(pool_.front())) {
        std::get<STerm>(pool_.front())->print(out);
    } else {
        out << "(" << p_range_with(pool_, ";", [](std::ostream &out, auto const &term_or_tuple) {
            visit_variant(
                term_or_tuple, [&](STerm const &term) { term->print(out); },
                [&](STermVec const &tuple) {
                    out << p_range(tuple);
                    if (tuple.size() == 1) {
                        out << ",";
                    }
                });
        }) << ")";
    }
}

[[nodiscard]] auto TermTuple::type() const -> TermType { return TermType::TermTuple; }

auto TermTuple::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermTuple const *>(&other);
    return d != nullptr && value_equal(pool_, d->pool_);
}

auto TermTuple::hash() const -> size_t { return value_hash(typeid(TermTuple), pool_); }

void TermTuple::variables(VariableSet &vars, VariableSelectMode mode) const {
    for (auto const &tuple_or_term : pool_) {
        visit_variant(
            tuple_or_term, [&](STerm const &term) { term->variables(vars, mode); },
            [&](STermVec const &tuple) {
                for (auto const &term : tuple) {
                    term->variables(vars, mode);
                }
            });
    }
}

void TermTuple::unpool(PoolTerm &pool) {
    for (auto &tuple_or_term : pool_) {
        visit_variant(
            tuple_or_term, [&](STerm &term) { term->unpool(pool); },
            [&](STermVec &tuple) {
                unpool_with(
                    [&](std::optional<STermVec> &opt_tuple) {
                        if (!opt_tuple.has_value() && pool_.size() == 1) {
                            pool.append(this);
                        } else {
                            pool.append_shared<TermTuple>(ElementVec{std::move(opt_tuple).value_or(tuple)});
                        }
                    },
                    unpool_crossproduct(pool, tuple));
            });
    }
}

////////// TermVariable //////////

void TermVariable::print(std::ostream &out) const { out << name_; }

auto TermVariable::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermVariable const *>(&other);
    return d != nullptr && name_ == d->name_;
}

auto TermVariable::hash() const -> size_t { return value_hash(typeid(TermVariable), name_); }

void TermVariable::unpool(PoolTerm &pool) { pool.append(this); }

[[nodiscard]] auto TermVariable::type() const -> TermType { return TermType::TermVariable; }

void TermVariable::variables(VariableSet &vars, VariableSelectMode mode) const {
    if (mode == VariableSelectMode::add) {
        vars.emplace(name_);
    } else {
        vars.erase(name_);
    }
}

////////// TermAbs //////////

void TermAbs::print(std::ostream &out) const { out << "|" << p_range(pool_, ";") << "|"; }

auto TermAbs::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermAbs const *>(&other);
    return d != nullptr && pool_ == d->pool_;
}

auto TermAbs::hash() const -> size_t { return value_hash(typeid(TermAbs), pool_); }

void TermAbs::unpool(PoolTerm &pool) {
    unpool_with(
        [&](std::optional<STerm> &arg) {
            if (!arg.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<TermAbs>(STermVec{std::move(arg).value()});
            }
        },
        unpool_union(pool, pool_));
}

[[nodiscard]] auto TermAbs::type() const -> TermType { return TermType::TermAbs; }

void TermAbs::variables(VariableSet &vars, VariableSelectMode mode) const {
    for (auto const &term : pool_) {
        term->variables(vars, mode);
    }
}

////////// TermFunction //////////

void TermFunction::print(std::ostream &out) const {
    if (external_) {
        out << "@";
    }
    out << name_;
    if (pool_.size() != 1 || !pool_.front().empty()) {
        out << "(" << p_range_with(pool_, ";", [](std::ostream &out, STermVec const &tuple) { out << p_range(tuple); })
            << ")";
    }
}

auto TermFunction::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermFunction const *>(&other);
    return d != nullptr && value_equal(external_, d->external_, name_, d->name_, pool_, d->pool_);
}

auto TermFunction::hash() const -> size_t { return value_hash(typeid(TermFunction), external_, name_, pool_); }

void TermFunction::unpool(PoolTerm &pool) {
    for (auto &tuple : pool_) {
        unpool_with(
            [&](std::optional<STermVec> &unpooled) {
                if (!unpooled.has_value() && pool_.size() == 1) {
                    pool.append(this);
                } else {
                    pool.append_shared<TermFunction>(name_, STermVecVec{std::move(unpooled).value_or(tuple)},
                                                     external_);
                }
            },
            unpool_crossproduct(pool, tuple));
    }
}

void TermFunction::variables(VariableSet &vars, VariableSelectMode mode) const {
    for (auto const &tuple : pool_) {
        for (auto const &term : tuple) {
            term->variables(vars, mode);
        }
    }
}

[[nodiscard]] auto TermFunction::type() const -> TermType { return TermType::TermFunction; }

[[nodiscard]] auto TermFunction::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::atom) {
        return !external_;
    }
    if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) && !external_ &&
        pool_.size() == 1 && pool_.front().empty()) {
        if (res != nullptr) {
            res->identifier = name_;
        }
        return true;
    }
    return false;
}

////////// TermUnary //////////

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream & {
    out << (op == UnaryOperator::negate ? "-" : "~");
    return out;
}

void TermUnary::print(std::ostream &out) const { out << "(" << op_ << *rhs_ << ")"; }

auto TermUnary::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermUnary const *>(&other);
    return d != nullptr && value_equal(op_, d->op_, rhs_, d->rhs_);
}

auto TermUnary::hash() const -> size_t { return value_hash(typeid(TermUnary), op_, rhs_); }

void TermUnary::unpool(PoolTerm &pool) {
    unpool_with(
        [&](std::optional<STerm> &rhs) {
            if (!rhs.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<TermUnary>(op_, std::move(rhs).value());
            }
        },
        unpool_element(pool, rhs_));
}

[[nodiscard]] auto TermUnary::type() const -> TermType { return TermType::TermUnary; }

[[nodiscard]] auto TermUnary::get_int(Attribute attr) -> int & {
    switch (attr) {
        case Attribute::Operator: {
            return reinterpret_cast<int &>(op_);
        }
        default: {
            return Term::get_int(attr);
        }
    }
}

[[nodiscard]] auto TermUnary::get_ast(Attribute attr) -> STerm & {
    switch (attr) {
        case Attribute::Right: {
            return rhs_;
        }
        default: {
            return Term::get_ast(attr);
        }
    }
}

[[nodiscard]] auto TermUnary::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::atom) {
        return op_ == UnaryOperator::negate && rhs_->check_type(type);
    }
    if (type == TermCheckType::signed_identifier && op_ == UnaryOperator::negate &&
        rhs_->check_type(TermCheckType::identifier, res)) {
        if (res != nullptr) {
            res->has_sign = true;
        }
        return true;
    }
    return false;
}

void TermUnary::variables(VariableSet &vars, VariableSelectMode mode) const { rhs_->variables(vars, mode); }

////////// TermBinary //////////

auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream & {
    switch (op) {
        case BinaryOperator::dots: {
            out << "^";
            break;
        }
        case BinaryOperator::xor_: {
            out << "^";
            break;
        }
        case BinaryOperator::or_: {
            out << "?";
            break;
        }
        case BinaryOperator::and_: {
            out << "&";
            break;
        }
        case BinaryOperator::plus: {
            out << "+";
            break;
        }
        case BinaryOperator::minus: {
            out << "-";
            break;
        }
        case BinaryOperator::times: {
            out << "*";
            break;
        }
        case BinaryOperator::div: {
            out << "/";
            break;
        }
        case BinaryOperator::mod: {
            out << "\\";
            break;
        }
        case BinaryOperator::pow: {
            out << "**";
            break;
        }
    }
    return out;
}

void TermBinary::print(std::ostream &out) const { out << "(" << *lhs_ << op_ << *rhs_ << ")"; }

auto TermBinary::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermBinary const *>(&other);
    return d != nullptr && value_equal(op_, d->op_, lhs_, d->lhs_, rhs_, d->rhs_);
}

auto TermBinary::hash() const -> size_t { return value_hash(typeid(TermBinary), op_, lhs_, rhs_); }

void TermBinary::unpool(PoolTerm &pool) {
    unpool_with(
        [&](std::optional<STerm> &lhs, std::optional<STerm> &rhs) {
            if (!lhs.has_value() && !rhs.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<TermBinary>(lhs.value_or(lhs_), op_, std::move(rhs).value_or(rhs_));
            }
        },
        unpool_element(pool, lhs_), unpool_element(pool, rhs_));
}

[[nodiscard]] auto TermBinary::type() const -> TermType { return TermType::TermBinary; }

[[nodiscard]] auto TermBinary::get_int(Attribute attr) -> int & {
    switch (attr) {
        case Attribute::Operator: {
            return reinterpret_cast<int &>(op_);
        }
        default: {
            return Term::get_int(attr);
        }
    }
}

[[nodiscard]] auto TermBinary::get_ast(Attribute attr) -> STerm & {
    switch (attr) {
        case Attribute::Left: {
            return lhs_;
        }
        case Attribute::Right: {
            return rhs_;
        }
        default: {
            return Term::get_ast(attr);
        }
    }
}

[[nodiscard]] auto TermBinary::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::sig) {
        return op_ == BinaryOperator::div && lhs_->check_type(TermCheckType::signed_identifier, res) &&
               rhs_->check_type(TermCheckType::pos_number, res);
    }
    return false;
}

void TermBinary::variables(VariableSet &vars, VariableSelectMode mode) const {
    lhs_->variables(vars, mode);
    rhs_->variables(vars, mode);
}
