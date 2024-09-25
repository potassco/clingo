#include <gringo/output/text.hh>

#include <gringo/util/ordered_set.hh>
#include <gringo/util/print.hh>
#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

namespace Gringo::Output {

namespace {

class OutputSimple : public OutputLit {
  private:
    auto do_cond_lit([[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        throw std::runtime_error("unsupported literal");
    }

    auto do_bd_aggr([[maybe_unused]] Sign sign, [[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        throw std::runtime_error("unsupported literal");
    }
};

class OutputBody : public OutputLit {
  public:
    OutputBody(size_t &uids) : uids_{&uids} {}

    void start() {
        buf_.reset();
        if (delayed_.empty() || !delayed_.back().empty()) {
            delayed_.emplace_back();
        }
        has_body_ = false;
    }

    [[nodiscard]] auto end() -> std::string_view {
        assert(delayed_.back().empty());
        return buf_.view();
    }

    [[nodiscard]] auto delayed() -> bool { return !delayed_.back().empty(); }

    void delay() {
        if (!buf_.empty()) {
            delayed_.back().emplace_back(buf_.str());
            buf_.reset();
        }
    }

    [[nodiscard]] auto delay_head(std::optional<size_t> uid, char const *sep) -> size_t {
        bool fact = buf_.empty() && delayed_.back().empty();
        buf_ << ".\n";
        delayed_.back().emplace_back(buf_.str());
        buf_.reset();
        if (!uid) {
            uid = ++*uids_;
        }
        // Note: leaves room for optimization...
        if (!fact) {
            delayed_.back().emplace(delayed_.back().begin(), sep);
        }
        delayed_.back().emplace(delayed_.back().begin(), *uid);
        return *uid;
    }

    [[nodiscard]] auto buf() -> Util::OutputBuffer & { return buf_; }

    void prepend() { delayed_.back().insert(delayed_.back().begin(), buf_.str()); }

    [[nodiscard]] auto empty() const -> bool { return !has_body_; }

    void define(size_t index, std::string str) { defined_.emplace(index, std::move(str)); }

    void flush(Util::OutputBuffer &out) {
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
                                out << "#false";
                            }
                        }
                    },
                    elem);
                out.endl();
            }
        }
        delayed_.clear();
    }

    void end_step() {
        delayed_.clear();
        defined_.clear();
    }

  private:
    void sep() {
        if (has_body_) {
            buf_ << "; ";
        } else {
            has_body_ = true;
        }
    }

    void do_lit(Sign sign, Symbol sym) override {
        sep();
        buf_ << sign << sym;
    }

    void do_boolean(bool value) override {
        sep();
        buf_ << (value ? "#true" : "#false");
    }

    auto do_cond_lit(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = ++*uids_;
        }
        sep();
        delay();
        delayed_.back().emplace_back(*uid);
        return *uid;
    }

    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = ++*uids_;
        }
        sep();
        buf_ << sign;
        delay();
        delayed_.back().emplace_back(*uid);
        return *uid;
    }

    bool has_body_ = false;
    size_t *uids_;
    Util::OutputBuffer buf_;
    Util::unordered_map<size_t, std::string> defined_;
    std::vector<std::vector<std::variant<std::string, size_t>>> delayed_;
};

class OutputCond : public OutputSimple {
  public:
    void start() {
        buf_.reset();
        has_lits_ = false;
    }

    [[nodiscard]] auto end() -> std::string_view { return buf_.view(); }

  private:
    void sep() {
        if (has_lits_) {
            buf_ << ", ";
        } else {
            has_lits_ = true;
        }
    }

    void do_lit(Sign sign, Symbol sym) override {
        sep();
        buf_ << sign << sym;
    }

    void do_boolean(bool value) override {
        sep();
        buf_ << (value ? "#true" : "#false");
    }

    bool has_lits_ = false;
    Util::OutputBuffer buf_;
};

class OutputText : public OutputStm {
  public:
    OutputText(Util::OutputBuffer &out) : out_{&out} {};

  private:
    template <class T> static void simple_head_(T &out, std::optional<std::pair<Symbol, bool>> const &head) {
        if (head) {
            if (head->second) {
                out << "{ ";
            }
            out << head->first;
            if (head->second) {
                out << " }";
            }
        }
    }

