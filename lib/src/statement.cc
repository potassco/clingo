#include <sstream>

#include <util/print.hh>

#include <statement.hh>

#include "unpool.hh"
#include "variables.hh"

#include <iostream>
////////// Statement { //////////

namespace {

void visit_body(VarVisitFun const &fun, VariableContext ctx, SBodyLiteralVec const &body) {
    for (auto const &lit : body) {
        lit->visit_variables(fun, ctx);
    }
}

} // namespace

class PoolStatement {
  public:
    explicit PoolStatement(SStatementVec &pool)
        : pool_{pool}, body_{body_lits_, lits_, terms_}, head_{head_lits_, lits_, terms_} {}

    operator PoolBodyLiteral &() { return body_; }
    operator PoolHeadLiteral &() { return head_; }
    template <class U> operator U &() { return static_cast<U &>(head_); }

    [[nodiscard]] auto size() const { return pool_.size(); }
    [[nodiscard]] auto operator[](size_t i) const -> SStatement const & { return pool_[i]; }

    template <typename... Args> void append(Args &&...args) { pool_.emplace_back(std::forward<Args>(args)...); };
    template <typename E, typename... Args> void append_shared(Args &&...args) {
        pool_.emplace_back(construct_shared<E, Statement>(std::forward<Args>(args)...));
    }

  private:
    SStatementVec &pool_;
    SBodyLiteralVec body_lits_;
    SHeadLiteralVec head_lits_;
    SLiteralVec lits_;
    STermVec terms_;
    PoolBodyLiteral body_;
    PoolHeadLiteral head_;
};

void destruct_pool::operator()(PoolStatement *pool) const { delete pool; }

auto construct_pool(SStatementVec &pool) -> std::unique_ptr<PoolStatement, destruct_pool> {
    return std::unique_ptr<PoolStatement, destruct_pool>{new PoolStatement{pool}};
}

auto Statement::unpool() -> SStatementVec {
    SStatementVec stms;
    PoolStatement pool{stms};
    unpool(pool);
    return stms;
}

void Statement::unpool(PoolStatement &pool) {
    size_t i = pool.size();
    do_unpool(pool);
    size_t n = pool.size();
    if (i != n - 1 || pool[i].get() != this) {
        VariableSet old_global;
        VariableSet new_global;
        visit_variables([&old_global](std::string const &var) { old_global.emplace(var); }, VariableContext::global);
        for (; i < n; ++i) {
            new_global.clear();
            auto const &stm = pool[i];
            stm->visit_variables([&new_global](std::string const &var) { new_global.emplace(var); },
                                 VariableContext::global);
            stm->visit_variables(
                [&](std::string const &var) {
                    if (old_global.contains(var) != new_global.contains(var)) {
                        std::ostringstream oss;
                        oss << "variable " << var << " in\n"
                            << "  " << *this << "\n"
                            << "is unsafe";
                        throw std::runtime_error(oss.str());
                    }
                },
                VariableContext::all);
        }
    }
}

[[nodiscard]] auto Statement::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream & {
    stm.print(out);
    return out;
}

////////// Rule //////////

void Rule::print(std::ostream &out) const {
    out << *head_;
    if (head_->print_empty() || !body_.empty()) {
        out << " :- " << p_range(body_, "; ");
    }
    out << ".";
}

void Rule::do_unpool(PoolStatement &pool) {
    unpool_with(
        [&](std::optional<SHeadLiteral> &head, std::optional<SBodyLiteralVec> &body) {
            if (!head.has_value() && !body.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<Rule>(head.value_or(head_), std::move(body).value_or(body_));
            }
        },
        unpool_element<PoolHeadLiteral>(pool, head_), unpool_crossproduct<PoolBodyLiteral>(pool, body_));
}

void Rule::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    head_->visit_variables(fun, ctx);
    visit_body(fun, ctx, body_);
}

////////// TheoryOpDefinition //////////

auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream & {
    out << def.op_ << " : " << def.prio_ << ", ";
    switch (def.type_) {
        case TheoryOpType::unary: {
            out << "unary";
            break;
        }
        case TheoryOpType::binary_left: {
            out << "binary, left";
            break;
        }
        case TheoryOpType::binary_right: {
            out << "binary, right";
            break;
        }
    }
    return out;
}

////////// TheoryTermDefinition //////////

auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream & {
    out << "  " << def.name_ << " {";
    if (def.op_defs_.empty()) {
        out << " }";
    } else if (def.op_defs_.size() == 1) {
        out << " " << def.op_defs_.front() << " }";
    } else {
        out << "\n"
            << p_range_with(def.op_defs_, ";\n", [](std::ostream &out, auto &op_def) { out << "    " << op_def; })
            << "\n  }";
    }
    return out;
}

