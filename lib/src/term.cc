#include <sstream>

#include <util/print.hh>

#include <term.hh>

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

////////// TermConstant //////////

auto operator<<(std::ostream &out, Constant op) -> std::ostream & {
    switch (op) {
        case Constant::supremum: {
            out << "#sup";
            break;
        }
        case Constant::infimum: {
            out << "#inf";
            break;
        }
    }
    return out;
}

void TermConstant::print(std::ostream &out) const { out << value_; }

[[nodiscard]] auto TermConstant::type() const -> TermType { return TermType::TermConstant; }

[[nodiscard]] auto TermConstant::get_int(Attribute attr) -> int & {
    switch (attr) {
        case Attribute::Value: {
            return reinterpret_cast<int &>(value_);
        }
        default: {
            return Term::get_int(attr);
        }
    }
}

////////// TermInteger //////////

[[nodiscard]] auto TermInteger::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::pos_number && value_ >= 0) {
        if (res != nullptr) {
            res->pos_number = value_;
        }
        return true;
    }
    return false;
}

void TermInteger::print(std::ostream &out) const { out << value_; }

[[nodiscard]] auto TermInteger::type() const -> TermType { return TermType::TermInteger; }

[[nodiscard]] auto TermInteger::get_int(Attribute attr) -> int & {
    switch (attr) {
        case Attribute::Value: {
            return value_;
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

////////// TermString //////////

void TermString::print(std::ostream &out) const { print_quoted(out, value_); }

[[nodiscard]] auto TermString::type() const -> TermType { return TermType::TermString; }

////////// TermVariable //////////

void TermVariable::print(std::ostream &out) const { out << name_; }

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
