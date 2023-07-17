#pragma once

#include <input/literal.hh>
#include <input/term.hh>

namespace Gringo::Input {

struct TheoryTermSymbol;
struct TheoryTermVariable;
struct TheoryTermTuple;
struct TheoryTermFunction;
struct TheoryTermUnparsed;

using TheoryTerm =
    std::variant<TheoryTermSymbol, TheoryTermVariable, TheoryTermTuple, TheoryTermFunction, TheoryTermUnparsed>;
using TheoryTermVec = std::vector<TheoryTerm>;

enum class TheoryTermTupleType { tuple, set, list };

struct TheoryTermSymbol {
    explicit TheoryTermSymbol(Symbol value) : value{std::move(value)} {}

    Symbol value;
};

struct TheoryTermVariable {
    explicit TheoryTermVariable(std::string value, bool is_anonymous = false)
        : name{std::move(value)}, is_anonymous{is_anonymous} {}

    std::string name;
    bool is_anonymous;
};

struct TheoryTermTuple {
    explicit TheoryTermTuple(TheoryTermTupleType type, TheoryTermVec elems);

    TheoryTermTupleType type;
    TheoryTermVec elems;
};

struct TheoryTermFunction {
  public:
    explicit TheoryTermFunction(std::string name);
    explicit TheoryTermFunction(std::string name, TheoryTermVec args);

    std::string name;
    TheoryTermVec args;
};

struct TheoryTermUnparsed {
    using OpVec = std::vector<std::string>;
    using Element = std::pair<OpVec, TheoryTerm>;
    using ElementVec = std::vector<Element>;

    explicit TheoryTermUnparsed(ElementVec elems);

    ElementVec elems;
};

inline TheoryTermTuple::TheoryTermTuple(TheoryTermTupleType type, TheoryTermVec elems)
    : type(type), elems{std::move(elems)} {}
inline TheoryTermFunction::TheoryTermFunction(std::string name) : TheoryTermFunction{std::move(name), {}} {}
inline TheoryTermFunction::TheoryTermFunction(std::string name, TheoryTermVec args)
    : name(std::move(name)), args{std::move(args)} {}
inline TheoryTermUnparsed::TheoryTermUnparsed(ElementVec elems) : elems{std::move(elems)} {}

struct TheoryAtom {
    using RGuard = std::optional<std::pair<std::string, TheoryTerm>>;
    using Element = std::pair<TheoryTermVec, LiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit TheoryAtom(Term name, ElementVec elems, RGuard rhs)
        : name{std::move(name)}, elems{std::move(elems)}, rhs{std::move(rhs)} {}
    explicit TheoryAtom(Term name, ElementVec elems) : TheoryAtom{std::move(name), std::move(elems), std::nullopt} {}
    explicit TheoryAtom(Term name, ElementVec elems, std::string rhs_op, TheoryTerm rhs_term)
        : TheoryAtom{std::move(name), std::move(elems), std::make_pair(std::move(rhs_op), std::move(rhs_term))} {}

    Term name;
    ElementVec elems;
    RGuard rhs;
};

} // namespace Gringo::Input
