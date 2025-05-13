#pragma once

#include <clingo/ground/statement.hh>

#include <clingo/util/enum.hh>

namespace CppClingo::Ground {

//! @addtogroup ground_stm
//! @{

//! Captures statements depending cyclically on each other.
class Component {
  public:
    //! Construct a component.
    //!
    //! If domain is true, then the component does not contain negative edges.
    Component(bool domain) : domain_{domain} {}
    //! Add a statement to a component.
    void add(UStm stm) { stms_.emplace_back(std::move(stm)); }
    //! Get the statements in the component.
    [[nodiscard]] auto stms() const -> UStmVec const & { return stms_; }
    //! Return true if the statements in this component only derive facts.
    [[nodiscard]] auto domain() const -> bool { return domain_; }

  private:
    UStmVec stms_;
    bool domain_;
};

//! @}

} // namespace CppClingo::Ground
