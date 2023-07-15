#include <input/algo/check_type.hh>

namespace Gringo::Input {

namespace {

struct CheckType {
    auto operator()(TermSymbol const &term) const -> bool {
        return Util::visit_variant(
            term.value,
            [this](int value) {
                if (type == TermCheckType::pos_number && value >= 0) {
                    if (res != nullptr) {
                        res->pos_number = value;
                    }
                    return true;
                }
                return false;
            },
            [this](Function const &value) {
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
            [](auto &&value) {
                static_cast<void>(value);
                return false;
            });
    }

    auto operator()(TermFunction const &term) const -> bool {
        if (type == TermCheckType::atom) {
            return !term.external;
        }
        if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) && !term.external &&
            term.pool.size() == 1 && term.pool.front().empty()) {
            if (res != nullptr) {
                res->identifier = term.name;
            }
            return true;
        }
        return false;
    }

    auto operator()(TermUnary const &term) const -> bool {
        if (type == TermCheckType::atom) {
            return term.op == UnaryOperator::negate && std::visit(*this, term.rhs);
        }
        if (type == TermCheckType::signed_identifier && term.op == UnaryOperator::negate &&
            std::visit(CheckType{TermCheckType::identifier, res}, term.rhs)) {
            if (res != nullptr) {
                res->has_sign = true;
            }
            return true;
        }
        return false;
    }

    auto operator()(TermBinary const &term) const -> bool {
        if (type == TermCheckType::sig) {
            return term.op == BinaryOperator::div &&
                   std::visit(CheckType{TermCheckType::signed_identifier, res}, term.lhs) &&
                   std::visit(CheckType{TermCheckType::signed_identifier, res}, term.rhs);
        }
        return false;
    }

    auto operator()(auto const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    TermCheckType type;
    CheckTypeResult *res;
};

} // namespace

auto check_type(TermV2 const &term, TermCheckType type, CheckTypeResult *res) -> bool {
    return std::visit(CheckType{type, res}, term);
}

} // namespace Gringo::Input
