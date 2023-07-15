#include <cmath>
#include <optional>
#include <tuple>
#include <utility>

#include <input/term.hh>

#include <input/algo/check_type.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

namespace Gringo::Input {

////////// Term //////////

// TODO: move/remove this

namespace {

auto projectable(Projection project, STerm const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(&term->get()->term);
    return var != nullptr && project.projectable(var->name, var->is_anonymous);
}

auto is_anonymous(STerm const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(&term->get()->term);
    return var != nullptr && var->is_anonymous;
}

struct ProjectAnonymous {
    auto operator()(STerm const &term) const { return term->project_anonymous(); }
    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (is_anonymous(std::get_if<STerm>(&elem))) {
            return {std::monostate{}};
        }
        auto sub = [](STerm const &term) { return term->project_anonymous(); };
        return transform(sub, elem);
    };
};

struct Project {
    auto operator()(STerm const &term) const { return term->project(project); }
    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (projectable(project, std::get_if<STerm>(&elem))) {
            return {std::monostate{}};
        }
        auto sub = [project = project](STerm const &term) { return term->project(project); };
        return transform(sub, elem);
    };
    Projection project;
};

auto tpa(auto const &x) { return Trans(x, ProjectAnonymous{}); }

auto tp(auto const &x, Projection project) { return Trans(x, Project{project}); }

} // namespace

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

auto Term::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    return Gringo::Input::check_type(term, type, res);
}

auto Term::is_equal(Term const &other) const -> bool { return value_equal(term, other.term); }

// until here

auto operator==(TermVariable const &a, TermVariable const &b) -> bool { return Util::value_equal(a.name, b.name); }

auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool { return Util::value_equal(a.value, b.value); }

auto operator==(TermTuple const &a, TermTuple const &b) -> bool { return Util::value_equal(a.pool, b.pool); }

auto operator==(TermFunction const &a, TermFunction const &b) -> bool {
    return Util::value_equal(a.name, b.name, a.pool, b.pool, a.external, b.external);
}

auto operator==(TermAbs const &a, TermAbs const &b) -> bool { return Util::value_equal(a.pool, b.pool); }

auto operator==(TermUnary const &a, TermUnary const &b) -> bool { return Util::value_equal(a.op, b.op, a.rhs, b.rhs); };

auto operator==(TermBinary const &a, TermBinary const &b) -> bool {
    return Util::value_equal(a.op, b.op, a.lhs, b.lhs, a.rhs, b.rhs);
};

