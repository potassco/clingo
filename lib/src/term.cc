#include <cmath>
#include <optional>
#include <sstream>
#include <tuple>
#include <utility>

#include <util/print.hh>

#include <term.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

////////// Term //////////

namespace {

struct p_tuple {
    auto operator()(std::ostream &out, TupleElem const &elem) const -> std::ostream & {
        visit_variant(
            elem, [&](std::monostate) { out << "*"; }, [&](auto const &elem) { out << *elem; });
        return out;
    }
};

auto projectable(Projection project, STerm const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = dynamic_cast<TermVariable const *>(term->get());
    return var != nullptr && project.projectable(var->name());
}

} // namespace

auto NameGen::new_name() -> std::string {
    while (true) {
        std::string name = "__Aux_" + std::to_string(num_);
        ++num_;
        if (!vars_.contains(name)) {
            return name;
        }
    }
}

auto Projection::projectable(std::string const &var) const -> bool {
    auto it = counts_.find(var);
    return it != counts_.end() && it->second == 1;
}

[[nodiscard]] auto Projection::counts() const -> std::unordered_map<std::string, size_t> const & { return counts_; }

auto operator<<(std::ostream &out, TermType type) -> std::ostream & {
    out << "TODO: type";
    return out;
}

auto operator<<(std::ostream &out, Attribute attr) -> std::ostream & {
    out << "TODO: attr";
    return out;
}

auto Term::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Term const &ast) -> std::ostream & {
    ast.print(out);
    return out;
}

auto Term::unpool() const -> STermVec {
    STermVec terms;
    PoolTerm pool{terms};
    unpool(pool);
    return terms;
}

auto Term::get_int(Attribute attr) -> int & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

auto Term::get_ast(Attribute attr) -> STerm & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

auto Term::get_ast_vec(Attribute attr) -> STermVec & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

auto Term::get_ast_vec_vec(Attribute attr) -> STermVecVec & {
    std::ostringstream out;
    out << "unknown attribute: " << attr;
    throw std::runtime_error(out.str().c_str());
}

auto Term::check_type(TermCheckType type, CheckTypeResult *res) const -> bool { return false; }

////////// TermSymbol //////////

void TermSymbol::print(std::ostream &out) const { out << value_; }

auto TermSymbol::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
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

void TermSymbol::unpool(PoolTerm &pool) const { pool.append(this); }

auto TermSymbol::unpool_v2() const -> std::optional<STermVec> { return std::nullopt; }

auto TermSymbol::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermSymbol const *>(&other);
    return d != nullptr && value_equal(value_, d->value_);
}

auto TermSymbol::hash() const -> size_t { return value_hash(typeid(TermSymbol), value_); }

void TermSymbol::visit_variables(VarVisitFun const &fun) const { static_cast<void>(fun); }

auto TermSymbol::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermSymbol::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> { return std::nullopt; }

auto TermSymbol::type() const -> TermType { return TermType::TermSymbol; }

auto TermSymbol::get_int(Attribute attr) -> int & {
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
                [&](TupleVec const &tuple) {
                    out << p_range_with(tuple, ",", p_tuple{});
                    if (tuple.size() == 1) {
                        out << ",";
                    }
                });
        }) << ")";
    }
}

auto TermTuple::type() const -> TermType { return TermType::TermTuple; }

auto TermTuple::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermTuple const *>(&other);
    return d != nullptr && value_equal(pool_, d->pool_);
}

auto TermTuple::hash() const -> size_t { return value_hash(typeid(TermTuple), pool_); }

void TermTuple::visit_variables(VarVisitFun const &fun) const {
    VarVisitor visit{fun};
    visit.add(pool_);
}

auto TermTuple::project(Projection project) const -> std::optional<STerm> {
    auto fun = [project](TupleElem const &elem) -> std::optional<TupleElem> {
        if (projectable(project, std::get_if<STerm>(&elem))) {
            return {std::monostate{}};
        }
        return std::nullopt;
    };

    return transform_construct_shared<TermTuple, Term>(Trans{pool_, fun});
}

auto TermTuple::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    auto fun = [&gen](STerm const &x) { return x->rewrite_anonymous(gen); };
    return transform_construct_shared<TermTuple, Term>(Trans{pool_, fun});
}

