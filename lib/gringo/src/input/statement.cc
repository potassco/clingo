#include <gringo/input/statement.hh>

namespace Gringo::Input {

auto operator==(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool {
    return std::tie(a.op_, a.prio_, a.type_) == std::tie(b.op_, b.prio_, b.type_);
}

auto operator<(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool {
    return std::tie(a.op_, a.prio_, a.type_) < std::tie(b.op_, b.prio_, b.type_);
}

auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
    return std::tie(a.name_, a.op_defs_) == std::tie(b.name_, b.op_defs_);
}

auto operator<(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
    return std::tie(a.name_, a.op_defs_) < std::tie(b.name_, b.op_defs_);
}

auto operator==(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.term_, a.type_, a.rhs_) ==
           std::tie(b.name_, b.arity_, b.term_, b.type_, b.rhs_);
}

auto operator<(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.term_, a.type_, a.rhs_) <
           std::tie(b.name_, b.arity_, b.term_, b.type_, b.rhs_);
}

auto operator==(TheoryDefinition const &a, TheoryDefinition const &b) -> bool {
    return std::tie(a.name_, a.term_defs_, a.atom_defs_) == std::tie(b.name_, b.term_defs_, b.atom_defs_);
}

auto operator<(TheoryDefinition const &a, TheoryDefinition const &b) -> bool {
    return std::tie(a.name_, a.term_defs_, a.atom_defs_) < std::tie(b.name_, b.term_defs_, b.atom_defs_);
}

auto operator==(Rule const &a, Rule const &b) -> bool {
    return std::tie(a.head_, a.body_) == std::tie(b.head_, b.body_);
}

auto operator<(Rule const &a, Rule const &b) -> bool { return std::tie(a.head_, a.body_) < std::tie(b.head_, b.body_); }

auto operator==(StatementOptimize::Tuple const &a, StatementOptimize::Tuple const &b) -> bool {
    return std::tie(a.weight_, a.terms_, a.priority_) == std::tie(b.weight_, b.terms_, b.priority_);
}

auto operator<(StatementOptimize::Tuple const &a, StatementOptimize::Tuple const &b) -> bool {
    return std::tie(a.weight_, a.terms_, a.priority_) < std::tie(b.weight_, b.terms_, b.priority_);
}

auto operator==(StatementOptimize const &a, StatementOptimize const &b) -> bool {
    return std::tie(a.type_, a.elems_) == std::tie(b.type_, b.elems_);
}

auto operator<(StatementOptimize const &a, StatementOptimize const &b) -> bool {
    return std::tie(a.type_, a.elems_) < std::tie(b.type_, b.elems_);
}

auto operator==(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool {
    return std::tie(a.body_, a.tuple_) == std::tie(b.body_, b.tuple_);
}

auto operator<(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool {
    return std::tie(a.body_, a.tuple_) < std::tie(b.body_, b.tuple_);
}

auto operator==(StatementShow const &a, StatementShow const &b) -> bool {
    return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
}

auto operator<(StatementShow const &a, StatementShow const &b) -> bool {
    return std::tie(a.term_, a.body_) < std::tie(b.term_, b.body_);
}

auto operator==(StatementShowSig const &a, StatementShowSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.has_sign_) == std::tie(b.name_, b.arity_, b.has_sign_);
}

auto operator<(StatementShowSig const &a, StatementShowSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.has_sign_) < std::tie(b.name_, b.arity_, b.has_sign_);
}

auto operator==(StatementProject const &a, StatementProject const &b) -> bool {
    return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
}

auto operator<(StatementProject const &a, StatementProject const &b) -> bool {
    return std::tie(a.term_, a.body_) < std::tie(b.term_, b.body_);
}

auto operator==(StatementProjectSig const &a, StatementProjectSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.has_sign_) == std::tie(b.name_, b.arity_, b.has_sign_);
}

auto operator<(StatementProjectSig const &a, StatementProjectSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.has_sign_) < std::tie(b.name_, b.arity_, b.has_sign_);
}

auto operator==(StatementDefined const &a, StatementDefined const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.has_sign_) == std::tie(b.name_, b.arity_, b.has_sign_);
}

auto operator<(StatementDefined const &a, StatementDefined const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.has_sign_) < std::tie(b.name_, b.arity_, b.has_sign_);
}

auto operator==(StatementExternal const &a, StatementExternal const &b) -> bool {
    return std::tie(a.term_, a.body_, a.type_) == std::tie(b.term_, b.body_, b.type_);
}

auto operator<(StatementExternal const &a, StatementExternal const &b) -> bool {
    return std::tie(a.term_, a.body_, a.type_) < std::tie(b.term_, b.body_, b.type_);
}

auto operator==(StatementEdge::Edge const &a, StatementEdge::Edge const &b) -> bool {
    return std::tie(a.u_, a.v_) == std::tie(b.u_, b.v_);
}

auto operator<(StatementEdge::Edge const &a, StatementEdge::Edge const &b) -> bool {
    return std::tie(a.u_, a.v_) < std::tie(b.u_, b.v_);
}

auto operator==(StatementEdge const &a, StatementEdge const &b) -> bool {
    return std::tie(a.edges_, a.body_) == std::tie(b.edges_, b.body_);
}

auto operator<(StatementEdge const &a, StatementEdge const &b) -> bool {
    return std::tie(a.edges_, a.body_) < std::tie(b.edges_, b.body_);
}

auto operator==(StatementHeuristic const &a, StatementHeuristic const &b) -> bool {
    return std::tie(a.atom_, a.body_, a.mod_, a.prio_, a.type_) == std::tie(b.atom_, b.body_, b.mod_, b.prio_, b.type_);
}

