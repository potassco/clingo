#pragma once

#include "backend.hh"
#include "base.hh"
#include "config.hh"
#include "ground.hh"
#include "iterable.hh"
#include "propagate.hh"
#include "solve.hh"
#include "symbol.hh"

#include <clingo/control.h>

namespace PyClingo {

namespace AST {
class Program;
}

using Part = clingo_part_t;
using PartSpan = std::span<Part const>;

class ConstMap {
  public:
    using key_type = std::string_view;
    using mapped_type = Symbol;
    using value_type = std::pair<key_type, mapped_type>;

    ConstMap(clingo_const_map_t const *map) : map_{map} {}
    [[nodiscard]] auto contains(key_type name) const -> bool;
    [[nodiscard]] auto get(key_type name, std::optional<mapped_type> def) const -> std::optional<mapped_type>;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto size() const -> size_t;

  private:
    clingo_const_map_t const *map_;
};

class Control;
using PyControl = Annotation<Control>;

class Control : public registered_handle<Control, clingo_control_t>, public reference_keeper<Control> {
  public:
    using HintConstMap = TypeHint<"typing.Mapping[str, clingo.symbol.Symbol]">;

    Control(Library &lib, std::span<std::string const> args);

    auto mode() -> clingo_mode_e;
    void parse_files(std::span<std::string const> files);
    void parse_string(std::string_view str);
    void write_aspif(std::string_view path, bool symbols, bool append, std::optional<bool> preamble, bool preprocess);
    void join(AST::Program &prg);
    void ground(std::optional<PartSpan> parts, py::handle ctx);
    auto start_ground(std::optional<PartSpan> parts, py::handle ctx,
                      Annotation<std::optional<GroundFinishCallback>> on_finish) -> Annotation<GroundHandle>;
    auto solve(MixedLitSpan const &assumptions, Annotation<std::optional<ModelCallback>> on_model,
               Annotation<std::optional<UnsatCallback>> on_unsat, Annotation<std::optional<StatsCallback>> on_stats,
               Annotation<std::optional<FinishCallback>> on_finish) -> SolveResult;
    auto start_solve(MixedLitSpan const &assumptions, Annotation<std::optional<ModelCallback>> on_model,
                     Annotation<std::optional<UnsatCallback>> on_unsat,
                     Annotation<std::optional<StatsCallback>> on_stats,
                     Annotation<std::optional<FinishCallback>> on_finish, bool yield, bool async)
        -> Annotation<SolveHandle>;
    auto base() -> Base;
    void observe(Observer &obs, bool preprocess);
    auto backend() -> BackendManager;
    auto config() -> Config;
    auto stats() -> py::dict;
    auto profile() -> py::list;
    void main();
    auto buffer() -> std::string_view;
    auto const_map() -> HintConstMap;
    auto parts() -> std::optional<PartSpan>;
    void set_parts(std::optional<PartSpan> parts);
    void interrupt();
    void discard(bool minimize, bool project);

    void register_propagator(Annotation<Propagator> const &propagator);

    static auto cast(clingo_control_t *ctl, bool convert = false) -> PyControl;
    //! This function acquires the control and user data.
    //!
    //! If no user data exists yet, it creates a default one.
    //!
    //! If parameter inc can be set to false to take ownership of a previously
    //! created control object.
    static void acquire(clingo_control_t *ctl, bool inc = true);
    //! This function releases the wrapped control and the associated user
    //! data.
    static void release(clingo_control_t *ctl) noexcept;
    //! Get the underlying C pointer.
    auto c_ptr() -> clingo_control_t * { return get(); }

  private:
    using Parent = registered_handle<Control, clingo_control_t>;
    Control(clingo_control_t *ctl) : Parent{ctl} {}
    static auto callable_([[maybe_unused]] char const *name, [[maybe_unused]] size_t name_size,
                          [[maybe_unused]] size_t arguments_size, [[maybe_unused]] void *data, bool *result) -> bool {
        CLINGO_TRY {
            *result = true;
        }
        CLINGO_CATCH;
    }
    static auto call_(clingo_lib_t *lib, clingo_location_t const *location, char const *name, size_t name_size,
                      clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                      clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> bool;

    constexpr static clingo_ground_event_handler_t ctx_ =
        clingo_ground_event_handler_t{&callable_, &call_, nullptr, nullptr};
};

void register_control(pybind11::module &m);

} // namespace PyClingo

namespace pybind11::detail {

struct part_span_holder {
    using name_conv = make_caster<std::string>;
    using params_conv = make_caster<PyClingo::SymbolVec>;
    using type = PyClingo::PartSpan;

    auto load(handle src, bool convert) -> bool {
        if (!isinstance<iterable>(src)) {
            return false;
        }
        auto n = len(src);
        names.reserve(n);
        args.reserve(n);
        parts.reserve(n);
        for (auto const &part : src) {
            name_conv nc;
            params_conv sc;
            if (!nc.load(part[pybind11::int_{0}], convert) || !sc.load(part[pybind11::int_{1}], convert)) {
                return false;
            }
            names.emplace_back(cast_op<std::string>(std::move(nc)));
            args.emplace_back(cast_op<PyClingo::SymbolVec>(std::move(sc)));
            parts.emplace_back(names.back().data(), names.back().size(), c_cast(args.back().data()),
                               args.back().size());
        }
        return true;
    }

    [[nodiscard]] auto cast() const -> type { return parts; }

    std::vector<std::string> names;
    std::vector<PyClingo::SymbolVec> args;
    std::vector<PyClingo::Part> parts;
};

template <> struct type_caster<PyClingo::PartSpan> {
  public:
    using name_conv = make_caster<std::string>;
    using params_conv = make_caster<PyClingo::SymbolVec>;
    using type = PyClingo::PartSpan;

    PYBIND11_TYPE_CASTER(type, _("Sequence[Tuple[str, Sequence[clingo.symbol.Symbol]]]"));

    auto load(handle src, bool convert) -> bool {
        if (!holder_.load(src, convert)) {
            return false;
        }
        value = holder_.cast();
        return true;
    }

    static auto cast(const type &src, return_value_policy policy, handle parent) -> handle {
        list res;
        for (auto const &part : src) {
            res.append(make_tuple(
                name_conv::cast(std::string{part.name, part.name_size}, policy, parent),
                params_conv::cast(std::span{PyClingo::cpp_cast(part.params), part.params_size}, policy, parent)));
        }
        return res.release();
    }

  private:
    part_span_holder holder_;
};

template <> struct type_caster<std::optional<PyClingo::PartSpan>> {
  public:
    using type = std::optional<PyClingo::PartSpan>;

    PYBIND11_TYPE_CASTER(type, _("Optional[Sequence[Tuple[str, Sequence[clingo.symbol.Symbol]]]]"));

    auto load(handle src, bool convert) -> bool {
        if (src.is_none()) {
            value = std::nullopt;
            return true;
        }
        if (!holder_.load(src, convert)) {
            return false;
        }
        value = holder_.cast();
        return true;
    }

    static auto cast(const type &src, return_value_policy policy, handle parent) -> handle {
        if (!src) {
            return none{};
        }
        return type_caster<PyClingo::PartSpan>::cast(*src, policy, parent);
    }

  private:
    part_span_holder holder_;
};

} // namespace pybind11::detail
