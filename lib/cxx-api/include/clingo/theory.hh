#pragma once

#include <clingo/app.hh>
#include <clingo/ast.hh>

#include <clingo/theory.h>

#include <iterator>
#include <variant>

namespace Clingo {

//! @addtogroup cpp_theory
//!
//! This module defines a well-specified C interface that must be implemented
//! by external theory plugins. The interface is designed to allow efficient
//! interoperation between the core solver and external theory implementations.
//! It facilitates integration across language barriers (for example, bridging
//! C++ and Python) and across modular boundaries within the system.
//!
//! Among its responsibilities, the theory interface includes: Providing
//! essential theory callbacks (e.g., rewriting ASTs, model handling, preparing
//! theory data, and statistics collection). Defining the lifecycle of a theory
//! object, including optional cleanup via a destroy callback.
//! @{

//! Class to represent a theory assignment.
class TheoryAssignment {
  public:
    //! Sentinel type for the end of the theory assignment.
    struct sentinel {};
    //! Iterator type for the theory assignment.
    class iterator {
      public:
        //! The iterator category.
        using iterator_category = std::input_iterator_tag;
        //! The value type of the iterator, which is a pair of a symbol and its value.
        using value_type = std::pair<Symbol, std::variant<int, double, Symbol>>;
        //! The difference type of the iterator.
        using difference_type = std::ptrdiff_t;
        //! The pointer type of the iterator.
        using pointer = value_type *;
        //! The reference type of the iterator.
        using reference = value_type &;

        //! Constructor for the iterator.
        //!
        //! @param assignment the theory assignment to iterate over
        iterator(TheoryAssignment const &assignment) : assignment_{&assignment} { advance(); }

        //! The default constructor for the iterator.
        //!
        //! For interface completeness, should not be used.
        iterator() = default;

        //! Get the current value of the iterator.
        //!
        //! @return the current value of the iterator
        auto operator*() const -> reference { return current_; }

        //! Increment the iterator to the next value.
        //!
        //! @return a reference to self
        auto operator++() -> iterator & {
            advance();
            return *this;
        }

        //! Increment the iterator to the next value (postfix).
        auto operator++(int) -> iterator { return std::exchange(*this, ++iterator(*this)); }

        //! Check whether their are still values in the assignment.
        //!
        //! @param a the iterator to compare
        //! @param b the sentinel
        //! @return whether the iterator is equal to the sentinel
        friend auto operator==(iterator const &a, [[maybe_unused]] sentinel const &b) -> bool { return !a.has_value_; }

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

    //! Construct a theory assignment from its C representation and a thread id.
    //!
    //! @param theory the C theory representation
    //! @param thread_id the thread id for which to get the assignment
    explicit TheoryAssignment(clingo_theory_t const *theory, uint32_t thread_id)
        : theory_{theory}, thread_id_{thread_id} {}

    //! Lookup a symbol in the theory assignment.
    //!
    //! @param sym the symbol to look up
    //! @return the index of the symbol if found, or std::nullopt if not found
    [[nodiscard]] auto lookup(Symbol const &sym) const -> std::optional<size_t> {
        bool found = false;
        size_t index = 0;
        theory_->lookup_symbol(theory_->self, c_cast(sym), &index, &found);
        return found ? std::optional{index} : std::nullopt;
    }

    //! Get the symbol value pair at the given index in the theory assignment.
    //!
    //! @param index the index of the value to get
    //! @return the symbol and its value at the given index
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

    //! Get an iterator pointing to the first element of the theory assignment.
    //!
    //! @return an iterator to the first element
    [[nodiscard]] auto begin() const -> iterator { return {*this}; }
    //! Get sentinel marking the end of the theory assignment.
    //!
    //! @return a sentinel marking the end of the theory assignment
    [[nodiscard]] static auto end() -> sentinel { return {}; }

  private:
    clingo_theory_t const *theory_;
    uint32_t thread_id_;
};

//! Theory class to represent a theory in Clingo.
class Theory {
  public:
    //! Constructor for the Theory class.
    //!
    //! @param lib the library to store symbols
    //! @param create the function to create the theory
    Theory(Library const &lib, bool (*create)(clingo_lib_t *lib, clingo_theory_t *theory)) {
        create(c_cast(lib), &theory_);
    }