auto operator<(StatementHeuristic const &a, StatementHeuristic const &b) -> bool {
    return std::tie(a.atom_, a.body_, a.mod_, a.prio_, a.type_) < std::tie(b.atom_, b.body_, b.mod_, b.prio_, b.type_);
}

auto operator==(StatementScript const &a, StatementScript const &b) -> bool {
    return std::tie(a.content_, a.type_) == std::tie(b.content_, b.type_);
}

auto operator<(StatementScript const &a, StatementScript const &b) -> bool {
    return std::tie(a.content_, a.type_) < std::tie(b.content_, b.type_);
}

auto operator==(StatementInclude const &a, StatementInclude const &b) -> bool {
    return std::tie(a.path_, a.type_) == std::tie(b.path_, b.type_);
}

auto operator<(StatementInclude const &a, StatementInclude const &b) -> bool {
    return std::tie(a.path_, a.type_) < std::tie(b.path_, b.type_);
}

auto operator==(StatementProgram const &a, StatementProgram const &b) -> bool {
    return std::tie(a.name_, a.args_) == std::tie(b.name_, b.args_);
}

auto operator<(StatementProgram const &a, StatementProgram const &b) -> bool {
    return std::tie(a.name_, a.args_) < std::tie(b.name_, b.args_);
}

auto operator==(StatementConst const &a, StatementConst const &b) -> bool {
    return std::tie(a.name_, a.value_, a.type_) == std::tie(b.name_, b.value_, b.type_);
}

auto operator<(StatementConst const &a, StatementConst const &b) -> bool {
    return std::tie(a.name_, a.value_, a.type_) < std::tie(b.name_, b.value_, b.type_);
}

auto operator==(Comment const &a, Comment const &b) -> bool {
    return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
}

auto operator<(Comment const &a, Comment const &b) -> bool {
    return std::tie(a.value_, a.type_) < std::tie(b.value_, b.type_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::TheoryOpDefinition>::operator()(Gringo::Input::TheoryOpDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryOpDefinition), x.op_, x.prio_, x.type_);
}

auto value_hasher<Gringo::Input::TheoryTermDefinition>::operator()(Gringo::Input::TheoryTermDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermDefinition), x.name_, x.op_defs_);
}

auto value_hasher<Gringo::Input::TheoryAtomDefinition>::operator()(Gringo::Input::TheoryAtomDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryAtomDefinition), x.name_, x.arity_, x.term_, x.type_,
                                    x.rhs_);
}

auto value_hasher<Gringo::Input::TheoryDefinition>::operator()(Gringo::Input::TheoryDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryDefinition), x.name_, x.term_defs_, x.atom_defs_);
}

auto value_hasher<Gringo::Input::Rule>::operator()(Gringo::Input::Rule const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Rule), x.head_, x.body_);
}

auto value_hasher<Gringo::Input::StatementOptimize::Tuple>::operator()(
    Gringo::Input::StatementOptimize::Tuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementOptimize::Tuple), x.weight_, x.priority_, x.terms_);
}

auto value_hasher<Gringo::Input::StatementEdge::Edge>::operator()(Gringo::Input::StatementEdge::Edge const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementEdge::Edge), x.u_, x.v_);
}

auto value_hasher<Gringo::Input::StatementOptimize>::operator()(Gringo::Input::StatementOptimize const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementOptimize), x.elems_, x.type_);
}

auto value_hasher<Gringo::Input::StatementWeakConstraint>::operator()(
    Gringo::Input::StatementWeakConstraint const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementWeakConstraint), x.tuple_, x.body_);
}

auto value_hasher<Gringo::Input::StatementShow>::operator()(Gringo::Input::StatementShow const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementShow), x.term_, x.body_);
}

auto value_hasher<Gringo::Input::StatementShowSig>::operator()(Gringo::Input::StatementShowSig const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementShowSig), x.name_, x.arity_, x.has_sign_);
}

auto value_hasher<Gringo::Input::StatementProject>::operator()(Gringo::Input::StatementProject const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementProject), x.term_, x.body_);
}

auto value_hasher<Gringo::Input::StatementProjectSig>::operator()(Gringo::Input::StatementProjectSig const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementProjectSig), x.name_, x.arity_, x.has_sign_);
}

auto value_hasher<Gringo::Input::StatementDefined>::operator()(Gringo::Input::StatementDefined const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementDefined), x.name_, x.arity_, x.has_sign_);
}

auto value_hasher<Gringo::Input::StatementExternal>::operator()(Gringo::Input::StatementExternal const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementExternal), x.term_, x.body_, x.type_);
}

auto value_hasher<Gringo::Input::StatementEdge>::operator()(Gringo::Input::StatementEdge const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementEdge), x.edges_, x.body_);
}

auto value_hasher<Gringo::Input::StatementHeuristic>::operator()(Gringo::Input::StatementHeuristic const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementHeuristic), x.atom_, x.body_, x.mod_, x.prio_,
                                    x.type_);
}

auto value_hasher<Gringo::Input::StatementScript>::operator()(Gringo::Input::StatementScript const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementScript), x.content_, x.type_);
}

auto value_hasher<Gringo::Input::StatementInclude>::operator()(Gringo::Input::StatementInclude const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementInclude), x.path_, x.type_);
}

auto value_hasher<Gringo::Input::StatementProgram>::operator()(Gringo::Input::StatementProgram const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementProgram), x.name_, x.args_);
}

auto value_hasher<Gringo::Input::StatementConst>::operator()(Gringo::Input::StatementConst const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementConst), x.name_, x.value_, x.type_);
}

auto value_hasher<Gringo::Input::Comment>::operator()(Gringo::Input::Comment const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Comment), x.value_, x.type_);
}

} // namespace Gringo::Util