////////// TheoryAtomDefinition //////////

auto operator<<(std::ostream &out, TheoryAtomType type) -> std::ostream & {
    switch (type) {
        case TheoryAtomType::head: {
            out << "head";
            break;
        }
        case TheoryAtomType::body: {
            out << "body";
            break;
        }
        case TheoryAtomType::any: {
            out << "any";
            break;
        }
        case TheoryAtomType::directive: {
            out << "directive";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream & {
    out << "  &" << def.name_ << "/" << def.arity_ << ": " << def.term_ << ", ";
    if (def.rhs_) {
        out << "{" << p_range(def.rhs_->first, ",") << "}, " << def.rhs_->second << ", ";
    }
    out << def.type_;
    return out;
}

////////// TheoryDefinition //////////

void TheoryDefinition::print(std::ostream &out) const {
    out << "#theory " << name_ << (term_defs_.empty() && atom_defs_.empty() ? " { " : " {\n");
    out << p_range(term_defs_, ";\n");
    if (!term_defs_.empty()) {
        if (!atom_defs_.empty()) {
            out << ";";
        }
        out << "\n";
    }
    out << p_range(atom_defs_, ";\n");
    if (!atom_defs_.empty()) {
        out << "\n";
    }
    out << "}.";
}

void TheoryDefinition::do_unpool(PoolStatement &pool) { pool.append(this); }

void TheoryDefinition::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

////////// StatementOptimize //////////

auto operator<<(std::ostream &out, OptimizeType type) -> std::ostream & {
    switch (type) {
        case OptimizeType::maximize: {
            out << "#maximize";
            break;
        }
        case OptimizeType::minimize: {
            out << "#minimize";
            break;
        }
    }
    return out;
}

void StatementOptimize::print(std::ostream &out) const {
    out << type_ << " { "
        << p_range_with(elems_, "; ",
                        [](std::ostream &out, auto const &elem) {
                            auto const &[tuple, cond] = elem;
                            auto const &[weight, prio, terms] = tuple;
                            out << *weight;
                            if (prio) {
                                out << "@" << *prio.value();
                            }
                            if (!terms.empty()) {
                                out << "," << p_range(terms);
                            }
                            if (!cond.empty()) {
                                out << ": " << p_range(cond, ", ");
                            }
                        })
        << (elems_.empty() ? "}" : " }") << ".";
}

void StatementOptimize::do_unpool(PoolStatement &pool) {
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto &elem : elems_) {
        unpool_with(
            [&](std::optional<STerm> &weight, std::optional<STerm> &prio, std::optional<STermVec> &terms,
                std::optional<SLiteralVec> &cond) {
                if (!weight.has_value() && !prio.has_value() && !terms.has_value() && !cond.has_value() &&
                    !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto &[e_tuple, e_cond] = elem;
                auto &[e_weight, e_prio, e_terms] = e_tuple;
                elems->emplace_back(Tuple{weight.value_or(e_weight), prio ? prio : e_prio, terms.value_or(e_terms)},
                                    std::move(cond).value_or(e_cond));
            },
            unpool_element<PoolTerm>(pool, std::get<0>(elem.first)),
            unpool_element<PoolTerm>(pool, std::get<1>(elem.first)),
            unpool_crossproduct<PoolTerm>(pool, std::get<2>(elem.first)),
            unpool_crossproduct<PoolLiteral>(pool, elem.second));
        ++i;
    }
    if (!elems.has_value()) {
        pool.append(this);
    } else {
        pool.append_shared<StatementOptimize>(type_, std::move(elems.value()));
    }
}

void StatementOptimize::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    if (ctx == VariableContext::all) {
        VarVisitor visit{fun};
        visit.add(elems_);
    }
}

////////// StatementWeakConstraint //////////

void StatementWeakConstraint::print(std::ostream &out) const {
    auto const &[weight, prio, terms] = tuple_;
    out << " :~ " << p_range(body_, "; ") << ". [" << *weight;
    if (prio) {
        out << "@" << *prio.value();
    }
    if (!terms.empty()) {
        out << "," << p_range(terms);
    }
    out << "]";
}

void StatementWeakConstraint::do_unpool(PoolStatement &pool) {
    unpool_with(
        [&](std::optional<STerm> &weight, std::optional<STerm> &prio, std::optional<STermVec> &terms,
            std::optional<SBodyLiteralVec> &body) {
            if (!weight.has_value() && !prio.has_value() && !terms.has_value() && !body.has_value()) {
                pool.append(this);
            } else {
                auto &[e_weight, e_prio, e_terms] = tuple_;
                pool.append_shared<StatementWeakConstraint>(
                    std::move(body).value_or(body_),
                    Tuple{weight.value_or(e_weight), prio ? prio : e_prio, terms.value_or(e_terms)});
            }
        },
        unpool_element<PoolTerm>(pool, std::get<0>(tuple_)), unpool_element<PoolTerm>(pool, std::get<1>(tuple_)),
        unpool_crossproduct<PoolTerm>(pool, std::get<2>(tuple_)), unpool_crossproduct<PoolBodyLiteral>(pool, body_));
}

void StatementWeakConstraint::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(tuple_);
    visit_body(fun, ctx, body_);
}

////////// StatementShow //////////

void StatementShow::print(std::ostream &out) const { out << "#show " << *term_ << ": " << p_range(body_, "; ") << "."; }

void StatementShow::do_unpool(PoolStatement &pool) {
    unpool_with(
        [&](std::optional<STerm> &term, std::optional<SBodyLiteralVec> &body) {
            if (!term.has_value() && !body.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<StatementShow>(term.value_or(term_), std::move(body).value_or(body_));
            }
        },
        unpool_element<PoolTerm>(pool, term_), unpool_crossproduct<PoolBodyLiteral>(pool, body_));
}

void StatementShow::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    term_->visit_variables(fun);
    visit_body(fun, ctx, body_);
}

