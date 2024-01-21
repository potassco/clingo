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

} // namespace Gringo::Util
