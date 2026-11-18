#pragma once

#include "base.hh"
#include "iterable.hh"
#include "stats.hh"
#include "symbol.hh"

#include <clingo/control.h>

namespace PyClingo {

namespace AST {
class Program;
}

using GroundFinishCallback = std::function<void(clingo_ground_result_e)>;

using Part = clingo_part_t;
using PartSpan = std::span<Part const>;

class ConstMap {
  public:
    using key_type = std::string_view;
    using mapped_type = Symbol;
    using value_type = std::pair<key_type, mapped_type>;

    explicit ConstMap(clingo_const_map_t const *map) : map_{map} {}
    [[nodiscard]] auto contains(key_type name) const -> bool;
    [[nodiscard]] auto get(key_type name, std::optional<mapped_type> def) const -> std::optional<mapped_type>;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto size() const -> size_t;

  private:
    clingo_const_map_t const *map_;
};

class Control;
using PyControl = Annotation<Control>;

//! How a clingo control object is passed to a callback (see py_control_begin()).
enum class ControlRef : uint8_t { share, claim };

//! A control object.
//!
//! Each clingo control object is represented by at most one control object,
//! which is registered as its owner so that callbacks receive the same object.
//! There are two kinds of control objects:
//!
//! - Owned control objects are created from Python or claimed for a callback
//!   (see py_control_begin()). They own their clingo control object.
//! - Non-owned control objects are created for clingo control objects that are
//!   shared with a callback but not represented by a Python object yet (see
//!   py_control_begin()). They neither acquire nor release their clingo
//!   control object. Instead, the clingo control object holds a reference to
//!   them, which is released by a finalizer when it is destroyed.
//!
//! In both cases, objects registered with the control object (e.g.,
//! propagators) are kept alive by the Python object, which also keeps the
//! library alive. The reference to the library and the registered objects are
//! visible to the garbage collector.
class Control : public managed_handle<Control, clingo_control_t>, public reference_keeper<Control> {
  public:
    static constexpr char const *invalid_message = "control is no longer valid";

    using HintConstMap = TypeHint<"typing.Mapping[str, clingo.symbol.Symbol]">;

    Control(Library &lib, std::span<std::string const> args);
    Control(Control const &other) = delete;
    Control(Control &&other) noexcept = delete;
    auto operator=(Control const &other) -> Control & = delete;
    auto operator=(Control &&other) noexcept -> Control & = delete;
    //! Unregister the control object from its clingo control object.
    ~Control();

    //! Create a non-owned control object.
    static auto borrow(py::object lib, clingo_control_t *ctl) -> py::object;
    //! Create an owned control object for an existing clingo control object.
    static auto acquire(py::object lib, clingo_control_t *ctl) -> py::object;
    //! Get the control object registered with the given clingo control object (nullptr if there is none).
    static auto owner(clingo_control_t *ctl) -> Control *;

    auto mode() -> clingo_mode_e;
    void parse_files(std::span<std::string const> files);
    void parse_string(std::string_view str);
    void write_aspif(std::optional<std::string_view> path, bool symbols, bool append, std::optional<bool> preamble,
                     bool preprocess);
    void join(AST::Program &prg);
    void ground(std::optional<PartSpan> parts, py::handle ctx);
    auto start_ground(std::optional<PartSpan> parts, py::handle ctx,
                      TypeHint<"collections.abc.Callable[[clingo.ground.GroundResult], None] | None"> on_finish)
        -> TypeHint<"clingo.ground.GroundHandle">;
    using ModelCallback = TypeHint<"collections.abc.Callable[[clingo.solve.Model], bool | None] | None">;
    using UnsatCallback = TypeHint<"collections.abc.Callable[[typing.Sequence[int]], None] | None">;
    using StatsCallback = TypeHint<"collections.abc.Callable[[clingo.stats.Stats, clingo.stats.Stats], None] | None">;
    using FinishCallback = TypeHint<"collections.abc.Callable[[clingo.solve.SolveResult], None] | None">;
    using SolveResult = TypeHint<"clingo.solve.SolveResult">;
    using SolveHandle = TypeHint<"clingo.solve.SolveHandle">;
    auto solve(MixedLitSpan const &assumptions, ModelCallback on_model, UnsatCallback on_unsat, StatsCallback on_stats,
               FinishCallback on_finish) -> SolveResult;
    auto start_solve(MixedLitSpan const &assumptions, ModelCallback on_model, UnsatCallback on_unsat,
                     StatsCallback on_stats, FinishCallback on_finish, bool yield, bool async)
        -> Annotation<SolveHandle>;
    auto base() -> TypeHint<"clingo.base.Base">;
    void observe(TypeHint<"clingo.backend.Observer"> &obs, bool preprocess);
    auto backend() -> TypeHint<"clingo.backend.BackendManager">;
    auto config() -> TypeHint<"clingo.config.Config">;
    auto stats() -> StatsView;
    auto profile() -> py::list;
    void main();
    auto buffer() -> std::string_view;
    auto const_map() -> HintConstMap;
    auto parts() -> std::optional<PartSpan>;
    void set_parts(std::optional<PartSpan> parts);
    void interrupt();
    void discard(bool minimize, bool project);

    void register_propagator(TypeHint<"clingo.propagate.Propagator"> propagator);

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

    //! Detach the control object from its clingo control object.
    void invalidate() noexcept;

    //! Set up garbage collection support.
    //!
    //! When cleared, an owning control object releases its clingo control
    //! object first.
    static void setup(PyHeapTypeObject *heap_type);

  private:
    using Parent = managed_handle<Control, clingo_control_t>;
    Control(py::object lib, clingo_control_t *ctl, Parent::borrow_t tag);
    Control(py::object lib, clingo_control_t *ctl);

    //! Register the control object with its clingo control object.
    void register_();
    //! Remove the registration of the control object.
    void unregister_() noexcept;

    static auto callable_([[maybe_unused]] char const *name, [[maybe_unused]] size_t name_size,
                          [[maybe_unused]] size_t arguments_size, [[maybe_unused]] void *data, bool *result) -> bool {
        CLINGO_TRY {
            *result = true;
        }
        CLINGO_CATCH;
    }
};

auto py_control_type() -> PyObject *;

//! Begin using a `clingo_control_t` object from Python.
//!
//! Returns a new reference to a `clingo.control.Control` object that must be
//! passed to `py_control_end` with the same reference type.
//!
//! If the reference type is claim, the control must not have a Python owner;
//! otherwise, an error is raised. The returned object owns the control until
//! the matching end call, which invalidates it.
//!
//! If the reference type is share and the control already has a Python owner,
//! that owner is returned. Otherwise, a non-owning object is returned and
//! registered as the control's owner. The `clingo_control_t` object keeps it
//! alive and invalidates it when it is destroyed.
auto py_control_begin(PyObject *type, PyObject *library, clingo_control_t *ctl, ControlRef ref_type) -> PyObject *;

//! End using a `clingo_control_t` object from Python.
//!
//! Steals the reference to the object returned by `py_control_begin`.
//!
//! See `py_control_begin` for the reference type semantics.
void py_control_end(PyObject *obj, ControlRef ref_type) noexcept;

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
