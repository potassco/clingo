#pragma once

#include <gringo/input/program.hh>

#include <gringo/util/enum.hh>

namespace Gringo::Input {

/*
class AtomNode {
public:
    AtomNode(Term const *term) : term_{term} {
    }
private:
    Term const *term_;
};

class StmNode {
public:
    StmNode(Stm const &stm) : stm_{&stm} { }
private:
    Stm const *stm_;
    // should be ordered/index by signature for faster lookup
    std::vector<AtomNode> provide_;
    std::vector<std::pair<AtomNode, bool>> depend_;
};
*/

// examples:
// - h :- a : b, not c; d; not e.
//   - depend:
//     - positive: a, d
//     - negative: b, c, e
//   - provide: h
// - h :- { a; b } >= 1.
//   - depend:
//     - positive: a, b
//   - provide: h
// - h :- { a; b } <= 1.
//   - depend:
//     - negative: a, b
//   - provide: h
// - h :- { a; b } != 1.
//   - depend:
//     - positive: a, b
//     - negative: a, b
//   - provide: h
// - {h} :- a.
//   - depend:
//     - positive: a
//     - negative: h
//   - provide: h
// - g | h :- a.
//   - depend:
//     - positive: a
//     - negative: g, h
//   - provide: g, h

// graph:
// - n: number of statements
// - m: number of atoms in depneds
// - nodes: [0,n) + [n,n+m)
// - statement depends on atoms occurring in it (either positively or negatively)
// - component graph uses both dependencies
// - refined component graph only considers positive ones

//! The type of a component.
enum class ComponentType : uint32_t {
    domain = 1,     //!< The component evaluates to facts.
    stratified = 2, //!< The component can be grounded in one pass.
};
consteval void is_bit_set_enum(ComponentType flags);

//! A refined component.
//!
//! A component consists of a (non-empty) set of statements and a set of incomplete literals.
//! Instances of incomplete literals are added while grounding a component.
//! In case of negative literals, instances might also be added after grounding the component.
//! We cannot assume that an instance of a incomplete negative literal is true
//! if there has been no instance deriving its positive counterpart previously.
struct Component {
    //! The statements in the component.
    std::vector<Stm const *> stms;
    //! This vector captures literals that are not yet complete.
    std::vector<std::pair<Term const *, bool>> incomplete;
    //! The type of the componnent.
    //!
    //! It seems like this information won't be necessary for grounding.
    //! On-the-fly propagation taking into account previously seen rule instances should suffice.
    //! The type might still be interesting for diagnostics.
    ComponentType type;
};

using Components = std::vector<std::vector<Component>>;

auto analyze(std::vector<Stm> const &stms) -> Components;

} // namespace Gringo::Input