/*
////////// TermSymbol //////////

auto TermSymbol::unpool() const -> std::optional<STermVec> { return std::nullopt; }

auto TermSymbol::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermSymbol::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

////////// TermTuple //////////

auto TermTuple::project(Projection project) const -> std::optional<STerm> {
    return transform_construct_shared<TermTuple, Term>(tp(pool_, project));
}

auto TermTuple::project_anonymous() const -> std::optional<STerm> {
    return transform_construct_shared<TermTuple, Term>(tpa(pool_));
}

auto TermTuple::unpool() const -> std::optional<STermVec> {
    // unpool the elements
    auto elems = unpool_union(pool_, [](Element const &tuple_or_term) {
        return Util::visit_variant(
            tuple_or_term,
            [](STerm const &term) -> std::optional<ElementVec> {
                return map_opt_vec(term->unpool(), [](auto term) { return Element{std::move(term)}; });
            },
            [](TupleVec const &tuple) -> std::optional<ElementVec> {
                return map_opt_vec(unpool_crossproduct(tuple,
                                                       [](TupleElem const &elem) {
                                                           return Util::visit_variant(
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
        return Util::visit_variant(
            std::move(elem), [](STerm term) { return term; },
            [](TupleVec tuple) { return Util::construct_shared<TermTuple, Term>(ElementVec{std::move(tuple)}); });
    });
}

////////// TermVariable //////////

auto TermVariable::unpool() const -> std::optional<STermVec> { return std::nullopt; }

auto TermVariable::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermVariable::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

////////// TermAbs //////////

auto TermAbs::unpool() const -> std::optional<STermVec> {
    auto unpooled = unpool_union(pool_);
    if (!unpooled.has_value() && pool_.size() != 1) {
        unpooled = pool_;
    }
    return map_opt_vec(std::move(unpooled),
                       [](auto term) { return Util::construct_shared<TermAbs, Term>(STermVec{std::move(term)}); });
}

auto TermAbs::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermAbs::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

////////// TermFunction //////////

auto TermFunction::unpool() const -> std::optional<STermVec> {
    auto elems = unpool_union(pool_, [](TupleVec const &tuple) {
        // unpool the elements
        return unpool_crossproduct(tuple, [](TupleElem const &elem) {
            return Util::visit_variant(
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
        return Util::construct_shared<TermFunction, Term>(name_, PoolVec{std::move(elem)}, external_);
    });
}

auto TermFunction::project(Projection project) const -> std::optional<STerm> {
    if (external_) {
        return std::nullopt;
    }
    return transform_construct_shared<TermFunction, Term>(name_, tp(pool_, project), external_);
}

auto TermFunction::project_anonymous() const -> std::optional<STerm> {
    if (external_) {
        return std::nullopt;
    }
    return transform_construct_shared<TermFunction, Term>(name_, tpa(pool_), external_);
}

////////// TermUnary //////////

auto TermUnary::unpool() const -> std::optional<STermVec> {
    return map_opt_vec(rhs_->unpool(),
                       [this](auto term) { return Util::construct_shared<TermUnary, Term>(op_, std::move(term)); });
}

auto TermUnary::project(Projection project) const -> std::optional<STerm> {
    if (check_type(TermCheckType::atom, nullptr)) {
        return transform_construct_shared<TermUnary, Term>(op_, tp(rhs_, project));
    }
    return std::nullopt;
}

auto TermUnary::project_anonymous() const -> std::optional<STerm> {
    if (check_type(TermCheckType::atom, nullptr)) {
        return transform_construct_shared<TermUnary, Term>(op_, tpa(rhs_));
    }
    return std::nullopt;
}

////////// TermBinary //////////

auto TermBinary::unpool() const -> std::optional<STermVec> {
    return unpool_crossproducts(
        [this](STerm lhs, STerm rhs) {
            return Util::construct_shared<TermBinary, Term>(std::move(lhs), op_, std::move(rhs));
        },
        [](STerm const &term) { return term->unpool(); }, lhs_, rhs_);
}

auto TermBinary::project(Projection project) const -> std::optional<STerm> {
    static_cast<void>(project);
    return std::nullopt;
}

auto TermBinary::project_anonymous() const -> std::optional<STerm> { return std::nullopt; }

*/

} // namespace Gringo::Input

namespace std {

auto hash<Gringo::Input::TermVariable>::operator()(Gringo::Input::TermVariable const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermVariable), x.name);
}

auto hash<Gringo::Input::TermSymbol>::operator()(Gringo::Input::TermSymbol const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermSymbol), x.value);
}

auto hash<Gringo::Input::TermTuple>::operator()(Gringo::Input::TermTuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermTuple), x.pool);
}

auto hash<Gringo::Input::TermFunction>::operator()(Gringo::Input::TermFunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermFunction), x.name, x.pool, x.external);
}

auto hash<Gringo::Input::TermAbs>::operator()(Gringo::Input::TermAbs const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermAbs), x.pool);
}

auto hash<Gringo::Input::TermUnary>::operator()(Gringo::Input::TermUnary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermUnary), x.op, x.rhs);
}

auto hash<Gringo::Input::TermBinary>::operator()(Gringo::Input::TermBinary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermBinary), x.op, x.lhs, x.rhs);
}

} // namespace std