////////// StatementShowSig //////////

void StatementShowSig::print(std::ostream &out) const {
    out << "#show " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

void StatementShowSig::do_unpool(PoolStatement &pool) { pool.append(this); }

void StatementShowSig::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

////////// StatementProject //////////

void StatementProject::print(std::ostream &out) const {
    out << "#project " << *term_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
}

void StatementProject::do_unpool(PoolStatement &pool) {
    unpool_with(
        [&](std::optional<STerm> &term, std::optional<SBodyLiteralVec> &body) {
            if (!term.has_value() && !body.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<StatementProject>(term.value_or(term_), std::move(body).value_or(body_));
            }
        },
        unpool_element<PoolTerm>(pool, term_), unpool_crossproduct<PoolBodyLiteral>(pool, body_));
}

void StatementProject::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    term_->visit_variables(fun);
    visit_body(fun, ctx, body_);
}

////////// StatementProjectSig //////////

void StatementProjectSig::print(std::ostream &out) const {
    out << "#project " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

void StatementProjectSig::do_unpool(PoolStatement &pool) { pool.append(this); }

void StatementProjectSig::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

////////// StatementDefined //////////

void StatementDefined::print(std::ostream &out) const {
    out << "#defined " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

void StatementDefined::do_unpool(PoolStatement &pool) { pool.append(this); }

void StatementDefined::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

////////// StatementExternal //////////

void StatementExternal::print(std::ostream &out) const {
    out << "#external " << *term_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
    if (type_.has_value()) {
        out << " [" << *type_.value() << "]";
    }
}

void StatementExternal::do_unpool(PoolStatement &pool) {
    unpool_with(
        [&](std::optional<STerm> &term, std::optional<STerm> &type, std::optional<SBodyLiteralVec> &body) {
            if (!term.has_value() && !type.has_value() && !body.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<StatementExternal>(term.value_or(term_), std::move(body).value_or(body_),
                                                      type ? type : type_);
            }
        },
        unpool_element<PoolTerm>(pool, term_), unpool_element<PoolTerm>(pool, type_),
        unpool_crossproduct<PoolBodyLiteral>(pool, body_));
}

void StatementExternal::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(term_, type_);
    visit_body(fun, ctx, body_);
}

////////// StatementEdge //////////

void StatementEdge::print(std::ostream &out) const {
    out << "#edge ("
        << p_range_with(edges_, ";", [](std::ostream &out, auto &edge) { out << *edge.first << "," << *edge.second; })
        << ")" << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
}

void StatementEdge::do_unpool(PoolStatement &pool) {
    // unpool bodies
    std::optional<std::vector<SBodyLiteralVec>> bodies;
    unpool_with(
        [&](std::optional<SBodyLiteralVec> &body) {
            if (!body.has_value()) {
                return;
            }
            if (!bodies.has_value()) {
                bodies = std::vector<SBodyLiteralVec>{};
            }
            bodies->emplace_back(std::move(body).value());
        },
        unpool_crossproduct<PoolBodyLiteral>(pool, body_));
    // combine bodies and edges
    for (auto &edge : edges_) {
        unpool_with(
            [&](std::optional<STerm> &u, std::optional<STerm> &v) {
                if (!u.has_value() && !v.has_value() && !bodies.has_value() && edges_.size() == 1) {
                    pool.append(this);
                }
                auto edges = EdgeVec{Edge{u.value_or(edge.first), v.value_or(edge.second)}};
                if (bodies.has_value()) {
                    for (auto &body : bodies.value()) {
                        pool.append_shared<StatementEdge>(edges, body);
                    }
                } else {
                    pool.append_shared<StatementEdge>(std::move(edges), body_);
                }
            },
            unpool_element<PoolTerm>(pool, edge.first), unpool_element<PoolTerm>(pool, edge.second));
    }
}

void StatementEdge::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(edges_);
    visit_body(fun, ctx, body_);
}

