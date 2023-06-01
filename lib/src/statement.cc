#include <sstream>

#include <util/print.hh>

#include <statement.hh>

////////// Statement { //////////

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

////////// StatementShow //////////

void StatementShow::print(std::ostream &out) const { out << "#show " << *term_ << ": " << p_range(body_, "; ") << "."; }

////////// StatementShowSig //////////

void StatementShowSig::print(std::ostream &out) const {
    out << "#show " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

////////// StatementProject //////////

void StatementProject::print(std::ostream &out) const {
    out << "#project " << *term_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
}

////////// StatementProjectSig //////////

void StatementProjectSig::print(std::ostream &out) const {
    out << "#project " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

////////// StatementDefined //////////

void StatementDefined::print(std::ostream &out) const {
    out << "#defined " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

////////// StatementExternal //////////

void StatementExternal::print(std::ostream &out) const {
    out << "#external " << *term_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
    if (type_.has_value()) {
        out << " [" << *type_.value() << "]";
    }
}

////////// StatementEdge //////////

void StatementEdge::print(std::ostream &out) const {
    out << "#edge ("
        << p_range_with(edges_, ";", [](std::ostream &out, auto &edge) { out << *edge.first << "," << *edge.second; })
        << ")" << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
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

////////// StatementProgram //////////

void StatementProgram::print(std::ostream &out) const {
    out << "#program " << name_;
    if (!args_.empty()) {
        out << "(" << p_range(args_) << ")";
    }
    out << ".";
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
