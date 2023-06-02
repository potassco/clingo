#include <optional>
#include <sstream>
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
    return std::visit(
        [&](auto &&value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int>) {
                if (type == TermCheckType::pos_number && value >= 0) {
                    if (res != nullptr) {
                        res->pos_number = value;
                    }
                    return true;
                }
            }
            if constexpr (std::is_same_v<T, Function>) {
                if (type == TermCheckType::atom) {
                    return true;
                }
                if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) &&
                    !value.name.empty() && value.args.size() == 0) {
                    if (res != nullptr) {
                        res->identifier = value.name;
                    }
                    return true;
                }
            }
            return false;
        },
        value_);
}

void TermSymbol::unpool(STermVec &pool) { pool.emplace_back(this); }

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
    if (args_.size() == 1 && std::holds_alternative<STerm>(args_.front())) {
        std::get<STerm>(args_.front())->print(out);
    } else {
        out << "(" << p_range_with(args_, ";", [](std::ostream &out, auto const &tuple) {
            std::visit(
                [&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, STerm>) {
                        arg->print(out);
                    } else if constexpr (std::is_same_v<T, STermVec>) {
                        bool comma = false;
                        for (const auto &term : arg) {
                            if (comma) {
                                out << ",";
                            } else {
                                comma = true;
                            }
                            term->print(out);
                        }
                        if (arg.size() == 1) {
                            out << ",";
                        }
                    }
                },
                tuple);
        }) << ")";
    }
}

[[nodiscard]] auto TermTuple::type() const -> TermType { return TermType::TermTuple; }

void TermTuple::unpool(STermVec &pool) {
    for (auto &tuple : args_) {
        std::visit(
            [&](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, STerm>) {
                    arg->unpool(pool);
                } else if constexpr (std::is_same_v<T, STermVec>) {
                    // Note: think about how to handle the unchanged case
                    std::optional<STerm> unchanged;
                    if (args_.size() == 1) {
                        unchanged = STerm(this);
                    }
                    detail::unpool_vec_with(unchanged, arg, pool, [&pool](STermVec terms) {
                        ElementVec elems;
                        elems.emplace_back(std::move(terms));
                        pool.emplace_back(construct_shared<TermTuple, Term>(std::move(elems)));
                    });
                }
            },
            tuple);
    }
}

////////// TermVariable //////////

void TermVariable::print(std::ostream &out) const { out << name_; }

void TermVariable::unpool(STermVec &pool) { pool.emplace_back(this); }

[[nodiscard]] auto TermVariable::type() const -> TermType { return TermType::TermVariable; }

////////// TermAbs //////////

void TermAbs::print(std::ostream &out) const {
    out << "|";
    bool comma = false;
    for (const auto &term : pool_) {
        if (comma) {
            out << ";";
        } else {
            comma = true;
        }
        term->print(out);
    }
    out << "|";
}

void TermAbs::unpool(STermVec &pool) {
    // Note: a generic version is possible
    size_t offset = pool.size();
    for (const auto &term : pool_) {
        term->unpool(pool);
    }
    if (pool_.size() == 1 && pool.size() - offset == 1 && pool.back() == pool_.back()) {
        pool.emplace_back(this);
    } else {
        for (auto it = pool.begin() + offset, ie = pool.end(); it != ie; ++it) {
            *it = construct_shared<TermAbs, Term>(STermVec{std::move(*it)});
        }
    }
}

[[nodiscard]] auto TermAbs::type() const -> TermType { return TermType::TermAbs; }

////////// TermFunction //////////

void TermFunction::print(std::ostream &out) const {
    if (external_) {
        out << "@";
    }
    out << name_;
    if (args_.size() != 1 || !args_.front().empty()) {
        out << "(";
        bool sem = false;
        for (const auto &tuple : args_) {
            if (sem) {
                out << ";";
            } else {
                sem = true;
            }
            bool comma = false;
            for (const auto &term : tuple) {
                if (comma) {
                    out << ",";
                } else {
                    comma = true;
                }
                term->print(out);
            }
        }
        out << ")";
    }
}

void TermFunction::unpool(STermVec &pool) {
    for (auto &tuple : args_) {
        // Note: think about unchanged case
        std::optional<STerm> unchanged;
        if (args_.size() == 1) {
            unchanged = STerm(this);
        }
        detail::unpool_vec_with(unchanged, tuple, pool, [&](STermVec terms) {
            pool.emplace_back(construct_shared<TermFunction, Term>(name_, STermVecVec{std::move(terms)}, external_));
        });
    }
}

[[nodiscard]] auto TermFunction::type() const -> TermType { return TermType::TermFunction; }

[[nodiscard]] auto TermFunction::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::atom) {
        return !external_;
    }
    if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) && !external_ &&
        args_.size() == 1 && args_.front().empty()) {
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

void TermUnary::unpool(STermVec &pool) {
    // Note: a generic version is possible
    size_t offset = pool.size();
    rhs_->unpool(pool);
    if (pool.size() - offset == 1 && pool.back() == rhs_) {
        pool.emplace_back(this);
    } else {
        for (auto it = pool.begin() + offset, ie = pool.end(); it != ie; ++it) {
            *it = construct_shared<TermUnary, Term>(op_, std::move(*it));
        }
    }
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

void TermBinary::unpool(STermVec &pool) {
    // Note: a generic version is possible
    auto begin_lhs = pool.size();
    lhs_->unpool(pool);
    auto begin_rhs = pool.size();
    rhs_->unpool(pool);
    if (pool.size() - begin_rhs == 2 && pool.back() == rhs_ && *(pool.end() - 2) == lhs_) {
        pool.emplace_back(this);
    } else {
        size_t end = pool.size();
        for (auto i = begin_lhs; i < begin_rhs; ++i) {
            for (auto j = begin_rhs; j < end; ++j) {
                pool.emplace_back(construct_shared<TermBinary, Term>(pool[i], op_, pool[j]));
            }
        }
        pool.erase(pool.begin() + begin_lhs, pool.begin() + end);
    }
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
