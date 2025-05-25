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
        using value_type = std::pair<Symbol, std::variant<int, double, Symbol>>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        iterator(TheoryAssignment const &assignment) : assignment_{&assignment} { advance(); }

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
            Detail::handle_error(assignment_->theory_->assignment_next(
                assignment_->theory_->self, assignment_->thread_id_, &init_, &index_, &has_value_));
            if (has_value_) {
                current_ = assignment_->at(index_);
            }
        }

        mutable value_type current_;
        TheoryAssignment const *assignment_ = nullptr;
        size_t index_ = 0;
        bool init_ = true;
        bool has_value_ = true;
    };
    static_assert(std::input_iterator<iterator>);
    static_assert(std::sentinel_for<sentinel, iterator>);

    explicit TheoryAssignment(clingo_theory_t const *theory, uint32_t thread_id)
        : theory_{theory}, thread_id_{thread_id} {}

    [[nodiscard]] auto lookup(Symbol const &sym) const -> std::optional<size_t> {
        bool found = false;
        size_t index = 0;
        theory_->lookup_symbol(theory_->self, c_cast(sym), &index, &found);
        return found ? std::optional{index} : std::nullopt;
    }

    [[nodiscard]] auto at(size_t index) const -> std::pair<Symbol, std::variant<int, double, Symbol>> {
        clingo_symbol_t symbol = 0;
        clingo_theory_value_t value;
        bool found = false;
        Detail::handle_error(theory_->assignment_get_value(theory_->self, thread_id_, index, &symbol, &value, &found));
        auto sym = Clingo::Symbol{symbol, false};
        if (!found) {
            throw std::out_of_range{"invalid index"};
        }
        switch (value.type) {
            case clingo_theory_value_type_int: {
                return {std::move(sym), value.int_number}; // NOLINT
                break;
            }
            case clingo_theory_value_type_double: {
                return {std::move(sym), value.double_number}; // NOLINT
                break;
            }
            case clingo_theory_value_type_symbol: {
                return {std::move(sym), Symbol{value.symbol, false}}; // NOLINT
                break;
            }
            default: {
                throw std::logic_error{"invalid symbol type"};
            }
        }
    }

    [[nodiscard]] auto begin() const -> iterator { return {*this}; }
    [[nodiscard]] static auto end() -> sentinel { return {}; }

  private:
    clingo_theory_t const *theory_;
    uint32_t thread_id_;
};

class Theory {
  public:
    Theory(Library const &lib, bool (*create)(clingo_lib_t *lib, clingo_theory_t *theory)) {
        create(c_cast(lib), &theory_);
    }
    Theory(Theory &&other) = delete;
    ~Theory() {
        if (theory_.destroy != nullptr) {
            theory_.destroy(theory_.self);
        }
    }

    void prepare(Control const &ctl) const { Detail::handle_error(theory_.prepare(theory_.self, c_cast(ctl))); }

    template <class F> void rewrite(AST::Node const &stm, F fun) const {
        constexpr auto add = [](clingo_ast_t *stm, void *data) -> bool {
            CLINGO_TRY {
                auto *fun = static_cast<F *>(data);
                std::invoke<F &>(*fun, AST::Node{stm, true});
            }
            CLINGO_CATCH;
        };
        Detail::handle_error(theory_.rewrite_ast(theory_.self, c_cast(stm), +add, static_cast<void *>(&fun)));
    }

    void rewrite(Library const &lib, Control const &ctl, std::string_view str) const {
        auto scanner = AST::Scanner{lib, str};
        auto prg = AST::Program{lib};
        for (auto const &stm : scanner) {
            rewrite(stm, [&](AST::Node const &node) { prg.add(node); });
        }
        ctl.join(prg);
    }

    void rewrite(Library const &lib, Control const &ctl, StringSpan files) const {
        auto scanner = AST::Scanner{lib, files};
        auto prg = AST::Program{lib};
        for (auto const &stm : scanner) {
            rewrite(stm, [&](AST::Node const &node) { prg.add(node); });
        }
        ctl.join(prg);
    }

    [[nodiscard]] auto assignment(uint32_t thread_id) const -> TheoryAssignment {
        return TheoryAssignment{&theory_, thread_id};
    }

    void stats(Stats step, [[maybe_unused]] Stats accu) const {
        Detail::handle_error(theory_.on_stats(theory_.self, c_cast(step)));
    }

    void model(Model model) const { Detail::handle_error(theory_.on_model(theory_.self, c_cast(model))); }

    void register_theory(Control const &ctl) const {
        Detail::handle_error(theory_.register_theory(theory_.self, c_cast(ctl)));
    }

    void register_options(Options const &opts) const {
        Detail::handle_error(theory_.register_options(theory_.self, c_cast(opts)));
    }

    void validate_options() const { Detail::handle_error(theory_.validate_options(theory_.self)); }

    void configure(std::string_view key, std::string_view value) const {
        Detail::handle_error(theory_.configure(theory_.self, key.data(), key.size(), value.data(), value.size()));
    }

  private:
    clingo_theory_t theory_{};
};

} // namespace Clingo
