#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/term.hh>

namespace Gringo::Ground {

enum class Sign {
    none,
    once,
    twice,
};
auto operator<<(std::ostream &out, Sign sign) -> std::ostream &;

enum class VarSelectMode {
    depend = 1,
    provide = 2,
    all = 3,
};

class Lit {
  public:
    virtual ~Lit() = default;
    virtual void print(std::ostream &out) const = 0;
    virtual void vars(VariableSet &vars, VarSelectMode mode) const = 0;
    [[nodiscard]] virtual auto recursive() const -> bool { return false; }
    friend auto operator<<(std::ostream &out, Lit const &lit) -> std::ostream & {
        lit.print(out);
        return out;
    }
};
using ULit = std::unique_ptr<Lit>;
using ULitVec = std::vector<ULit>;

class LitSymbolic : public Lit {
  public:
    LitSymbolic(Sign sign, UTerm atom, size_t index) : sign_{sign}, atom_{std::move(atom)}, index_{index} {}
    void vars(VariableSet &vars, VarSelectMode mode) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto recursive() const -> bool override;

  private:
    Sign sign_;
    UTerm atom_;
    //! The index of the literal.
    //!
    //! Note that only recursive literals have indices.
    size_t index_;
};

} // namespace Gringo::Ground
