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
    TheoryBackend(TheoryBackend const &other) = delete;

    auto operator=(TheoryBackend const &other) -> TheoryBackend & = delete;

    TheoryBackend(TheoryBackend &&other) = delete;

    auto operator=(TheoryBackend &&other) -> TheoryBackend & = delete;

    [[nodiscard]] auto number(int number) const -> ProgramId {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_backend_theory_term_number(backend_, number, &id));
        return id;
    }

    [[nodiscard]] auto string(std::string_view str) const -> ProgramId {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_backend_theory_term_string(backend_, str.data(), str.size(), &id));
        return id;
    }

    [[nodiscard]] auto symbol(Symbol const &symbol) const -> ProgramId {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_backend_theory_term_symbol(backend_, c_cast(symbol), &id));
        return id;
    }

    [[nodiscard]] auto sequence(TheorySequenceType type, IdSpan elements) const -> ProgramId {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_backend_theory_term_sequence(
            backend_, static_cast<clingo_theory_sequence_type_t>(type), elements.data(), elements.size(), &id));
        return id;
    }

    [[nodiscard]] auto function(std::string_view name, IdSpan elements) const -> ProgramId {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_backend_theory_term_function(backend_, name.data(), name.size(), elements.data(),
                                                                 elements.size(), &id));
        return id;
    }

    [[nodiscard]] auto element(IdSpan tuple, ProgramLiteralSpan condition) const -> ProgramId {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_backend_theory_element(backend_, tuple.data(), tuple.size(), condition.data(),
                                                           condition.size(), &id));
        return id;
    }

    [[nodiscard]] auto atom(std::optional<ProgramAtom> atom, Symbol const &name, IdSpan elements,
                            std::optional<std::pair<std::string_view, ProgramId>> const &guard) const -> ProgramId {
        clingo_atom_t res = 0;
        auto op = clingo_string_t{guard ? guard->first.data() : nullptr, guard ? guard->first.size() : 0};
        Detail::handle_error(clingo_backend_theory_atom(backend_, c_cast(name), elements.data(), elements.size(),
                                                        guard ? &op : nullptr, guard ? guard->second : 0,
                                                        atom ? &*atom : nullptr, &res));
        return res;
    }

  private:
    explicit TheoryBackend(clingo_backend_t *backend) : backend_{backend} {}

    friend class ProgramBackend;

    clingo_backend_t *backend_;
};

class ProgramBackend : private TheoryBackend {
  public:
    explicit ProgramBackend(clingo_backend_t *backend) : TheoryBackend{backend} {
        // NOTE: We assume that backends are only created during normal
        // operation - not during exception handling.
        assert(std::uncaught_exceptions() == 0);
    }

    ProgramBackend(ProgramBackend const &other) = delete;

    auto operator=(ProgramBackend const &other) -> ProgramBackend & = delete;

    ProgramBackend(ProgramBackend &&other) noexcept : TheoryBackend{std::exchange(other.backend_, nullptr)} {}

    auto operator=(ProgramBackend &&other) noexcept -> ProgramBackend & {
        std::swap(backend_, other.backend_);
        return *this;
    }

    ~ProgramBackend() noexcept(false) {
        if (std::uncaught_exceptions() == 0) {
            close();
        }
    }

    void close() { Detail::handle_error(clingo_backend_close(std::exchange(backend_, nullptr))); }

    [[nodiscard]] auto atom(std::optional<Symbol> symbol) const -> ProgramAtom {
        clingo_atom_t atom = 0;
        Detail::handle_error(clingo_backend_add_atom(backend_, symbol ? c_cast(&symbol.value()) : nullptr, &atom));
        return atom;
    }

    void rule(ProgramAtomSpan head, ProgramLiteralSpan body, bool choice) const {
        Detail::handle_error(clingo_backend_rule(backend_, choice, head.data(), head.size(), body.data(), body.size()));
    }

    void weight_rule(ProgramAtomSpan head, Weight lower, WeightedLiteralSpan body, bool choice) const {
        Detail::handle_error(
            clingo_backend_weight_rule(backend_, choice, head.data(), head.size(), lower, body.data(), body.size()));
    }

    void minimize(WeightedLiteralSpan literals, Weight priority) const {
        Detail::handle_error(clingo_backend_minimize(backend_, priority, literals.data(), literals.size()));
    }

    void project(ProgramAtomSpan atoms) const {
        Detail::handle_error(clingo_backend_project(backend_, atoms.data(), atoms.size()));
    }

    void external(ProgramAtom atom, ExternalType type) const {
        Detail::handle_error(clingo_backend_external(backend_, atom, static_cast<clingo_external_type_t>(type)));
    }

    void assume(ProgramLiteralSpan literals) const {
        Detail::handle_error(clingo_backend_assume(backend_, literals.data(), literals.size()));
    }

    void heuristic(ProgramAtom atom, HeuristicType type, int bias, unsigned priority,
                   ProgramLiteralSpan condition) const {
        Detail::handle_error(clingo_backend_heuristic(backend_, atom, static_cast<clingo_heuristic_type_t>(type), bias,
                                                      priority, condition.data(), condition.size()));
    }

    void edge(int node_u, int node_v, ProgramLiteralSpan condition) const {
        Detail::handle_error(clingo_backend_acyc_edge(backend_, node_u, node_v, condition.data(), condition.size()));
    }

    [[nodiscard]] auto theory() const -> TheoryBackend const & { return *this; }
};

} // namespace Clingo
