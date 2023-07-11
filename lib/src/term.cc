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

namespace Gringo::Input {

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
    return var != nullptr && project.projectable(var->name(), var->is_anonymous());
}

auto is_anonymous(STerm const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = dynamic_cast<TermVariable const *>(term->get());
    return var != nullptr && var->is_anonymous();
}

auto associativity(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::dots:
        case BinaryOperator::xor_:
        case BinaryOperator::or_:
        case BinaryOperator::and_:
        case BinaryOperator::plus:
        case BinaryOperator::minus:
        case BinaryOperator::times:
        case BinaryOperator::div:
        case BinaryOperator::mod: {
            return Position::left;
        }
        case BinaryOperator::pow: {
            break;
        }
    }
    return Position::right;
}

auto priority(BinaryOperator op) -> unsigned int {
    switch (op) {
        case BinaryOperator::dots: {
            return 1;
        }
        case BinaryOperator::xor_: {
            return 2;
        }
        case BinaryOperator::or_: {
            return 3;
        }
        case BinaryOperator::and_: {
            return 4;
        }
        case BinaryOperator::plus:
        case BinaryOperator::minus: {
            return 5; // NOLINT
        }
        case BinaryOperator::times:
        case BinaryOperator::div:
        case BinaryOperator::mod: {
            return 6; // NOLINT
        }
        case BinaryOperator::pow: {
            break;
        }
    }
    return 8; // NOLINT
}

auto priority(UnaryOperator op) -> unsigned int {
    static_cast<void>(op);
    return priority(BinaryOperator::times) + 1;
}

struct ProjectAnonymous {
    auto operator()(STerm const &term) const { return term->project_anonymous(); }
    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (is_anonymous(std::get_if<STerm>(&elem))) {
            return {std::monostate{}};
        }
        auto sub = [](STerm const &term) { return term->project_anonymous(); };
        return Util::transform(sub, elem);
    };
};

struct Project {
    auto operator()(STerm const &term) const { return term->project(project); }
    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (projectable(project, std::get_if<STerm>(&elem))) {
            return {std::monostate{}};
        }
        auto sub = [project = project](STerm const &term) { return term->project(project); };
        return Util::transform(sub, elem);
    };
    Projection project;
};

auto tra(auto const &x, NameGen &gen) {
    return Util::Trans(x, [&gen](STerm const &term) { return term->rewrite_anonymous(gen); });
}

auto tpa(auto const &x) { return Util::Trans(x, ProjectAnonymous{}); }

auto tp(auto const &x, Projection project) { return Util::Trans(x, Project{project}); }

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

auto Projection::projectable(std::string const &var, bool anonymous) const -> bool {
    if (mode_ == ProjectionMode::disabled) {
        return false;
    }
    if (mode_ == ProjectionMode::anonymous && !anonymous) {
        return false;
    }
    auto it = counts_.find(var);
    return it != counts_.end() && it->second == 1;
}

[[nodiscard]] auto Projection::counts() const -> std::unordered_map<std::string, size_t> const & { return counts_; }

auto Projection::mode() const -> ProjectionMode { return mode_; }

void Term::print(std::ostream &out) const { do_print(out, false, 0, Position::none); }

/*
auto operator<<(std::ostream &out, TermType type) -> std::ostream & {
    static_cast<void>(type);
    out << "TODO: type";
    return out;
}

auto operator<<(std::ostream &out, Attribute attr) -> std::ostream & {
    static_cast<void>(attr);
    out << "TODO: attr";
    return out;
}
*/
auto Term::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Term const &ast) -> std::ostream & {
    ast.print(out);
    return out;
}

/*
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
*/

auto Term::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    static_cast<void>(type);
    static_cast<void>(res);
    return false;
}

////////// TermSymbol //////////

void TermSymbol::do_print(std::ostream &out, bool no_leading_op, unsigned int prio, Position pos) const {
    static_cast<void>(prio);
    static_cast<void>(pos);
    char const *lp = "";
    char const *rp = "";
    if (no_leading_op && has_sign(value_)) {
        lp = "(";
        rp = "(";
    }
    out << lp << value_ << rp;
}

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
        [&](auto &&value) {
            static_cast<void>(value);
            return false;
        });
}

auto TermSymbol::unpool() const -> std::optional<STermVec> { return std::nullopt; }

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

auto TermSymbol::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

auto TermSymbol::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    static_cast<void>(gen);
    return std::nullopt;
}

/*
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
*/

////////// TermTuple //////////