    //! Get the underlying C representation of the theory.
    //! @param x the theory to cast
    //! @return the C representation of the theory
    friend auto c_cast(Theory const &x) -> clingo_theory_t const * { return &x.theory_; }

    //! Disable copy and move operations for the Theory class.
    Theory(Theory &&other) = delete;

    //! Destructor for the Theory class.
    ~Theory() {
        if (theory_.destroy != nullptr) {
            theory_.destroy(theory_.self);
        }
    }

    //! Prepare the theory for solving.
    //!
    //! Must be called before solve calls.
    //!
    //! @param ctl the control object to prepare the theory with
    void prepare(Control const &ctl) const { Detail::handle_error(theory_.prepare(theory_.self, c_cast(ctl))); }

    //! Rewrite the given AST node using the theory's rewrite function.
    //!
    //! Rewritten asts are passed to the given function.
    //!
    //! @param stm the AST node to rewrite
    //! @param fun the function to call with the rewritten AST nodes
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

    //! Rewrite the given program using the theory's rewrite function.
    //!
    //! @param lib the library to store symbols
    //! @param ctl the control object to add statements to
    //! @param str the string to parse and rewrite
    void rewrite(Library const &lib, Control const &ctl, std::string_view str) const {
        auto prg = AST::Program{lib};
        AST::parse(
            lib, str, [&](AST::Node const &stm) { rewrite(stm, [&](AST::Node const &stm) { prg.add(stm); }); }, &ctl);
        ctl.join(prg);
    }

    //! Rewrite the programs in the files using the theory's rewrite function.
    //!
    //! @param lib the library to store symbols
    //! @param ctl the control object to add statements to
    //! @param files the files to parse and rewrite
    void rewrite(Library const &lib, Control const &ctl, StringSpan files) const {
        auto prg = AST::Program{lib};
        AST::parse(
            lib, files, [&](AST::Node const &stm) { rewrite(stm, [&](AST::Node const &stm) { prg.add(stm); }); }, &ctl);
        ctl.join(prg);
    }

    //! Get the theory assignment for the given thread id.
    //!
    //! @param thread_id the thread id for which to get the assignment
    //! @return the theory assignment for the given thread id
    [[nodiscard]] auto assignment(uint32_t thread_id) const -> TheoryAssignment {
        return TheoryAssignment{&theory_, thread_id};
    }

    //! Incorporate statistics from the theory into the solver's statistics.
    //!
    //! @param step the current step of the solver
    //! @param accu the accumulated statistics
    void stats(Stats step, [[maybe_unused]] Stats accu) const {
        Detail::handle_error(theory_.on_stats(theory_.self, c_cast(step)));
    }

    //! Incorporate theory symbols into the model.
    //! @param model the model to extend with theory symbols
    void model(Model model) const { Detail::handle_error(theory_.on_model(theory_.self, c_cast(model))); }

    //! Register the theory with the control object.
    //!
    //! This gives a theory the possibility to register propagators. Hence, it
    //! must be called before grounding and solving.
    //!
    //! @param ctl the control object to register the theory with
    void register_theory(Control const &ctl) const {
        Detail::handle_error(theory_.register_theory(theory_.self, c_cast(ctl)));
    }

    //! Add theory related options to the given options object.
    //!
    //! Should be called from the Clingo::App::register_options() method.
    //!
    //! @param opts the options object to register the theory options with
    void register_options(Options const &opts) const {
        Detail::handle_error(theory_.register_options(theory_.self, c_cast(opts)));
    }

    //! Validate previously parsed the theory options.
    //!
    //! Should be called from the Clingo::App::validate_options() method.
    void validate_options() const { Detail::handle_error(theory_.validate_options(theory_.self)); }

    //! Configure the theory with the given key and value.
    //!
    //! It is theory dependent at which point this function can be called. Some
    //! theories might require configuration before theory registration, while
    //! others might allow configuration at any time.
    //!
    //! @param key the key to configure
    //! @param value the value to configure
    void configure(std::string_view key, std::string_view value) const {
        Detail::handle_error(theory_.configure(theory_.self, key.data(), key.size(), value.data(), value.size()));
    }

  private:
    clingo_theory_t theory_{};
};

//! @}

} // namespace Clingo
