#include <gringo/output/text.hh>

namespace Gringo::Output {

namespace {

class OutputSimple : public OutputLit {
  public:
    OutputSimple(std::ostream &out) : out_{&out} {};

    void sep() { do_sep(); }
    auto out() -> std::ostream & { return *out_; }

  private:
    virtual void do_sep() = 0;
    void do_lit(Sign sign, Symbol sym) override {
        sep();
        *out_ << sign << " " << sym;
    }
    void do_cond_lit([[maybe_unused]] size_t uid) override { throw std::runtime_error("unsupported literal"); }
    void do_boolean(bool value) override {
        sep();
        *out_ << (value ? "#true" : "#false");
    }

    std::ostream *out_;
};

class OutputRule : public OutputSimple {
  public:
    OutputRule(std::ostream &out) : OutputSimple{out} {};

    void start(std::optional<Symbol> head) {
        has_body_ = false;
        has_head_ = head.has_value();
        if (has_head_) {
            out() << *head;
        }
    }

  private:
    void do_sep() override {
        if (has_body_) {
            out() << "; ";
        } else {
            out() << " :- ";
            has_body_ = true;
        }
    }
    void do_end() override {
        if (!has_body_ && !has_head_) {
            out() << " :- ";
        }
        out() << ".\n";
    }
    void do_cond_lit(size_t uid) override {
        sep();
        out() << "#cond_lit(TODO: " << uid << ")";
    }

    bool has_head_ = false;
    bool has_body_ = false;
};

class OutputCondLitPremise : public OutputSimple {
  public:
    OutputCondLitPremise(std::ostream &out) : OutputSimple{out} {};

  private:
    void do_sep() override {}
    void do_lit(Sign sign, Symbol sym) override { out() << "% premise: " << sign << " " << sym << "\n"; }
    void do_boolean(bool value) override { out() << "% premise: " << (value ? "#true" : "#false") << "\n"; }
    void do_end() override {}
};

class OutputCondLitConclusion : public OutputSimple {
  public:
    OutputCondLitConclusion(std::ostream &out) : OutputSimple{out} {};

  private:
    void do_sep() override {}
    void do_lit(Sign sign, Symbol sym) override { out() << "%   conclusion: " << sign << " " << sym << "\n"; }
    void do_boolean(bool value) override { out() << "%   conclusion: " << (value ? "#true" : "#false") << "\n"; }
    void do_end() override {}
};

class OutputText : public OutputStm {
  public:
    OutputText(std::ostream &out) : out_{&out} {};

  private:
    void do_fact(Symbol sym) override { *out_ << sym << ".\n"; }
    auto do_rule(std::optional<Symbol> head) -> OutputLit & override {
        rule_.start(head);
        return rule_;
    }
    auto do_cond_lit_premise(size_t index) -> OutputLit & override {
        static_cast<void>(index);
        return cond_lit_premise_;
    }
    auto do_cond_lit_conclusion(size_t index) -> OutputLit & override {
        static_cast<void>(index);
        return cond_lit_conclusion_;
    }

    std::ostream *out_;
    OutputRule rule_{*out_};
    OutputCondLitPremise cond_lit_premise_{*out_};
    OutputCondLitConclusion cond_lit_conclusion_{*out_};
};

} // namespace

auto make_text_output(std::ostream &out) -> UOutputStm { return std::make_unique<OutputText>(out); }

} // namespace Gringo::Output
