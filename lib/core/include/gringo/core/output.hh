#pragma once

#include <gringo/core/core.hh>
#include <gringo/core/symbol.hh>

namespace Gringo {

class OutputLit {
  public:
    virtual ~OutputLit() = default;
    void lit(Sign sign, Symbol sym) { do_lit(sign, sym); }
    void boolean(bool value) { do_boolean(value); }
    void cond_lit(size_t uid) { do_cond_lit(uid); }
    void end() { do_end(); }

  private:
    virtual void do_lit(Sign sign, Symbol sym) = 0;
    virtual void do_boolean(bool value) = 0;
    virtual void do_cond_lit(size_t uid) = 0;
    virtual void do_end() = 0;
};

class OutputStm {
  public:
    virtual ~OutputStm() = default;
    void fact(Symbol sym) { do_fact(sym); }
    auto rule(std::optional<Symbol> head) -> OutputLit & { return do_rule(head); }
    auto cond_lit_premise(size_t index) -> OutputLit & { return do_cond_lit_premise(index); }
    auto cond_lit_conclusion(size_t index) -> OutputLit & { return do_cond_lit_conclusion(index); }

  private:
    virtual void do_fact(Symbol sym) = 0;
    virtual auto do_rule(std::optional<Symbol> head) -> OutputLit & = 0;
    virtual auto do_cond_lit_premise(size_t index) -> OutputLit & = 0;
    virtual auto do_cond_lit_conclusion(size_t index) -> OutputLit & = 0;
};

using UOutputStm = std::unique_ptr<OutputStm>;

} // namespace Gringo
