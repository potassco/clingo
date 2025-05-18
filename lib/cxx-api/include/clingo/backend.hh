#pragma once

#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/backend.h>

#include <cassert>

namespace Clingo {

enum class TheorySequenceType : clingo_theory_sequence_type_t {
    tuple = clingo_theory_sequence_type_tuple, //!< Theory tuples "(t1,...,tn)".
    set = clingo_theory_sequence_type_set,     //!< Theory sets "{t1,...,tn}".
    list = clingo_theory_sequence_type_list    //!< Theory lists "[t1,...,tn]".
};

class TheoryBackend {
  public:
    [[nodiscard]] auto number(int number) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_number>(backend_.get(), number);
    }

    [[nodiscard]] auto string(std::string_view str) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_string>(backend_.get(), str.data(), str.size());
    }

    [[nodiscard]] auto symbol(Symbol const &symbol) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_symbol>(backend_.get(), c_cast(symbol));
    }

    [[nodiscard]] auto sequence(TheorySequenceType type, ProgramIdSpan elements) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_sequence>(
            backend_.get(), static_cast<clingo_theory_sequence_type_t>(type), elements.data(), elements.size());
    }

    [[nodiscard]] auto function(std::string_view name, ProgramIdSpan elements) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_function>(backend_.get(), name.data(), name.size(),
                                                                 elements.data(), elements.size());
    }

    [[nodiscard]] auto element(ProgramIdSpan tuple, ProgramLiteralSpan condition) const -> ProgramId {
        return Detail::call<clingo_backend_theory_element>(backend_.get(), tuple.data(), tuple.size(), condition.data(),
                                                           condition.size());
    }

    [[nodiscard]] auto atom(std::optional<ProgramAtom> atom, Symbol const &name, ProgramIdSpan elements,
                            std::optional<std::pair<std::string_view, ProgramId>> const &guard) const -> ProgramId {
        auto op = clingo_string_t{guard ? guard->first.data() : nullptr, guard ? guard->first.size() : 0};
        return Detail::call<clingo_backend_theory_atom>(backend_.get(), c_cast(name), elements.data(), elements.size(),
                                                        guard ? &op : nullptr, guard ? guard->second : 0,
                                                        atom ? &*atom : nullptr);
    }

  private:
    friend class ProgramBackend;

    struct Free {
        Free() {
            // NOTE: We assume that backends are only created during normal
            // operation - not during exception handling.
            assert(std::uncaught_exceptions() == 0);
        }
        auto operator()(clingo_backend_t *backend) const noexcept(false) -> void {
            if (std::uncaught_exceptions() == 0) {
                Detail::handle_error(clingo_backend_close(std::exchange(backend, nullptr)));
            }
        }
    };

    explicit TheoryBackend(clingo_backend_t *backend) : backend_{backend} {}

    ~TheoryBackend() = default;

    std::unique_ptr<clingo_backend_t, Free> backend_;
};

class ProgramBackend : private TheoryBackend {
  public:
    explicit ProgramBackend(clingo_backend_t *backend) : TheoryBackend{backend} {}

    void close() { Detail::handle_error(clingo_backend_close(backend_.release())); }

    [[nodiscard]] auto atom(std::optional<Symbol> symbol) const -> ProgramAtom {
        return Detail::call<clingo_backend_add_atom>(backend_.get(), symbol ? c_cast(&symbol.value()) : nullptr);
    }

    void rule(ProgramAtomSpan head, ProgramLiteralSpan body = {}, bool choice = false) const {
        Detail::handle_error(
            clingo_backend_rule(backend_.get(), choice, head.data(), head.size(), body.data(), body.size()));
    }

    void weight_rule(ProgramAtomSpan head, Weight lower, WeightedLiteralSpan body, bool choice = false) const {
        Detail::handle_error(clingo_backend_weight_rule(backend_.get(), choice, head.data(), head.size(), lower,
                                                        body.data(), body.size()));
    }

    void minimize(WeightedLiteralSpan literals, Weight priority) const {
        Detail::handle_error(clingo_backend_minimize(backend_.get(), priority, literals.data(), literals.size()));
    }

    void project(ProgramAtomSpan atoms) const {
        Detail::handle_error(clingo_backend_project(backend_.get(), atoms.data(), atoms.size()));
    }

    void external(ProgramAtom atom, ExternalType type) const {
        Detail::handle_error(clingo_backend_external(backend_.get(), atom, static_cast<clingo_external_type_t>(type)));
    }

    void assume(ProgramLiteralSpan literals) const {
        Detail::handle_error(clingo_backend_assume(backend_.get(), literals.data(), literals.size()));
    }

    void heuristic(ProgramAtom atom, HeuristicType type, int bias, unsigned priority,
                   ProgramLiteralSpan condition) const {
        Detail::handle_error(clingo_backend_heuristic(backend_.get(), atom, static_cast<clingo_heuristic_type_t>(type),
                                                      bias, priority, condition.data(), condition.size()));
    }

    void edge(int node_u, int node_v, ProgramLiteralSpan condition) const {
        Detail::handle_error(
            clingo_backend_acyc_edge(backend_.get(), node_u, node_v, condition.data(), condition.size()));
    }

    [[nodiscard]] auto theory() const -> TheoryBackend const & { return *this; }
};

} // namespace Clingo
