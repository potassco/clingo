#pragma once

#include <clingo/app.hh>
#include <clingo/ast.hh>

#include <clingo/theory.h>

#include <iterator>
#include <variant>

namespace Clingo {

class TheoryAssignment {
  public:
    struct sentinel {};
    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<Clingo::Symbol, std::variant<int, double, Clingo::Symbol>>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        iterator(clingo_theory_t *theory, uint32_t thread_id) : theory_{theory}, thread_id_{thread_id} { advance(); }

        iterator() = default;

        auto operator*() const -> reference { return current_; }

        auto operator++() -> iterator & {
            advance();
            return *this;
        }

        auto operator++(int) -> iterator { return std::exchange(*this, ++iterator(*this)); }

        auto operator==([[maybe_unused]] sentinel const &other) const -> bool { return !has_value_; }

      private:
        void advance() {
            Clingo::Detail::handle_error(
                theory_->assignment_next(theory_->self, thread_id_, &init_, &index_, &has_value_));
            if (has_value_) {
                clingo_symbol_t symbol = 0;
                clingo_theory_value_t value;
                Clingo::Detail::handle_error(
                    theory_->assignment_get_value(theory_->self, thread_id_, index_, &symbol, &value, nullptr));
                current_.first = Clingo::Symbol{symbol, false};
                switch (value.type) {
                    case clingo_theory_value_type_int: {
                        current_.second = value.int_number; // NOLINT
                        break;
                    }
                    case clingo_theory_value_type_double: {
                        current_.second = value.double_number; // NOLINT
                        break;
                    }
                    case clingo_theory_value_type_symbol: {
                        current_.second = Clingo::Symbol{value.symbol, false}; // NOLINT
                        break;
                    }
                    default: {
                        throw std::logic_error{"invalid symbol type"};
                    }
                }
            }
        }

        mutable value_type current_;
        clingo_theory_t *theory_ = nullptr;
        size_t index_ = 0;
        uint32_t thread_id_ = 0;
        bool init_ = true;
        bool has_value_ = true;
    };
    static_assert(std::input_iterator<iterator>);
    static_assert(std::sentinel_for<sentinel, iterator>);

    explicit TheoryAssignment(clingo_theory_t *theory, uint32_t thread_id) : theory_{theory}, thread_id_{thread_id} {}

    [[nodiscard]] auto begin() const -> iterator { return {theory_, thread_id_}; }
    [[nodiscard]] static auto end() -> sentinel { return {}; }

  private:
    clingo_theory_t *theory_;
    uint32_t thread_id_;
};

class Theory {
  public:
    Theory(Clingo::Library const &lib, bool (*create)(clingo_lib_t *lib, clingo_theory_t *theory)) {
        create(c_cast(lib), &theory_);
    }
    Theory(Theory &&other) = delete;
    ~Theory() {
        if (theory_.destroy != nullptr) {
            theory_.destroy(theory_.self);
        }
    }

    void prepare(Clingo::Control const &ctl) const {
        Clingo::Detail::handle_error(theory_.prepare(theory_.self, c_cast(ctl)));
    }

    template <class F> void rewrite(Clingo::AST::Node const &stm, F fun) const {
        constexpr auto add = [](clingo_ast_t *stm, void *data) -> bool {
            CLINGO_TRY {
                auto *fun = static_cast<F *>(data);
                std::invoke<F &>(*fun, Clingo::AST::Node{stm, true});
            }
            CLINGO_CATCH;
        };
        Clingo::Detail::handle_error(theory_.rewrite_ast(theory_.self, c_cast(stm), +add, static_cast<void *>(&fun)));
    }

    void rewrite(Clingo::Library const &lib, Clingo::Control const &ctl, std::string_view str) const {
        auto scanner = Clingo::AST::Scanner{lib, str};
        auto prg = Clingo::AST::Program{lib};
        for (auto const &stm : scanner) {
            rewrite(stm, [&](Clingo::AST::Node const &node) { prg.add(node); });
        }
        ctl.join(prg);
    }

    void rewrite(Clingo::Library const &lib, Clingo::Control const &ctl, Clingo::StringSpan files) const {
        auto scanner = Clingo::AST::Scanner{lib, files};
        auto prg = Clingo::AST::Program{lib};
        for (auto const &stm : scanner) {
            rewrite(stm, [&](Clingo::AST::Node const &node) { prg.add(node); });
        }
        ctl.join(prg);
    }

    auto assignment(uint32_t thread_id) -> TheoryAssignment { return TheoryAssignment{&theory_, thread_id}; }

    void stats(Clingo::Stats step, [[maybe_unused]] Clingo::Stats accu) const {
        Clingo::Detail::handle_error(theory_.on_stats(theory_.self, c_cast(step)));
    }

    void register_theory(Clingo::Control const &ctl) const {
        Clingo::Detail::handle_error(theory_.register_theory(theory_.self, c_cast(ctl)));
    }

    void configure(std::string_view key, std::string_view value) const {
        Clingo::Detail::handle_error(
            theory_.configure(theory_.self, key.data(), key.size(), value.data(), value.size()));
    }

  private:
    clingo_theory_t theory_{};
};

} // namespace Clingo
