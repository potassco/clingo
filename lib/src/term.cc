#include <term.hh>

////////// AST //////////

auto operator<<(std::ostream &out, ASTType type) -> std::ostream & {
    out << "TODO: type";
    return out;
}

auto operator<<(std::ostream &out, ASTAttr attr) -> std::ostream & {
    out << "TODO: attr";
    return out;
}

[[nodiscard]] auto AST::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}
auto operator<<(std::ostream &out, AST const &ast) -> std::ostream & {
    ast.print(out);
    return out;
}

[[nodiscard]] auto AST::get_int(ASTAttr attr) -> int & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

[[nodiscard]] auto AST::get_ast(ASTAttr attr) -> SAST & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

////////// Term //////////

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

void TermConstant::print(std::ostream &out) const { out << value; }

[[nodiscard]] auto TermConstant::type() const -> ASTType { return ASTType::TermConstant; }

////////// TermInteger //////////

[[nodiscard]] auto TermInteger::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::pos_number && value >= 0) {
        if (res != nullptr) {
            res->pos_number = value;
        }
        return true;
    }
    return false;
}

void TermInteger::print(std::ostream &out) const { out << value; }

[[nodiscard]] auto TermInteger::type() const -> ASTType { return ASTType::TermInteger; }

////////// TermTuple //////////

void TermTuple::print(std::ostream &out) const {
    if (args.size() == 1 && std::holds_alternative<UTerm>(args.front())) {
        std::get<UTerm>(args.front())->print(out);
    } else {
        out << "(";
        bool sem = false;
        for (const auto &tuple : args) {
            if (sem) {
                out << ";";
            } else {
                sem = true;
            }
            std::visit(
                [&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, UTerm>) {
                        arg->print(out);
                    } else if constexpr (std::is_same_v<T, UTermVec>) {
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
        }
        out << ")";
    }
}

[[nodiscard]] auto TermTuple::type() const -> ASTType { return ASTType::TermTuple; }

////////// TermString //////////

void TermString::print(std::ostream &out) const { print_quoted(out, value); }

[[nodiscard]] auto TermString::type() const -> ASTType { return ASTType::TermString; }

////////// TermVariable //////////

void TermVariable::print(std::ostream &out) const { out << name; }

[[nodiscard]] auto TermVariable::type() const -> ASTType { return ASTType::TermVariable; }

////////// TermAbs //////////

void TermAbs::print(std::ostream &out) const {
    out << "|";
    bool comma = false;
    for (const auto &term : pool) {
        if (comma) {
            out << ";";
        } else {
            comma = true;
        }
        term->print(out);
    }
    out << "|";
}

[[nodiscard]] auto TermAbs::type() const -> ASTType { return ASTType::TermAbs; }

////////// TermFunction //////////

void TermFunction::print(std::ostream &out) const {
    if (external) {
        out << "@";
    }
    out << name;
    if (args.size() != 1 || !args.front().empty()) {
        out << "(";
        bool sem = false;
        for (const auto &tuple : args) {
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
[[nodiscard]] auto TermFunction::type() const -> ASTType { return ASTType::TermFunction; }

[[nodiscard]] auto TermFunction::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::atom) {
        return !external;
    }
    if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) && !external &&
        args.size() == 1 && args.front().empty()) {
        if (res != nullptr) {
            res->identifier = name;
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

void TermUnary::print(std::ostream &out) const { out << "(" << op << *rhs << ")"; }
[[nodiscard]] auto TermUnary::type() const -> ASTType { return ASTType::TermUnary; }

[[nodiscard]] auto TermUnary::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::atom) {
        return op == UnaryOperator::negate && rhs->check_type(type);
    }
    if (type == TermCheckType::signed_identifier && op == UnaryOperator::negate &&
        rhs->check_type(TermCheckType::identifier, res)) {
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

[[nodiscard]] auto TermBinary::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::sig) {
        return op == BinaryOperator::div && lhs->check_type(TermCheckType::signed_identifier, res) &&
               rhs->check_type(TermCheckType::pos_number, res);
    }
    return false;
}

void TermBinary::print(std::ostream &out) const { out << "(" << *lhs << op << *rhs << ")"; }

[[nodiscard]] auto TermBinary::type() const -> ASTType { return ASTType::TermBinary; }