void TermTuple::do_print(std::ostream &out, bool no_leading_op, unsigned int prio, Position pos) const {
    if (pool_.size() == 1 && std::holds_alternative<STerm>(pool_.front())) {
        std::get<STerm>(pool_.front())->do_print(out, no_leading_op, prio, pos);
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
    return Util::transform_construct_shared<TermTuple, Term>(tp(pool_, project));
}

auto TermTuple::project_anonymous() const -> std::optional<STerm> {
    return Util::transform_construct_shared<TermTuple, Term>(tpa(pool_));
}

auto TermTuple::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    return Util::transform_construct_shared<TermTuple, Term>(tra(pool_, gen));
}

auto TermTuple::unpool() const -> std::optional<STermVec> {
    // unpool the elements
    auto elems = unpool_union(pool_, [](Element const &tuple_or_term) {
        return visit_variant(
            tuple_or_term,
            [](STerm const &term) -> std::optional<ElementVec> {
                return map_opt_vec(term->unpool(), [](auto term) { return Element{std::move(term)}; });
            },
            [](TupleVec const &tuple) -> std::optional<ElementVec> {
                return map_opt_vec(unpool_crossproduct(tuple,
                                                       [](TupleElem const &elem) {
                                                           return visit_variant(
                                                               elem,
                                                               [](STerm const &term) -> std::optional<TupleVec> {
                                                                   return map_opt_vec(term->unpool(), [](auto term) {
                                                                       return TupleElem{std::move(term)};
                                                                   });
                                                               },
                                                               [](std::monostate x) -> std::optional<TupleVec> {
                                                                   static_cast<void>(x);
                                                                   return std::nullopt;
                                                               });
                                                       }),
                                   [](auto tuple) { return Element{std::move(tuple)}; });
            });
    });

    // turn the elements into individual tuple terms or terms
    if (!elems.has_value() && (pool_.size() != 1 || std::holds_alternative<STerm>(pool_.front()))) {
        elems = pool_;
    }
    return map_opt_vec(std::move(elems), [](auto elem) -> STerm {
        return visit_variant(
            std::move(elem), [](STerm term) { return term; },
            [](TupleVec tuple) { return construct_shared<TermTuple, Term>(ElementVec{std::move(tuple)}); });
    });
}

/*
auto TermTuple::type() const -> TermType { return TermType::TermTuple; }
*/

////////// TermVariable //////////

auto TermVariable::name() const -> std::string const & { return name_; }

auto TermVariable::is_anonymous() const -> bool { return is_anonymous_; }

void TermVariable::do_print(std::ostream &out, bool no_leading_op, unsigned int prio, Position pos) const {
    static_cast<void>(no_leading_op);
    static_cast<void>(prio);
    static_cast<void>(pos);
    out << name_;
}

auto TermVariable::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermVariable const *>(&other);
    return d != nullptr && name_ == d->name_;
}

auto TermVariable::hash() const -> size_t { return value_hash(typeid(TermVariable), name_); }

auto TermVariable::unpool() const -> std::optional<STermVec> { return std::nullopt; }

void TermVariable::visit_variables(VarVisitFun const &fun) const { fun(name_); }

auto TermVariable::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermVariable::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

auto TermVariable::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    if (is_anonymous_) {
        return construct_shared<TermVariable, Term>(gen.new_name(), true);
    }
    return std::nullopt;
}

/*
auto TermVariable::type() const -> TermType { return TermType::TermVariable; }
*/

////////// TermAbs //////////

void TermAbs::do_print(std::ostream &out, bool no_leading_op, unsigned int prio, Position pos) const {
    static_cast<void>(no_leading_op);
    static_cast<void>(prio);
    static_cast<void>(pos);
    out << "|" << p_range(pool_, ";") << "|";
}

auto TermAbs::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermAbs const *>(&other);
    return d != nullptr && pool_ == d->pool_;
}

auto TermAbs::hash() const -> size_t { return value_hash(typeid(TermAbs), pool_); }

auto TermAbs::unpool() const -> std::optional<STermVec> {
    auto unpooled = unpool_union(pool_);
    if (!unpooled.has_value() && pool_.size() != 1) {
        unpooled = pool_;
    }
    return map_opt_vec(std::move(unpooled),
                       [](auto term) { return construct_shared<TermAbs, Term>(STermVec{std::move(term)}); });
}

void TermAbs::visit_variables(VarVisitFun const &fun) const {
    for (auto const &term : pool_) {
        term->visit_variables(fun);
    }
}

auto TermAbs::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermAbs::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

auto TermAbs::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    return Util::transform_construct_shared<TermAbs, Term>(tra(pool_, gen));
}

/*
auto TermAbs::type() const -> TermType { return TermType::TermAbs; }
*/

////////// TermFunction //////////

