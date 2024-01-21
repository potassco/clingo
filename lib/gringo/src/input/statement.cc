#include <gringo/input/statement.hh>

namespace Gringo::Input {

auto operator==(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool {
    return std::tie(a.op, a.prio, a.type) == std::tie(b.op, b.prio, b.type);
}

auto operator<(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool {
    return std::tie(a.op, a.prio, a.type) < std::tie(b.op, b.prio, b.type);
}

auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
    return std::tie(a.name, a.op_defs) == std::tie(b.name, b.op_defs);
}

auto operator<(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
    return std::tie(a.name, a.op_defs) < std::tie(b.name, b.op_defs);
}

auto operator==(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool {
    return std::tie(a.name, a.arity, a.term, a.type, a.rhs) == std::tie(b.name, b.arity, b.term, b.type, b.rhs);
}

auto operator<(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool {
    return std::tie(a.name, a.arity, a.term, a.type, a.rhs) < std::tie(b.name, b.arity, b.term, b.type, b.rhs);
}

auto operator==(TheoryDefinition const &a, TheoryDefinition const &b) -> bool {
    return std::tie(a.name, a.term_defs, a.atom_defs) == std::tie(b.name, b.term_defs, b.atom_defs);
}

auto operator<(TheoryDefinition const &a, TheoryDefinition const &b) -> bool {
    return std::tie(a.name, a.term_defs, a.atom_defs) < std::tie(b.name, b.term_defs, b.atom_defs);
}

auto operator==(Rule const &a, Rule const &b) -> bool { return std::tie(a.head, a.body) == std::tie(b.head, b.body); }

auto operator<(Rule const &a, Rule const &b) -> bool { return std::tie(a.head, a.body) < std::tie(b.head, b.body); }

auto operator==(StatementOptimize::Tuple const &a, StatementOptimize::Tuple const &b) -> bool {
    return std::tie(a.weight, a.terms, a.priority) == std::tie(b.weight, b.terms, b.priority);
}

auto operator<(StatementOptimize::Tuple const &a, StatementOptimize::Tuple const &b) -> bool {
    return std::tie(a.weight, a.terms, a.priority) < std::tie(b.weight, b.terms, b.priority);
}

auto operator==(StatementOptimize const &a, StatementOptimize const &b) -> bool {
    return std::tie(a.type, a.elems) == std::tie(b.type, b.elems);
}

auto operator<(StatementOptimize const &a, StatementOptimize const &b) -> bool {
    return std::tie(a.type, a.elems) < std::tie(b.type, b.elems);
}

auto operator==(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool {
    return std::tie(a.body, a.tuple) == std::tie(b.body, b.tuple);
}

auto operator<(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool {
    return std::tie(a.body, a.tuple) < std::tie(b.body, b.tuple);
}

auto operator==(StatementShow const &a, StatementShow const &b) -> bool {
    return std::tie(a.term, a.body) == std::tie(b.term, b.body);
}

auto operator<(StatementShow const &a, StatementShow const &b) -> bool {
    return std::tie(a.term, a.body) < std::tie(b.term, b.body);
}

auto operator==(StatementShowSig const &a, StatementShowSig const &b) -> bool {
    return std::tie(a.name, a.arity, a.has_sign) == std::tie(b.name, b.arity, b.has_sign);
}

auto operator<(StatementShowSig const &a, StatementShowSig const &b) -> bool {
    return std::tie(a.name, a.arity, a.has_sign) < std::tie(b.name, b.arity, b.has_sign);
}

auto operator==(StatementProject const &a, StatementProject const &b) -> bool {
    return std::tie(a.term, a.body) == std::tie(b.term, b.body);
}

auto operator<(StatementProject const &a, StatementProject const &b) -> bool {
    return std::tie(a.term, a.body) < std::tie(b.term, b.body);
}

auto operator==(StatementProjectSig const &a, StatementProjectSig const &b) -> bool {
    return std::tie(a.name, a.arity, a.has_sign) == std::tie(b.name, b.arity, b.has_sign);
}

auto operator<(StatementProjectSig const &a, StatementProjectSig const &b) -> bool {
    return std::tie(a.name, a.arity, a.has_sign) < std::tie(b.name, b.arity, b.has_sign);
}

auto operator==(StatementDefined const &a, StatementDefined const &b) -> bool {
    return std::tie(a.name, a.arity, a.has_sign) == std::tie(b.name, b.arity, b.has_sign);
}

auto operator<(StatementDefined const &a, StatementDefined const &b) -> bool {
    return std::tie(a.name, a.arity, a.has_sign) < std::tie(b.name, b.arity, b.has_sign);
}

auto operator==(StatementExternal const &a, StatementExternal const &b) -> bool {
    return std::tie(a.term, a.body, a.type) == std::tie(b.term, b.body, b.type);
}

auto operator<(StatementExternal const &a, StatementExternal const &b) -> bool {
    return std::tie(a.term, a.body, a.type) < std::tie(b.term, b.body, b.type);
}

auto operator==(StatementEdge::Edge const &a, StatementEdge::Edge const &b) -> bool {
    return std::tie(a.u, a.v) == std::tie(b.u, b.v);
}

auto operator<(StatementEdge::Edge const &a, StatementEdge::Edge const &b) -> bool {
    return std::tie(a.u, a.v) < std::tie(b.u, b.v);
}

auto operator==(StatementEdge const &a, StatementEdge const &b) -> bool {
    return std::tie(a.edges, a.body) == std::tie(b.edges, b.body);
}

auto operator<(StatementEdge const &a, StatementEdge const &b) -> bool {
    return std::tie(a.edges, a.body) < std::tie(b.edges, b.body);
}

auto operator==(StatementHeuristic const &a, StatementHeuristic const &b) -> bool {
    return std::tie(a.atom, a.body, a.mod, a.prio, a.type) == std::tie(b.atom, b.body, b.mod, b.prio, b.type);
}

auto operator<(StatementHeuristic const &a, StatementHeuristic const &b) -> bool {
    return std::tie(a.atom, a.body, a.mod, a.prio, a.type) < std::tie(b.atom, b.body, b.mod, b.prio, b.type);
}

auto operator==(StatementScript const &a, StatementScript const &b) -> bool {
    return std::tie(a.content, a.type) == std::tie(b.content, b.type);
}

auto operator<(StatementScript const &a, StatementScript const &b) -> bool {
    return std::tie(a.content, a.type) < std::tie(b.content, b.type);
}

auto operator==(StatementInclude const &a, StatementInclude const &b) -> bool {
    return std::tie(a.path, a.type) == std::tie(b.path, b.type);
}

auto operator<(StatementInclude const &a, StatementInclude const &b) -> bool {
    return std::tie(a.path, a.type) < std::tie(b.path, b.type);
}

auto operator==(StatementProgram const &a, StatementProgram const &b) -> bool {
    return std::tie(a.name, a.args) == std::tie(b.name, b.args);
}

auto operator<(StatementProgram const &a, StatementProgram const &b) -> bool {
    return std::tie(a.name, a.args) < std::tie(b.name, b.args);
}

auto operator==(StatementConst const &a, StatementConst const &b) -> bool {
    return std::tie(a.name, a.value, a.type) == std::tie(b.name, b.value, b.type);
}

auto operator<(StatementConst const &a, StatementConst const &b) -> bool {
    return std::tie(a.name, a.value, a.type) < std::tie(b.name, b.value, b.type);
}

auto operator==(Comment const &a, Comment const &b) -> bool {
    return std::tie(a.value, a.type) == std::tie(b.value, b.type);
}

auto operator<(Comment const &a, Comment const &b) -> bool {
    return std::tie(a.value, a.type) < std::tie(b.value, b.type);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::TheoryOpDefinition>::operator()(Gringo::Input::TheoryOpDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryOpDefinition), x.op, x.prio, x.type);
}

auto value_hasher<Gringo::Input::TheoryTermDefinition>::operator()(Gringo::Input::TheoryTermDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermDefinition), x.name, x.op_defs);
}

auto value_hasher<Gringo::Input::TheoryAtomDefinition>::operator()(Gringo::Input::TheoryAtomDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryAtomDefinition), x.name, x.arity, x.term, x.type,
                                    x.rhs);
}

auto value_hasher<Gringo::Input::TheoryDefinition>::operator()(Gringo::Input::TheoryDefinition const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryDefinition), x.name, x.term_defs, x.atom_defs);
}