////////// StatementHeuristic //////////

void StatementHeuristic::print(std::ostream &out) const {
    out << "#heuristic " << (has_sign_ ? "-" : "") << *atom_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ")
        << ". [" << *type_;
    if (prio_) {
        out << "@" << *prio_.value();
    }
    out << "," << *mod_ << "]";
}

void StatementHeuristic::do_unpool(PoolStatement &pool) {
    unpool_with(
        [&](std::optional<STerm> &atom, std::optional<STerm> &type, std::optional<STerm> &prio,
            std::optional<STerm> &mod, std::optional<SBodyLiteralVec> &body) {
            if (!atom.has_value() && !type.has_value() && !prio.has_value() && !mod.has_value() && !body.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<StatementHeuristic>(has_sign_, atom.value_or(atom_), std::move(body).value_or(body_),
                                                       type.value_or(type_), prio ? prio : prio_, mod.value_or(mod_));
            }
        },
        unpool_element<PoolTerm>(pool, atom_), unpool_element<PoolTerm>(pool, type_),
        unpool_element<PoolTerm>(pool, prio_), unpool_element<PoolTerm>(pool, mod_),
        unpool_crossproduct<PoolBodyLiteral>(pool, body_));
}

void StatementHeuristic::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(type_, prio_, mod_);
    visit_body(fun, ctx, body_);
}

////////// StatementScript //////////

auto operator<<(std::ostream &out, ScriptType type) -> std::ostream & {
    switch (type) {
        case ScriptType::lua: {
            out << "lua";
            break;
        }
        case ScriptType::python: {
            out << "python";
            break;
        }
    }
    return out;
}

void StatementScript::print(std::ostream &out) const { out << "#script (" << type_ << ")" << content_ << "#end."; }

void StatementScript::do_unpool(PoolStatement &pool) { pool.append(this); }

void StatementScript::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

////////// StatementInclude //////////

auto operator<<(std::ostream &out, IncludeType type) -> std::ostream & {
    switch (type) {
        case IncludeType::inbuild: {
            out << "lua";
            break;
        }
        case IncludeType::system: {
            out << "python";
            break;
        }
    }
    return out;
}

void StatementInclude::print(std::ostream &out) const {
    if (type_ == IncludeType::inbuild) {
        out << "#include <" << path_ << ">.";
    } else {
        out << "#include ";
        print_quoted(out, path_);
        out << ".";
    }
}

void StatementInclude::do_unpool(PoolStatement &pool) { pool.append(this); }

void StatementInclude::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

////////// StatementProgram //////////

void StatementProgram::print(std::ostream &out) const {
    out << "#program " << name_;
    if (!args_.empty()) {
        out << "(" << p_range(args_) << ")";
    }
    out << ".";
}

void StatementProgram::do_unpool(PoolStatement &pool) { pool.append(this); }

void StatementProgram::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

////////// StatementConst //////////

auto operator<<(std::ostream &out, ConstType type) -> std::ostream & {
    switch (type) {
        case ConstType::default_: {
            out << "default";
            break;
        }
        case ConstType::override_: {
            out << "override";
            break;
        }
    }
    return out;
}

void StatementConst::print(std::ostream &out) const {
    out << "#const " << name_ << "=" << *value_ << ". [" << type_ << "]";
}

void StatementConst::do_unpool(PoolStatement &pool) {
    size_t n = 0;
    unpool_with(
        [&](std::optional<STerm> value) {
            if (!value.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<StatementConst>(type_, name_, value.value());
            }
            ++n;
        },
        unpool_element<PoolTerm>(pool, value_));
    if (n != 1) {
        throw std::runtime_error("const statements must not contain pools");
    }
}

void StatementConst::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}
