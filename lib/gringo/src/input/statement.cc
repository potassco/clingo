#include <gringo/input/statement.hh>

namespace Gringo::Input {} // namespace Gringo::Input

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

auto value_hasher<Gringo::Input::StmTheory>::operator()(Gringo::Input::StmTheory const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmTheory), x.name_, x.term_defs_, x.atom_defs_);
}

auto value_hasher<Gringo::Input::StmRule>::operator()(Gringo::Input::StmRule const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmRule), x.head_, x.body_);
}

auto value_hasher<Gringo::Input::OptimizeTuple>::operator()(Gringo::Input::OptimizeTuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::OptimizeTuple), x.weight_, x.prio_, x.terms_);
}

auto value_hasher<Gringo::Input::Edge>::operator()(Gringo::Input::Edge const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Edge), x.src_, x.dst_);
}

auto value_hasher<Gringo::Input::StmOptimize>::operator()(Gringo::Input::StmOptimize const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmOptimize), x.elems_, x.type_);
}

auto value_hasher<Gringo::Input::StmWeakConstraint>::operator()(Gringo::Input::StmWeakConstraint const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmWeakConstraint), x.tuple_, x.body_);
}

auto value_hasher<Gringo::Input::StmShow>::operator()(Gringo::Input::StmShow const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmShow), x.term_, x.body_);
}

auto value_hasher<Gringo::Input::StmShowSig>::operator()(Gringo::Input::StmShowSig const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmShowSig), x.name_, x.arity_, x.sign_);
}

auto value_hasher<Gringo::Input::StmProject>::operator()(Gringo::Input::StmProject const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmProject), x.term_, x.body_);
}

auto value_hasher<Gringo::Input::StmProjectSig>::operator()(Gringo::Input::StmProjectSig const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmProjectSig), x.name_, x.arity_, x.sign_);
}

auto value_hasher<Gringo::Input::StmDefined>::operator()(Gringo::Input::StmDefined const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmDefined), x.name_, x.arity_, x.sign_);
}

auto value_hasher<Gringo::Input::StmExternal>::operator()(Gringo::Input::StmExternal const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmExternal), x.term_, x.body_, x.type_);
}

auto value_hasher<Gringo::Input::StmEdge>::operator()(Gringo::Input::StmEdge const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmEdge), x.edges_, x.body_);
}

auto value_hasher<Gringo::Input::StmHeuristic>::operator()(Gringo::Input::StmHeuristic const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmHeuristic), x.atom_, x.body_, x.type_, x.prio_, x.weight_);
}

auto value_hasher<Gringo::Input::StmScript>::operator()(Gringo::Input::StmScript const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmScript), x.value_, x.type_);
}

auto value_hasher<Gringo::Input::StmInclude>::operator()(Gringo::Input::StmInclude const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmInclude), x.value_, x.type_);
}

auto value_hasher<Gringo::Input::StmProgram>::operator()(Gringo::Input::StmProgram const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmProgram), x.name_, x.args_);
}

auto value_hasher<Gringo::Input::StmConst>::operator()(Gringo::Input::StmConst const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmConst), x.name_, x.value_, x.type_);
}

auto value_hasher<Gringo::Input::StmComment>::operator()(Gringo::Input::StmComment const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StmComment), x.value_, x.type_);
}

} // namespace Gringo::Util
