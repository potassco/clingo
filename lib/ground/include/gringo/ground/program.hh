#pragma once

#include <gringo/ground/statement.hh>

#include <gringo/util/enum.hh>

namespace Gringo::Ground {

//! The type of a component.
enum class ComponentType : uint8_t {
    domain = 1,      //!< The component evaluates to facts.
    single_pass = 2, //!< The component can be grounded in one pass.
};
consteval void is_bit_set_enum(ComponentType flags);

class Component {
  public:
    // TODO: eta-rules
    // TODO: epsilon-rules (might be merged with the above)
    // TODO: alpha-rules
    // TODO: info about aggregates
    Component(ComponentType type) : type_{type} {}
    void add(UStm stm) { stms_.emplace_back(std::move(stm)); }
    [[nodiscard]] auto stms() const -> UStmVec const & { return stms_; }
    [[nodiscard]] auto type() const -> ComponentType { return type_; }

  private:
    UStmVec stms_;
    ComponentType type_;
};

} // namespace Gringo::Ground