auto value_hasher<Gringo::Input::Rule>::operator()(Gringo::Input::Rule const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Rule), x.head, x.body);
}

auto value_hasher<Gringo::Input::StatementOptimize::Tuple>::operator()(
    Gringo::Input::StatementOptimize::Tuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementOptimize::Tuple), x.weight, x.priority, x.terms);
}

auto value_hasher<Gringo::Input::StatementEdge::Edge>::operator()(Gringo::Input::StatementEdge::Edge const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementEdge::Edge), x.u, x.v);
}

auto value_hasher<Gringo::Input::StatementOptimize>::operator()(Gringo::Input::StatementOptimize const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementOptimize), x.elems, x.type);
}

auto value_hasher<Gringo::Input::StatementWeakConstraint>::operator()(
    Gringo::Input::StatementWeakConstraint const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementWeakConstraint), x.tuple, x.body);
}

auto value_hasher<Gringo::Input::StatementShow>::operator()(Gringo::Input::StatementShow const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementShow), x.term, x.body);
}

auto value_hasher<Gringo::Input::StatementShowSig>::operator()(Gringo::Input::StatementShowSig const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementShowSig), x.name, x.arity, x.has_sign);
}

auto value_hasher<Gringo::Input::StatementProject>::operator()(Gringo::Input::StatementProject const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementProject), x.term, x.body);
}

auto value_hasher<Gringo::Input::StatementProjectSig>::operator()(Gringo::Input::StatementProjectSig const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementProjectSig), x.name, x.arity, x.has_sign);
}

auto value_hasher<Gringo::Input::StatementDefined>::operator()(Gringo::Input::StatementDefined const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementDefined), x.name, x.arity, x.has_sign);
}

auto value_hasher<Gringo::Input::StatementExternal>::operator()(Gringo::Input::StatementExternal const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementExternal), x.term, x.body, x.type);
}

auto value_hasher<Gringo::Input::StatementEdge>::operator()(Gringo::Input::StatementEdge const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementEdge), x.edges, x.body);
}

auto value_hasher<Gringo::Input::StatementHeuristic>::operator()(Gringo::Input::StatementHeuristic const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementHeuristic), x.atom, x.body, x.mod, x.prio, x.type);
}

auto value_hasher<Gringo::Input::StatementScript>::operator()(Gringo::Input::StatementScript const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementScript), x.content, x.type);
}

auto value_hasher<Gringo::Input::StatementInclude>::operator()(Gringo::Input::StatementInclude const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementInclude), x.path, x.type);
}

auto value_hasher<Gringo::Input::StatementProgram>::operator()(Gringo::Input::StatementProgram const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementProgram), x.name, x.args);
}

auto value_hasher<Gringo::Input::StatementConst>::operator()(Gringo::Input::StatementConst const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::StatementConst), x.name, x.value, x.type);
}

auto value_hasher<Gringo::Input::Comment>::operator()(Gringo::Input::Comment const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Comment), x.value, x.type);
}

} // namespace Gringo::Util
