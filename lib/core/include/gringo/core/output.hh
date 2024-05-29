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

  private:
    virtual void do_lit(Sign sign, Symbol sym) = 0;
    virtual void do_boolean(bool value) = 0;
    virtual void do_cond_lit(size_t uid) = 0;
};

class OutputStm {
  public:
    virtual ~OutputStm() = default;
    auto uid() -> size_t { return do_uid(); }

    void fact(Symbol sym) { do_fact(sym); }

    auto body() -> OutputLit & { return do_body(); }
    void rule(std::optional<Symbol> head) { do_rule(head); }

    auto cond() -> OutputLit & { return do_cond(); }
    void cond_lit_premise(size_t lit_uid, size_t elem_uid) { do_cond_lit_premise(lit_uid, elem_uid); }
    void cond_lit_conclusion(size_t lit_uid, size_t elem_uid) { do_cond_lit_conclusion(lit_uid, elem_uid); }

    void flush() { do_flush(); }

  private:
    virtual auto do_uid() -> size_t = 0;

    virtual void do_fact(Symbol sym) = 0;

    virtual auto do_body() -> OutputLit & = 0;
    virtual void do_rule(std::optional<Symbol> head) = 0;

    virtual auto do_cond() -> OutputLit & = 0;
    virtual void do_cond_lit_premise(size_t lit_uid, size_t elem_uid) = 0;
    virtual void do_cond_lit_conclusion(size_t lit_uid, size_t elem_uid) = 0;

    virtual void do_flush() = 0;
};

using UOutputStm = std::unique_ptr<OutputStm>;

} // namespace Gringo