void TermFunction::do_print(std::ostream &out, bool no_leading_op, unsigned int prio, Position pos) const {
    static_cast<void>(no_leading_op);
    static_cast<void>(prio);
    static_cast<void>(pos);
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

auto TermFunction::unpool() const -> std::optional<STermVec> {
    auto elems = unpool_union(pool_, [](TupleVec const &tuple) {
        // unpool the elements
        return unpool_crossproduct(tuple, [](TupleElem const &elem) {
            return visit_variant(
                elem,
                [](STerm const &term) -> std::optional<TupleVec> {
                    return map_opt_vec(term->unpool(), [](auto term) { return TupleElem{std::move(term)}; });
                },
                [](std::monostate x) -> std::optional<TupleVec> {
                    static_cast<void>(x);
                    return std::nullopt;
                });
        });
    });

    if (!elems.has_value() && pool_.size() != 1) {
        elems = pool_;
    }

    return map_opt_vec(std::move(elems), [this](auto elem) {
        // turn individual elements into function terms
        return construct_shared<TermFunction, Term>(name_, PoolVec{std::move(elem)}, external_);
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
    return Util::transform_construct_shared<TermFunction, Term>(name_, tp(pool_, project), external_);
}

auto TermFunction::project_anonymous() const -> std::optional<STerm> {
    if (external_) {
        return std::nullopt;
    }
    return Util::transform_construct_shared<TermFunction, Term>(name_, tpa(pool_), external_);
}

auto TermFunction::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    return Util::transform_construct_shared<TermFunction, Term>(name_, tra(pool_, gen), external_);
}

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

/*
auto TermFunction::type() const -> TermType { return TermType::TermFunction; }
*/

////////// TermUnary //////////

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream & {
    out << (op == UnaryOperator::negate ? "-" : "~");
    return out;
}

void TermUnary::do_print(std::ostream &out, bool no_leading_op, unsigned int prio, Position pos) const {
    static_cast<void>(pos);
    char const *lp = "";
    char const *rp = "";
    // No need to consider associativity/position because the unary priority is
    // different from all binary ones.
    if (no_leading_op || (priority(op_) < prio)) {
        lp = "(";
        rp = ")";
    }
    out << lp << op_;
    rhs_->do_print(out, true, priority(op_), Position::none);
    out << rp;
}

auto TermUnary::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermUnary const *>(&other);
    return d != nullptr && value_equal(op_, d->op_, rhs_, d->rhs_);
}

auto TermUnary::hash() const -> size_t { return value_hash(typeid(TermUnary), op_, rhs_); }

auto TermUnary::unpool() const -> std::optional<STermVec> {
    return map_opt_vec(rhs_->unpool(),
                       [this](auto term) { return construct_shared<TermUnary, Term>(op_, std::move(term)); });
}

auto TermUnary::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    return Util::transform_construct_shared<TermUnary, Term>(op_, tra(rhs_, gen));
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
    if (check_type(TermCheckType::atom, nullptr)) {
        return Util::transform_construct_shared<TermUnary, Term>(op_, tp(rhs_, project));
    }
    return std::nullopt;
}

auto TermUnary::project_anonymous() const -> std::optional<STerm> {
    if (check_type(TermCheckType::atom, nullptr)) {
        return Util::transform_construct_shared<TermUnary, Term>(op_, tpa(rhs_));
    }
    return std::nullopt;
}

/*
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
*/

////////// TermBinary //////////

auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream & {
    switch (op) {
        case BinaryOperator::dots: {
            out << "..";
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

void TermBinary::do_print(std::ostream &out, bool no_leading_op, unsigned int prio, Position pos) const {
    char const *lp = "";
    char const *rp = "";
    // We assume that operators with the same priority have the same associativity.
    if (priority(op_) < prio || (prio == priority(op_) && associativity(op_) != pos)) {
        lp = "(";
        rp = ")";
        no_leading_op = false;
    }
    out << lp;
    lhs_->do_print(out, no_leading_op, priority(op_), Position::left);
    out << op_;
    rhs_->do_print(out, true, priority(op_), Position::right);
    out << rp;
}

auto TermBinary::is_equal(Term const &other) const -> bool {
    auto const *d = dynamic_cast<TermBinary const *>(&other);
    return d != nullptr && value_equal(op_, d->op_, lhs_, d->lhs_, rhs_, d->rhs_);
}

auto TermBinary::hash() const -> size_t { return value_hash(typeid(TermBinary), op_, lhs_, rhs_); }

auto TermBinary::unpool() const -> std::optional<STermVec> {
    return unpool_crossproducts(
        [this](STerm lhs, STerm rhs) {
            return construct_shared<TermBinary, Term>(std::move(lhs), op_, std::move(rhs));
        },
        [](STerm const &term) { return term->unpool(); }, lhs_, rhs_);
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

auto TermBinary::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

auto TermBinary::rewrite_anonymous(NameGen &gen) const -> std::optional<STerm> {
    return Util::transform_construct_shared<TermBinary, Term>(tra(lhs_, gen), op_, tra(rhs_, gen));
}

/*
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
*/

} // namespace Gringo::Input
