#include <sstream>

#include <util/print.hh>

#include <body_literal.hh>

////////// BodyLiteral //////////

[[nodiscard]] auto BodyLiteral::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, BodyLiteral const &literal) -> std::ostream & {
    literal.print(out);
    return out;
}

////////// ConditionalLiteral //////////

void ConditionalLiteral::add_sign(Sign sign) { lit_->add_sign(sign); }

void ConditionalLiteral::print(std::ostream &out) const {
    out << *lit_;
    if (!cond_.empty()) {
        out << ": " << p_range(cond_, ", ");
    }
}

////////// BodyAggregate //////////

void BodyAggregate::add_sign(Sign sign) { sign_ += sign; }

void BodyAggregate::set_left_guard(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

void BodyAggregate::print(std::ostream &out) const {
    out << sign_;
    if (lhs_) {
        out << *lhs_->first << " " << lhs_->second << " ";
    }
    out << fun_ << " { " << p_range_with(elements, "; ", [](std::ostream &out, auto const &elem) {
        out << p_range{std::get<0>(elem), ","};
        if (!std::get<1>(elem).empty()) {
            out << ": " << p_range{std::get<1>(elem), ", "};
        }
    }) << (elements.empty() ? "}" : " }");
    if (rhs_) {
        out << " " << rhs_->first << " " << *rhs_->second;
    }
}

////////// BodySetAggregate //////////

void BodySetAggregate::add_sign(Sign sign) { sign_ += sign; }

void BodySetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void BodySetAggregate::print(std::ostream &out) const { out << sign_ << aggr_; }

////////// BodyTheoryAtom //////////

void BodyTheoryAtom::add_sign(Sign sign) { sign_ += sign; }

void BodyTheoryAtom::print(std::ostream &out) const { out << sign_ << atom_; }