void TermTuple::unpool(PoolTerm &pool) const {
    for (auto const &tuple_or_term : pool_) {
        visit_variant(
            tuple_or_term, [&](STerm const &term) { term->unpool(pool); },
            [&](TupleVec const &tuple) {
                STermVec terms;
                terms.reserve(tuple.size());
                for (auto const &elem : tuple) {
                    visit_variant(
                        elem, [&](STerm const &term) { terms.emplace_back(term); }, [](auto const &) {});
                }
                unpool_with(
                    [&](std::optional<STermVec> &unpooled) {
                        if (!unpooled.has_value() && pool_.size() == 1) {
                            pool.append(this);
                        } else {
                            TupleVec unpooled_tuple;
                            if (unpooled.has_value()) {
                                auto it = unpooled->begin();
                                unpooled_tuple.reserve(tuple.size());
                                for (auto const &elem : tuple) {
                                    if (std::holds_alternative<std::monostate>(elem)) {
                                        unpooled_tuple.emplace_back(std::monostate{});
                                    } else {
                                        unpooled_tuple.emplace_back(std::move(*it));
                                        ++it;
                                    }
                                }
                            } else {
                                unpooled_tuple = tuple;
                            }
                            pool.append_shared<TermTuple>(ElementVec{std::move(unpooled_tuple)});
                        }
                    },
                    unpool_crossproduct(pool, terms));
            });
    }
}

auto TermTuple::unpool_v2() const -> std::optional<STermVec> {
    // unpool the elements
    auto elems = unpool_union_v2(pool_, [](Element const &tuple_or_term) {
        return visit_variant(
            tuple_or_term,
            [](STerm const &term) -> std::optional<ElementVec> {
                return map_opt(term->unpool_v2(), [](auto &&unpooled) {
                    return ElementVec{std::make_move_iterator(unpooled.begin()),
                                      std::make_move_iterator(unpooled.end())};
                });
            },
            [](TupleVec const &tuple) -> std::optional<ElementVec> {
                return map_opt(unpool_crossproduct_v2(
                                   tuple,
                                   [](TupleElem const &elem) {
                                       return visit_variant(
                                           elem,
                                           [](STerm const &term) -> std::optional<TupleVec> {
                                               return map_opt(term->unpool_v2(), [](auto &&unpooled) {
                                                   return TupleVec{std::make_move_iterator(unpooled.begin()),
                                                                   std::make_move_iterator(unpooled.end())};
                                               });
                                           },
                                           [](std::monostate x) -> std::optional<TupleVec> { return std::nullopt; });
                                   }),
                               [](auto &&unpooled) {
                                   return ElementVec{std::make_move_iterator(unpooled.begin()),
                                                     std::make_move_iterator(unpooled.end())};
                               });
            });
    });

    // turn the elements into individual tuple terms or terms
    if (pool_.size() != 1 || std::holds_alternative<STerm>(pool_.front())) {
        elems = pool_;
    }
    return map_opt(std::move(elems), [](auto &&elems) {
        STermVec ret;
        ret.reserve(elems.size());
        for (auto &&tuple_or_term : elems) {
            visit_variant(
                std::move(tuple_or_term), [&ret](STerm term) { ret.emplace_back(std::move(term)); },
                [&ret](TupleVec tuple) {
                    ret.emplace_back(construct_shared<TermTuple, Term>(ElementVec{std::move(tuple)}));
                });
        }
        return ret;
    });
}

////////// TermVariable //////////

auto TermVariable::name() const -> std::string const & { return name_; }

void TermVariable::print(std::ostream &out) const { out << name_; }

auto TermVariable::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermVariable const *>(&other);
    return d != nullptr && name_ == d->name_;
}

auto TermVariable::hash() const -> size_t { return value_hash(typeid(TermVariable), name_); }

void TermVariable::unpool(PoolTerm &pool) const { pool.append(this); }

auto TermVariable::unpool_v2() const -> std::optional<STermVec> { return std::nullopt; }

auto TermVariable::type() const -> TermType { return TermType::TermVariable; }

void TermVariable::visit_variables(VarVisitFun const &fun) const { fun(name_); }

auto TermVariable::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermVariable::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    if (name_ == "_") {
        return construct_shared<TermVariable, Term>(gen.new_name());
    }
    return std::nullopt;
}

////////// TermAbs //////////

