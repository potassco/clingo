#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>

#include <clingo/propagate.h>

namespace Clingo {

class Trail {
  public:
    using value_type = clingo_literal_t;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<Trail>;

    explicit Trail(clingo_assignment_t const *assignment) : assignment_{assignment} {}

    [[nodiscard]] auto operator[](size_t index) const -> clingo_literal_t {
        clingo_literal_t lit = 0;
        Detail::handle_error(clingo_assignment_trail_at(assignment_, index, &lit));
        return lit;
    }

    [[nodiscard]] auto size() const -> size_t {
        uint32_t size = 0;
        Detail::handle_error(clingo_assignment_trail_size(assignment_, &size));
        return size;
    }

    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

    [[nodiscard]] auto begin(uint32_t level) const -> iterator {
        uint32_t offset = 0;
        Detail::handle_error(clingo_assignment_trail_begin(assignment_, level, &offset));
        return iterator{*this, level};
    }

    [[nodiscard]] auto end(uint32_t level) const -> iterator {
        uint32_t offset = 0;
        Detail::handle_error(clingo_assignment_trail_end(assignment_, level, &offset));
        return iterator{*this, level};
    }

    [[nodiscard]] auto level(uint32_t level) const { return std::ranges::subrange{begin(level), end(level)}; }

  private:
    clingo_assignment_t const *assignment_;
};

class Assignment {
  public:
    using value_type = clingo_literal_t;

    explicit Assignment(clingo_assignment_t const *assignment) : assignment_(assignment) {}

    [[nodiscard]] auto size() const -> size_t {
        size_t size = 0;
        Detail::handle_error(clingo_assignment_size(assignment_, &size));
        return size;
    }

    [[nodiscard]] auto operator[](size_t size) const -> clingo_literal_t {
        clingo_literal_t lit = 0;
        Detail::handle_error(clingo_assignment_at(assignment_, size, &lit));
        return lit;
    }

    [[nodiscard]] auto decision(uint32_t level) const -> clingo_literal_t {
        clingo_literal_t lit = 0;
        Detail::handle_error(clingo_assignment_decision(assignment_, level, &lit));
        return lit;
    }

    [[nodiscard]] auto decision_level() const -> uint32_t {
        uint32_t level = 0;
        Detail::handle_error(clingo_assignment_decision_level(assignment_, &level));
        return level;
    }

    [[nodiscard]] auto has_conflict() const -> bool {
        auto res = false;
        Detail::handle_error(clingo_assignment_has_conflict(assignment_, &res));
        return res;
    }

