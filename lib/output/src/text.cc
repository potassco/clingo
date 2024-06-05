#include <gringo/output/text.hh>

#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

#include <sstream>

namespace Gringo::Output {

namespace {

class OutputSimple : public OutputLit {
  private:
    void do_cond_lit([[maybe_unused]] size_t uid) override { throw std::runtime_error("unsupported literal"); }
};

class OutputBody : public OutputLit {
  public:
    void start() {
        buf_.str({});
        if (delayed_.empty() || !delayed_.back().empty()) {
            delayed_.emplace_back();
        }
        has_body_ = false;
    }

    auto end() -> std::string_view {
        assert(delayed_.back().empty());
        return buf_.view();
    }

    auto delayed() -> bool { return !delayed_.back().empty(); }

    void delay() {
        if (!buf_.view().empty()) {
            std::string ret = buf_.str();
            buf_.str({});
            delayed_.back().emplace_back(std::move(ret));
        }
    }

    auto buf() -> std::ostringstream & { return buf_; }

    void prepend() { delayed_.back().insert(delayed_.back().begin(), buf_.str()); }

    [[nodiscard]] auto empty() const -> bool { return !has_body_; }

    void define(size_t index, std::string str) { defined_.emplace(index, std::move(str)); }

    void flush(std::ostream &out) {
        for (auto &delayed : delayed_) {
            for (auto &elem : delayed) {
                std::visit(
                    [&out, this]<class T>(T const &elem) {
                        if constexpr (Util::is_among_v<T, std::string>) {
                            out << elem;
                        } else if constexpr (Util::is_among_v<T, size_t>) {
                            auto it = defined_.find(elem);
                            if (it != defined_.end()) {
                                out << it.value();
                            } else {
                                out << "#true";
                            }
                        } else {
                        }
                    },
                    elem);
            }
        }
        delayed_.clear();
    }

  private:
    void sep() {
        if (has_body_) {
            buf_ << "; ";
        } else {
            has_body_ = true;
        }
    }
    void do_lit(Sign sign, SymbolRef sym) override {
        sep();
        buf_ << sign << sym;
    }
    void do_boolean(bool value) override {
        sep();
        buf_ << (value ? "#true" : "#false");
    }
    void do_cond_lit(size_t uid) override {
        sep();
        delay();
        delayed_.back().emplace_back(uid);
    }

    bool has_body_ = false;
    std::ostringstream buf_;
    Util::unordered_map<size_t, std::string> defined_;
    std::vector<std::vector<std::variant<std::string, size_t>>> delayed_;
};

class OutputCond : public OutputSimple {
  public:
    void start() {
        buf_.str({});
        has_lits_ = false;
    }

    auto end() -> std::string_view { return buf_.view(); }

  private:
    void sep() {
        if (has_lits_) {
            buf_ << ", ";
        } else {
            has_lits_ = true;
        }
    }
    void do_lit(Sign sign, SymbolRef sym) override {
        sep();
        buf_ << sign << sym;
    }
    void do_boolean(bool value) override {
        sep();
        buf_ << (value ? "#true" : "#false");
    }

    bool has_lits_ = false;
    std::ostringstream buf_;
};

class OutputText : public OutputStm {
  public:
    OutputText(std::ostream &out) : out_{&out} {};

  private:
    void do_fact(SymbolRef sym) override { *out_ << sym << ".\n"; }
    auto do_body() -> OutputLit & override {
        body_.start();
        return body_;
    }
    void do_rule(std::optional<SymbolRef> head) override {
        if (!body_.delayed()) {
            if (head) {
                *out_ << *head;
            }
            if (!body_.empty() || !head) {
                *out_ << " :- ";
            }
            *out_ << body_.end() << ".\n";
        } else {
            body_.buf() << ".\n";
            body_.delay();
            if (head) {
                body_.buf() << *head;
            }
            body_.buf() << " :- ";
            body_.prepend();
        }
    }
    auto do_cond() -> OutputLit & override {
        cond_.start();
        return cond_;
    }
    void do_cond_lit_premise(size_t lit_uid, size_t elem_uid) override {
        cond_lits_[lit_uid].emplace_back(elem_uid);
        cond_lit_elems_.emplace(std::make_pair(lit_uid, elem_uid), std::make_pair("#false", cond_.end()));
    }
    void do_cond_lit_conclusion(size_t lit_uid, size_t elem_uid) override {
        cond_lit_elems_[std::make_pair(lit_uid, elem_uid)].first = cond_.end();
    }
    auto do_uid() -> size_t override { return ++uids_; }

    void do_flush() override {
        for (auto const &[lit_index, elems] : cond_lits_) {
            body_.buf().str({});
            if (elems.empty()) {
                body_.buf() << "#true";
            }
            bool comma = false;
            for (auto elem_index : elems) {
                if (comma) {
                    body_.buf() << "; ";
                } else {
                    comma = true;
                }
                auto const &[conclusion, premise] = cond_lit_elems_[std::make_pair(lit_index, elem_index)];
                body_.buf() << conclusion << ": " << premise;
            }
            body_.define(lit_index, body_.buf().str());
        }
        cond_lit_elems_.clear();
        cond_lits_.clear();
        body_.flush(*out_);
    }

    std::ostream *out_;
    OutputBody body_;
    OutputCond cond_;
    Util::unordered_map<std::pair<size_t, size_t>, std::pair<std::string, std::string>> cond_lit_elems_;
    Util::unordered_map<size_t, std::vector<size_t>> cond_lits_;
    size_t uids_ = 0;
};

} // namespace

auto make_text_output(std::ostream &out) -> UOutputStm { return std::make_unique<OutputText>(out); }

} // namespace Gringo::Output