void TermAbs::print(std::ostream &out) const { out << "|" << p_range(pool_, ";") << "|"; }

auto TermAbs::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermAbs const *>(&other);
    return d != nullptr && pool_ == d->pool_;
}

auto TermAbs::hash() const -> size_t { return value_hash(typeid(TermAbs), pool_); }

void TermAbs::unpool(PoolTerm &pool) const {
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

auto TermAbs::unpool_v2() const -> std::optional<STermVec> {
    return map_opt(unpool_union_v2(pool_), [this](auto &&unpooled) {
        STermVec ret;
        ret.reserve(unpooled.size());
        for (auto &&term : unpooled) {
            ret.emplace_back(construct_shared<TermAbs, Term>(STermVec{std::move(term)}));
        }
        return ret;
    });
}

auto TermAbs::type() const -> TermType { return TermType::TermAbs; }

void TermAbs::visit_variables(VarVisitFun const &fun) const {
    for (auto const &term : pool_) {
        term->visit_variables(fun);
    }
}

auto TermAbs::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermAbs::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    auto fun = [&gen](STerm const &x) { return x->rewrite_anonymous(gen); };
    return transform_construct_shared<TermAbs, Term>(Trans{pool_, fun});
}

////////// TermFunction //////////

void TermFunction::print(std::ostream &out) const {
    if (external_) {
        out << "@";
    }
    out << name_;
    if (pool_.size() != 1 || !pool_.front().empty()) {
        out << "(" << p_range_with(pool_, ";", [](std::ostream &out, TupleVec const &tuple) {
            out << p_range_with(tuple, ",", p_tuple{});
        }) << ")";
    }
}

auto TermFunction::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermFunction const *>(&other);
    return d != nullptr && value_equal(external_, d->external_, name_, d->name_, pool_, d->pool_);
}

auto TermFunction::hash() const -> size_t { return value_hash(typeid(TermFunction), external_, name_, pool_); }

void TermFunction::unpool(PoolTerm &pool) const {
    for (auto const &tuple : pool_) {
        STermVec terms;
        terms.reserve(tuple.size());
        for (auto const &elem : tuple) {
            visit_variant(
                elem, [&](STerm const &term) { terms.emplace_back(term); }, [](auto const &) {});
        }
        unpool_with(
            [&](std::optional<STermVec> &unpooled) {
                if (!unpooled.has_value() && pool_.size() == 1) {
                    pool.append(this);
                } else {
                    TupleVec unpooled_tuple;
                    if (unpooled.has_value()) {
                        auto it = unpooled->begin();
                        unpooled_tuple.reserve(tuple.size());
                        for (auto const &elem : tuple) {
                            if (std::holds_alternative<std::monostate>(elem)) {
                                unpooled_tuple.emplace_back(std::monostate{});
                            } else {
                                unpooled_tuple.emplace_back(std::move(*it));
                                ++it;
                            }
                        }
                    } else {
                        unpooled_tuple = tuple;
                    }
                    pool.append_shared<TermFunction>(name_, PoolVec{std::move(unpooled_tuple)}, external_);
                }
            },
            unpool_crossproduct(pool, terms));
    }
}

auto TermFunction::unpool_v2() const -> std::optional<STermVec> {
    return map_opt(
        unpool_union_v2(pool_,
                        [](TupleVec const &tuple) {
                            // unpool the elements
                            return map_opt(
                                unpool_crossproduct_v2(
                                    tuple,
                                    [](TupleElem const &elem) {
                                        return visit_variant(
                                            elem,
                                            [](STerm const &term) -> std::optional<TupleVec> {
                                                return map_opt(term->unpool_v2(), [](auto &&unpooled) {
                                                    return TupleVec{std::make_move_iterator(unpooled.begin()),
                                                                    std::make_move_iterator(unpooled.end())};
                                                });
                                            },
                                            [](std::monostate x) -> std::optional<TupleVec> { return std::nullopt; });
                                    }),
                                [](auto &&unpooled) {
                                    return PoolVec{std::make_move_iterator(unpooled.begin()),
                                                   std::make_move_iterator(unpooled.end())};
                                });
                        }),
        [this](auto &&elems) {
            // turn individual elements into function terms
            STermVec ret;
            ret.reserve(elems.size());
            for (auto &&tuple : elems) {
                ret.emplace_back(construct_shared<TermFunction, Term>(name_, PoolVec{std::move(tuple)}, external_));
            }
            return ret;
        });
}

