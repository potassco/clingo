#include <sstream>

#include <util/print.hh>

#include <theory.hh>

#include "unpool.hh"

////////// TheoryTerm //////////

[[nodiscard]] auto TheoryTerm::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream & {
    term.print(out);
    return out;
}

////////// TheoryTermUnparsed //////////

void TheoryTermUnparsed::print(std::ostream &out) const {
    bool needs_parens = !ops_.empty() || !rhs_.empty();
    if (needs_parens) {
        out << "(";
    }
    if (!ops_.empty()) {
        out << p_range(ops_, " ") << " ";
    }
    out << *term_ << p_range_with(rhs_, [](std::ostream &out, RHS const &guard) {
        out << " " << p_range(guard.first, " ") << " " << *guard.second;
    });
    if (needs_parens) {
        out << ")";
    }
}

void TheoryTermUnparsed::visit_variables(std::function<void(std::string const &var)> fun) const {
    term_->visit_variables(fun);
    for (auto const &guard : rhs_) {
        guard.second->visit_variables(fun);
    }
}

////////// TheoryTermTuple //////////

auto left_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::Tuple: {
            return '(';
        }
        case TheoryTermTupleType::Set: {
            return '{';
        }
        case TheoryTermTupleType::List: {
            break;
        }
    }
    return '[';
}

auto right_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::Tuple: {
            return ')';
        }
        case TheoryTermTupleType::Set: {
            return '}';
        }
        case TheoryTermTupleType::List: {
            break;
        }
    }
    return ']';
}

void TheoryTermTuple::print(std::ostream &out) const {
    out << left_bracket(type_) << p_range(elems_);
    if (type_ == TheoryTermTupleType::Tuple && elems_.size() == 1) {
        out << ",";
    }
    out << right_bracket(type_);
}

void TheoryTermTuple::visit_variables(std::function<void(std::string const &var)> fun) const {
    for (auto const &term : elems_) {
        term->visit_variables(fun);
    }
}

////////// TheoryTermConstant //////////

void TheoryTermSymbol::print(std::ostream &out) const { out << value_; }

void TheoryTermSymbol::visit_variables(std::function<void(std::string const &var)> fun) const {
    static_cast<void>(fun);
}

////////// TheoryTermVariable //////////

void TheoryTermVariable::print(std::ostream &out) const { out << name_; }

void TheoryTermVariable::visit_variables(std::function<void(std::string const &var)> fun) const { fun(name_); }

////////// TheoryTermFunction //////////

void TheoryTermFunction::print(std::ostream &out) const {
    out << name_;
    if (!args_.empty()) {
        out << "(" << p_range(args_) << ")";
    }
}

void TheoryTermFunction::visit_variables(std::function<void(std::string const &var)> fun) const {
    for (auto const &term : args_) {
        term->visit_variables(fun);
    }
}

////////// TheoryAtom //////////

void TheoryAtom::unpool(PoolLiteral &pool, std::function<void(std::optional<TheoryAtom>)> cb) {
    // unpool the elements
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto &elem : elems_) {
        unpool_with(
            [&](std::optional<SLiteralVec> &cond) {
                if (!cond.has_value() && !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto &[tuple, e_cond] = elem;
                elems->emplace_back(tuple, std::move(cond).value_or(e_cond));
            },
            unpool_crossproduct(pool, elem.second));
        ++i;
    }

    // unpool name and combine with the elements
    unpool_with(
        [&](std::optional<STerm> &name) {
            if (!name.has_value() && !elems.has_value()) {
                cb(std::nullopt);
                return;
            }
            cb(TheoryAtom(std::move(name).value_or(name_), elems.value_or(elems_), rhs_));
        },
        unpool_element<PoolTerm>(pool, name_));
}

auto operator<<(std::ostream &out, TheoryAtom const &atom) -> std::ostream & {
    out << "&" << *atom.name_;
    if (!atom.elems_.empty() || atom.rhs_.has_value()) {
        out << " { " << p_range_with(atom.elems_, "; ", [](std::ostream &out, TheoryAtom::Element const &elem) {
            out << p_range(elem.first);
            if (!elem.second.empty() || elem.first.empty()) {
                out << ": " << p_range(elem.second, ", ");
            }
        }) << (atom.elems_.empty() ? "}" : " }");
    }
    if (atom.rhs_.has_value()) {
        out << " " << atom.rhs_.value().first << " " << *atom.rhs_.value().second;
    }
    return out;
}

void TheoryAtom::visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const {
    name_->visit_variables(fun);
    if (rhs_.has_value()) {
        rhs_->second->visit_variables(fun);
    }
    if (ctx == VariableContext::all) {
        for (auto const &[tuple, cond] : elems_) {
            for (auto const &term : tuple) {
                term->visit_variables(fun);
            }
            for (auto const &lit : cond) {
                lit->visit_variables(fun);
            }
        }
    }
}
