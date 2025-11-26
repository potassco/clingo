#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>
#include <clingo/stats.hh>

#include <clingo/ground.h>

namespace Clingo {

//! @addtogroup cpp_ground
//! Grounding logic programs.
//! @{

//! A program part to provide inputs to program directives.
struct Part {
    //! Constructs a program part with the given name and parameters.
    //!
    //! @param name the name of the part
    //! @param params the parameters of the part
    constexpr Part(std::string name, SymbolVector params = {}) : name{std::move(name)}, params(std::move(params)) {
        assert(!this->name.empty());
    }

    //! The name of the part.
    std::string name;
    //! The parameters of the part.
    SymbolVector params;
};
//! A span of program parts.
using PartSpan = std::span<Part const>;
//! An initializer list of program parts.
using PartList = std::initializer_list<Part>;
//! A vector of program parts.
using PartVector = std::vector<Part>;

//! Enumeration of the results of a grounding process.
enum class GroundResult : clingo_ground_result_t {
    ok = clingo_ground_result_ok,                       //!< Grounding completed successfully.
    unsatisfiable = clingo_ground_result_unsatisfiable, //!< An inconsistency was detected during grounding.
    interrupted = clingo_ground_result_interrupted,     //!< Grounding was interrupted.
};

//! An interface for handling grounding events.
class GroundEventHandler {
  public:
    //! Default constructor.
    GroundEventHandler() = default;
    //! Default destructor.
    virtual ~GroundEventHandler() = default;

    //! Check whether a function is callable for the given number of arguments.
    //!
    //! @param name the name of the function
    //! @param args the number of arguments
    auto callable(std::string_view name, size_t args) -> bool { return do_callable(name, args); }

    //! Call the given function with the given arguments.
    //!
    //! @param loc the location of the function call
    //! @param name the name of the function
    //! @param args the arguments of the function
    //! @return the symbols to inject
    auto call(Location const &loc, std::string_view name, SymbolSpan args) -> SymbolVector {
        return do_call(loc, name, args);
    }

    //! Notify the handler that grounding has finished with the given result.
    void finish(GroundResult result) noexcept { do_finish(result); }

  private:
    virtual auto do_callable([[maybe_unused]] std::string_view name, [[maybe_unused]] size_t args) -> bool {
        return false;
    }
    virtual auto do_call([[maybe_unused]] Location const &loc, [[maybe_unused]] std::string_view name,
                         [[maybe_unused]] SymbolSpan args) -> SymbolVector {
        return {};
    }
    virtual void do_finish([[maybe_unused]] GroundResult result) noexcept {}
};

//! Class to control a running ground call.
class GroundHandle {
  public:
    //! Constructor from the underlying C representation.
    //!
    //! For internal use.
    //!
    //! @param hnd the C representation of the solve handle
    explicit GroundHandle(clingo_ground_handle_t *hnd) : hnd_{hnd} {}

    //! Cast the solve handle to its C representation.
    //! @param x the solve handle to cast
    //! @return the C representation of the solve handle
    friend auto c_cast(GroundHandle const &x) -> clingo_ground_handle_t * { return x.hnd_.get(); }

    //! Get the solve result.
    //!
    //! This is a blocking operation and should always be called at the end of
    //! a search.
    //!
    //! @return the solve result
    [[nodiscard]] auto get() const -> GroundResult {
        return GroundResult{Detail::call<clingo_ground_handle_get>(hnd_.get())};
    }

    //! Cancel the current search.
    //!
    //! This is a blocking operation.
    void cancel() { Detail::handle_error(clingo_ground_handle_cancel(hnd_.get())); }

    //! Closes the solve handle.
    //!
    //! Blocks until the grounding process has been cleaned up.
    void close() noexcept { hnd_.reset(); }

    //! Wait for a running ground call.
    //!
    //! If no timeout is given, waits until the ground call is finished.
    //! Otherwise, waits for the given timeout in seconds. A value of zero can
    //! be used for polling.
    //!
    //! @param timeout the optional timeout in seconds
    //! @return whether a model or result is available
    [[nodiscard]] auto wait(std::optional<double> timeout) -> bool {
        return Detail::call<clingo_ground_handle_wait>(hnd_.get(), timeout.value_or(-1));
    }

  private:
    struct Del {
        void operator()(clingo_ground_handle_t *hnd) const noexcept { clingo_ground_handle_close(hnd); }
    };
    using Ptr = std::unique_ptr<clingo_ground_handle_t, Del>;

    Ptr hnd_;
};

//! @}

} // namespace Clingo
