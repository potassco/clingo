#pragma once

#include <gringo/ground/statement.hh>

#include <gringo/util/enum.hh>

namespace Gringo::Ground {

//! @addtogroup ground_program
//! @{

//! Captures statements dependening cyclically on each other.
class Component {
  public:
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

} // namespace Gringo::Ground
