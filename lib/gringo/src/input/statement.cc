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

auto operator==(StmTheory const &a, StmTheory const &b) -> bool {
    return std::tie(a.name_, a.term_defs_, a.atom_defs_) == std::tie(b.name_, b.term_defs_, b.atom_defs_);
}

auto operator<(StmTheory const &a, StmTheory const &b) -> bool {
    return std::tie(a.name_, a.term_defs_, a.atom_defs_) < std::tie(b.name_, b.term_defs_, b.atom_defs_);
}

auto operator==(StmRule const &a, StmRule const &b) -> bool {
    return std::tie(a.head_, a.body_) == std::tie(b.head_, b.body_);
}

auto operator<(StmRule const &a, StmRule const &b) -> bool {
    return std::tie(a.head_, a.body_) < std::tie(b.head_, b.body_);
}

auto operator==(OptimizeTuple const &a, OptimizeTuple const &b) -> bool {
    return std::tie(a.weight_, a.terms_, a.prio_) == std::tie(b.weight_, b.terms_, b.prio_);
}

auto operator<(OptimizeTuple const &a, OptimizeTuple const &b) -> bool {
    return std::tie(a.weight_, a.terms_, a.prio_) < std::tie(b.weight_, b.terms_, b.prio_);
}

auto operator==(StmOptimize const &a, StmOptimize const &b) -> bool {
    return std::tie(a.type_, a.elems_) == std::tie(b.type_, b.elems_);
}

auto operator<(StmOptimize const &a, StmOptimize const &b) -> bool {
    return std::tie(a.type_, a.elems_) < std::tie(b.type_, b.elems_);
}

auto operator==(StmWeakConstraint const &a, StmWeakConstraint const &b) -> bool {
    return std::tie(a.body_, a.tuple_) == std::tie(b.body_, b.tuple_);
}

auto operator<(StmWeakConstraint const &a, StmWeakConstraint const &b) -> bool {
    return std::tie(a.body_, a.tuple_) < std::tie(b.body_, b.tuple_);
}

auto operator==(StmShow const &a, StmShow const &b) -> bool {
    return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
}

auto operator<(StmShow const &a, StmShow const &b) -> bool {
    return std::tie(a.term_, a.body_) < std::tie(b.term_, b.body_);
}

auto operator==(StmShowSig const &a, StmShowSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
}

auto operator<(StmShowSig const &a, StmShowSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.sign_) < std::tie(b.name_, b.arity_, b.sign_);
}

auto operator==(StmProject const &a, StmProject const &b) -> bool {
    return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
}

auto operator<(StmProject const &a, StmProject const &b) -> bool {
    return std::tie(a.term_, a.body_) < std::tie(b.term_, b.body_);
}

auto operator==(StmProjectSig const &a, StmProjectSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
}

auto operator<(StmProjectSig const &a, StmProjectSig const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.sign_) < std::tie(b.name_, b.arity_, b.sign_);
}

auto operator==(StmDefined const &a, StmDefined const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
}

auto operator<(StmDefined const &a, StmDefined const &b) -> bool {
    return std::tie(a.name_, a.arity_, a.sign_) < std::tie(b.name_, b.arity_, b.sign_);
}

auto operator==(StmExternal const &a, StmExternal const &b) -> bool {
    return std::tie(a.term_, a.body_, a.type_) == std::tie(b.term_, b.body_, b.type_);
}

auto operator<(StmExternal const &a, StmExternal const &b) -> bool {
    return std::tie(a.term_, a.body_, a.type_) < std::tie(b.term_, b.body_, b.type_);
}

auto operator==(Edge const &a, Edge const &b) -> bool { return std::tie(a.src_, a.dst_) == std::tie(b.src_, b.dst_); }

auto operator<(Edge const &a, Edge const &b) -> bool { return std::tie(a.src_, a.dst_) < std::tie(b.src_, b.dst_); }

auto operator==(StmEdge const &a, StmEdge const &b) -> bool {
    return std::tie(a.edges_, a.body_) == std::tie(b.edges_, b.body_);
}

auto operator<(StmEdge const &a, StmEdge const &b) -> bool {
    return std::tie(a.edges_, a.body_) < std::tie(b.edges_, b.body_);
}

auto operator==(StmHeuristic const &a, StmHeuristic const &b) -> bool {
    return std::tie(a.atom_, a.body_, a.type_, a.prio_, a.weight_) ==
           std::tie(b.atom_, b.body_, b.type_, b.prio_, b.weight_);
}

auto operator<(StmHeuristic const &a, StmHeuristic const &b) -> bool {
    return std::tie(a.atom_, a.body_, a.type_, a.prio_, a.weight_) <
           std::tie(b.atom_, b.body_, b.type_, b.prio_, b.weight_);
}

auto operator==(StmScript const &a, StmScript const &b) -> bool {
    return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
}

auto operator<(StmScript const &a, StmScript const &b) -> bool {
    return std::tie(a.value_, a.type_) < std::tie(b.value_, b.type_);
}

auto operator==(StmInclude const &a, StmInclude const &b) -> bool {
    return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
}

auto operator<(StmInclude const &a, StmInclude const &b) -> bool {
    return std::tie(a.value_, a.type_) < std::tie(b.value_, b.type_);
}

auto operator==(StmProgram const &a, StmProgram const &b) -> bool {
    return std::tie(a.name_, a.args_) == std::tie(b.name_, b.args_);
}

auto operator<(StmProgram const &a, StmProgram const &b) -> bool {
    return std::tie(a.name_, a.args_) < std::tie(b.name_, b.args_);
}

auto operator==(StmConst const &a, StmConst const &b) -> bool {
    return std::tie(a.name_, a.value_, a.type_) == std::tie(b.name_, b.value_, b.type_);
}

auto operator<(StmConst const &a, StmConst const &b) -> bool {
    return std::tie(a.name_, a.value_, a.type_) < std::tie(b.name_, b.value_, b.type_);
}

auto operator==(StmComment const &a, StmComment const &b) -> bool {
    return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
}

auto operator<(StmComment const &a, StmComment const &b) -> bool {
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