    [[nodiscard]] auto contains(clingo_literal_t lit) const -> bool {
        auto res = false;
        Detail::handle_error(clingo_assignment_has_literal(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto is_false(clingo_literal_t lit) const -> bool {
        auto res = false;
        Detail::handle_error(clingo_assignment_is_false(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto is_fixed(clingo_literal_t lit) const -> bool {
        auto res = false;
        Detail::handle_error(clingo_assignment_is_fixed(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto is_free(clingo_literal_t lit) const -> bool {
        clingo_truth_value_t res = 0;
        Detail::handle_error(clingo_assignment_truth_value(assignment_, lit, &res));
        return res == clingo_truth_value_free;
    }

    [[nodiscard]] auto is_total() const -> bool {
        auto res = false;
        Detail::handle_error(clingo_assignment_is_total(assignment_, &res));
        return res;
    }

    [[nodiscard]] auto is_true(clingo_literal_t lit) const -> bool {
        auto res = false;
        Detail::handle_error(clingo_assignment_is_true(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto level(clingo_literal_t lit) const -> uint32_t {
        uint32_t level = 0;
        Detail::handle_error(clingo_assignment_level(assignment_, lit, &level));
        return level;
    }

    [[nodiscard]] auto value(clingo_literal_t lit) const -> std::optional<bool> {
        clingo_truth_value_t res = 0;
        Detail::handle_error(clingo_assignment_truth_value(assignment_, lit, &res));
        switch (res) {
            case clingo_truth_value_true: {
                return true;
            }
            case clingo_truth_value_false: {
                return false;
            }
            default: {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] auto root_level() const -> uint32_t {
        uint32_t level = 0;
        Detail::handle_error(clingo_assignment_root_level(assignment_, &level));
        return level;
    }

    [[nodiscard]] auto trail() const -> Trail { return Trail{assignment_}; }

  private:
    clingo_assignment_t const *assignment_;
};

class PropagateInit {
  public:
    explicit PropagateInit(clingo_propagate_init_t *init) : init_{init} {}

    auto assignment() -> Assignment {
        clingo_assignment_t const *assignment = nullptr;
        Detail::handle_error(clingo_propagate_init_assignment(init_, &assignment));
        return Assignment{assignment};
    }

    /*
    auto library() -> PyLibrary {
        clingo_lib_t *lib = nullptr;
        Detail::handle_error(clingo_propagate_init_library(init_, &lib));
        return Library::cast(lib);
    }
    */

    auto base() -> Base {
        clingo_base_t const *base = nullptr;
        Detail::handle_error(clingo_propagate_init_base(init_, &base));
        return {base};
    }

    auto get_check_mode() -> clingo_propagator_check_mode_e {
        clingo_propagator_check_mode_t mode = 0;
        Detail::handle_error(clingo_propagate_init_get_check_mode(init_, &mode));
        return static_cast<clingo_propagator_check_mode_e>(mode);
    }

    void set_check_mode(clingo_propagator_check_mode_e mode) {
        Detail::handle_error(clingo_propagate_init_set_check_mode(init_, mode));
    }

    auto number_of_threads() -> size_t {
        size_t res = 0;
        Detail::handle_error(clingo_propagate_init_number_of_threads(init_, &res));
        return res;
    }

    auto get_undo_mode() -> clingo_propagator_undo_mode_e {
        clingo_propagator_check_mode_t mode = 0;
        Detail::handle_error(clingo_propagate_init_get_undo_mode(init_, &mode));
        return static_cast<clingo_propagator_undo_mode_e>(mode);
    }

    void set_undo_mode(clingo_propagator_undo_mode_e mode) {
        Detail::handle_error(clingo_propagate_init_set_undo_mode(init_, mode));
    }

    auto add_clause(ProgramLiteralSpan literals) -> bool {
        auto res = false;
        Detail::handle_error(clingo_propagate_init_add_clause(init_, literals.data(), literals.size(), &res));
        return res;
    }

    auto add_literal(bool freeze) -> clingo_literal_t {
        clingo_literal_t lit = 0;
        Detail::handle_error(clingo_propagate_init_add_literal(init_, freeze, &lit));
        return lit;
    }

    void add_minimize(clingo_literal_t literal, clingo_weight_t weight, clingo_weight_t priority) {
        Detail::handle_error(clingo_propagate_init_add_minimize(init_, literal, weight, priority));
    }

    void add_watch(clingo_literal_t lit, std::optional<uint32_t> thread_id) {
        if (thread_id) {
            Detail::handle_error(clingo_propagate_init_add_watch_to_thread(init_, lit, *thread_id));
        } else {
            Detail::handle_error(clingo_propagate_init_add_watch(init_, lit));
        }
    }

    auto add_weight_constraint(clingo_literal_t literal, WeightedLiteralSpan literals, clingo_weight_t bound,
                               clingo_weight_constraint_type_e type, bool compare_equal) -> bool {
        auto res = false;
        Detail::handle_error(clingo_propagate_init_add_weight_constraint(
            init_, literal, literals.data(), literals.size(), bound, type, compare_equal, &res));
        return res;
    }

    void freeze_literal(clingo_literal_t lit) {
        Detail::handle_error(clingo_propagate_init_freeze_literal(init_, lit));
    }

    auto propagate() -> bool {
        auto res = false;
        Detail::handle_error(clingo_propagate_init_propagate(init_, &res));
        return res;
    }

    void remove_watch(clingo_literal_t lit, std::optional<uint32_t> thread_id) {
        if (thread_id) {
            Detail::handle_error(clingo_propagate_init_remove_watch_from_thread(init_, lit, *thread_id));
        } else {
            Detail::handle_error(clingo_propagate_init_remove_watch(init_, lit));
        }
    }

    auto solver_literal(clingo_literal_t lit) -> clingo_literal_t {
        clingo_literal_t res = 0;
        Detail::handle_error(clingo_propagate_init_solver_literal(init_, lit, &res));
        return res;
    }

  private:
    clingo_propagate_init_t *init_;
};

class PropagateControl {
  public:
    explicit PropagateControl(clingo_propagate_control_t *ctl) : ctl_{ctl} {}

    auto add_clause(ProgramLiteralSpan literals, bool tag, bool lock) -> bool {
        clingo_clause_type_t type = 0;
        if (tag) {
            type |= clingo_clause_type_volatile;
        }
        if (lock) {
            type |= clingo_clause_type_static;
        }
        auto res = false;
        Detail::handle_error(clingo_propagate_control_add_clause(ctl_, literals.data(), literals.size(), type, &res));
        return res;
    }

    auto add_literal() -> clingo_literal_t {
        clingo_literal_t lit = 0;
        Detail::handle_error(clingo_propagate_control_add_literal(ctl_, &lit));
        return lit;
    }

    auto add_nogood(ProgramLiteralSpan literals, bool tag, bool lock) -> bool {
        thread_local auto lits = ProgramLiteralVector{};
        lits.clear();
        lits.reserve(literals.size());
        std::ranges::transform(literals.begin(), literals.end(), std::back_inserter(lits),
                               [](auto const &lit) { return -lit; });
        return add_clause(lits, tag, lock);
    }

    void add_watch(clingo_literal_t lit) { Detail::handle_error(clingo_propagate_control_add_watch(ctl_, lit)); }

    auto has_watch(clingo_literal_t lit) -> bool {
        auto res = false;
        Detail::handle_error(clingo_propagate_control_has_watch(ctl_, lit, &res));
        return res;
    }

    auto propagate() -> bool {
        auto res = false;
        Detail::handle_error(clingo_propagate_control_propagate(ctl_, &res));
        return res;
    }

    void remove_watch(clingo_literal_t lit) { Detail::handle_error(clingo_propagate_control_remove_watch(ctl_, lit)); }

    auto assignment() -> Assignment {
        clingo_assignment_t const *assignment = nullptr;
        Detail::handle_error(clingo_propagate_control_assignment(ctl_, &assignment));
        return Assignment{assignment};
    }

    auto thread_id() -> uint32_t {
        uint32_t id = 0;
        Detail::handle_error(clingo_propagate_control_thread_id(ctl_, &id));
        return id;
    }

  private:
    clingo_propagate_control_t *ctl_;
};

class Propagator {
  public:
    void init(PropagateInit &init);
    void propagate(PropagateControl &ctl, ProgramLiteralSpan changes);
    void undo(uint32_t thread_id, Assignment &assignment, ProgramLiteralSpan changes);
    void check(PropagateControl &ctl);
    auto decide(uint32_t thread_id, Assignment &assignment, clingo_literal_t lit) -> clingo_literal_t;
};

} // namespace Clingo