void TermFunction::visit_variables(VarVisitFun const &fun) const {
    VarVisitor visit{fun};
    visit.add(pool_);
}

auto TermFunction::project(Projection project) const -> std::optional<STerm> {
    if (external_) {
        return std::nullopt;
    }

    auto fun = [project](TupleElem const &elem) -> std::optional<TupleElem> {
        if (projectable(project, std::get_if<STerm>(&elem))) {
            return {std::monostate{}};
        }
        return std::nullopt;
    };

    return transform_construct_shared<TermFunction, Term>(name_, Trans{pool_, fun}, external_);
}

auto TermFunction::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    auto fun = [&gen](STerm const &x) { return x->rewrite_anonymous(gen); };
    return transform_construct_shared<TermFunction, Term>(name_, Trans{pool_, fun}, external_);
}

auto TermFunction::type() const -> TermType { return TermType::TermFunction; }

auto TermFunction::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
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

void TermUnary::unpool(PoolTerm &pool) const {
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

auto TermUnary::unpool_v2() const -> std::optional<STermVec> {
    return map_opt(rhs_->unpool_v2(), [this](auto &&unpooled) {
        STermVec ret;
        ret.reserve(unpooled.size());
        for (auto &&term : unpooled) {
            ret.emplace_back(construct_shared<TermUnary, Term>(op_, std::move(term)));
        }
        return ret;
    });
}

auto TermUnary::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    auto fun = [&gen](STerm const &x) { return x->rewrite_anonymous(gen); };
    return transform_construct_shared<TermUnary, Term>(op_, Trans{rhs_, fun});
}

auto TermUnary::type() const -> TermType { return TermType::TermUnary; }

auto TermUnary::get_int(Attribute attr) -> int & {
    switch (attr) {
        case Attribute::Operator: {
            return reinterpret_cast<int &>(op_);
        }
        default: {
            return Term::get_int(attr);
        }
    }
}

auto TermUnary::get_ast(Attribute attr) -> STerm & {
    switch (attr) {
        case Attribute::Right: {
            return rhs_;
        }
        default: {
            return Term::get_ast(attr);
        }
    }
}

auto TermUnary::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
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

void TermUnary::visit_variables(VarVisitFun const &fun) const { rhs_->visit_variables(fun); }

auto TermUnary::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
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

auto TermBinary::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermBinary const *>(&other);
    return d != nullptr && value_equal(op_, d->op_, lhs_, d->lhs_, rhs_, d->rhs_);
}

auto TermBinary::hash() const -> size_t { return value_hash(typeid(TermBinary), op_, lhs_, rhs_); }

void TermBinary::unpool(PoolTerm &pool) const {
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

auto TermBinary::unpool_v2() const -> std::optional<STermVec> {
    return unpool_crossproducts(
        [this](STerm lhs, STerm rhs) {
            return construct_shared<TermBinary, Term>(std::move(lhs), op_, std::move(rhs));
        },
        [](STerm const &term) { return term->unpool_v2(); }, lhs_, rhs_);
}

auto TermBinary::type() const -> TermType { return TermType::TermBinary; }

auto TermBinary::get_int(Attribute attr) -> int & {
    switch (attr) {
        case Attribute::Operator: {
            return reinterpret_cast<int &>(op_);
        }
        default: {
            return Term::get_int(attr);
        }
    }
}

auto TermBinary::get_ast(Attribute attr) -> STerm & {
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

auto TermBinary::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    if (type == TermCheckType::sig) {
        return op_ == BinaryOperator::div && lhs_->check_type(TermCheckType::signed_identifier, res) &&
               rhs_->check_type(TermCheckType::pos_number, res);
    }
    return false;
}

void TermBinary::visit_variables(VarVisitFun const &fun) const {
    lhs_->visit_variables(fun);
    rhs_->visit_variables(fun);
}

auto TermBinary::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermBinary::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    auto fun = [&gen](STerm const &x) { return x->rewrite_anonymous(gen); };
    // TODO: translation could also work with like unpool_crossproducts
    return transform_construct_shared<TermBinary, Term>(Trans{lhs_, fun}, op_, Trans{rhs_, fun});
}