    void do_fact(Symbol sym) override {
        *out_ << sym << ".\n";
        out_->endl();
    }

    [[nodiscard]] auto do_body() -> OutputLit & override {
        body_.start();
        return body_;
    }

    void do_rule(std::optional<std::pair<Symbol, bool>> head) override {
        if (!body_.delayed()) {
            simple_head_(*out_, head);
            if (!body_.empty() || !head) {
                *out_ << " :- ";
            }
            *out_ << body_.end() << ".\n";
            out_->endl();
        } else {
            body_.buf() << ".\n";
            body_.delay();
            simple_head_(body_.buf(), head);
            body_.buf() << " :- ";
            body_.prepend();
        }
    }

    auto do_aggr_rule(std::optional<size_t> uid) -> size_t override { return body_.delay_head(uid, " :- "); }

    auto do_cond() -> OutputLit & override {
        cond_.start();
        return cond_;
    }

    auto do_cond_id() -> size_t override {
        auto it = conds_.emplace(cond_.end()).first;
        return it - conds_.begin();
    }

    auto do_uid() -> size_t override { return ++uids_; }

    void do_cond_lit(size_t uid, CondLits elems) override {
        if (elems.empty()) {
            body_.define(uid, "#true");
        } else {
            tmp_.reset();
            tmp_ << Util::p_range(elems, "; ", [this](auto &buf, auto const &elem) {
                if (elem.first) {
                    buf << *conds_.nth(*elem.first);
                } else {
                    buf << "#false";
                }
                buf << ": " << *conds_.nth(elem.second);
            });
            body_.define(uid, tmp_.str());
        }
    }

    void do_bd_aggr(size_t uid, AggregateFunction fun, BdElems elems, Guards guards) override {
        tmp_.reset();
        auto it = guards.begin();
        if (guards.size() > 1) {
            tmp_ << it->second << " " << flip(it->first) << " ";
            ++it;
        }
        tmp_ << fun << " { " << Util::p_range(elems, "; ", [this](auto &buf, auto const &elem) {
            if (elem.second.empty()) {
                buf << Util::p_range(elem.first);
                if (elem.first.empty()) {
                    buf << ": ";
                }
            } else {
                buf << Util::p_range(elem.second, "; ", [this, &elem](auto &buf, auto const &cond) {
                    buf << Util::p_range(elem.first) << ": " << *conds_.nth(cond);
                });
            }
        }) << (elems.empty() ? "}" : " }");
        for (auto ie = guards.end(); it != ie; ++it) {
            tmp_ << " " << it->first << " " << it->second;
        }
        body_.define(uid, tmp_.str());
    }

    void do_hd_aggr(size_t uid, AggregateFunction fun, HdElems elems, Guards guards) override {
        // Note: too much c&p from bd_aggr
        tmp_.reset();
        auto it = guards.begin();
        if (guards.size() > 1) {
            tmp_ << it->second << " " << flip(it->first) << " ";
            ++it;
        }
        tmp_ << fun << " { " << Util::p_range(elems, "; ", [this](auto &buf, HdElem const &elem) {
            buf << Util::p_range(elem.second, "; ", [this, &elem](auto &buf, auto const &hc) {
                buf << Util::p_range(elem.first) << ": ";
                if (hc.first.type() == SymbolType::sup) {
                    buf << "#true";
                } else {
                    buf << hc.first;
                }
                buf << *conds_.nth(hc.second);
            });
        }) << (elems.empty() ? "}" : " }");
        for (auto ie = guards.end(); it != ie; ++it) {
            tmp_ << " " << it->first << " " << it->second;
        }
        body_.define(uid, tmp_.str());
    }

    void do_flush() override { body_.flush(*out_); }

    void do_end_step() override { out_->flush(); }

    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}

    Util::OutputBuffer *out_;
    Util::OutputBuffer tmp_;
    OutputBody body_{uids_};
    OutputCond cond_;
    Util::ordered_set<std::string> conds_;
    size_t uids_ = 0;
};

} // namespace

auto make_text_output(Util::OutputBuffer &out) -> UOutputStm { return std::make_unique<OutputText>(out); }

} // namespace Gringo::Output
