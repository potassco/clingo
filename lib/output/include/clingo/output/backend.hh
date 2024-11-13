#include <clingo/core/output.hh>

namespace Clingo::Output {

//! Interface connecting grounder and solver.
//!
//! The backend is repsonsible for passig grounded statements to the solver (or
//! other forms of backends).
class Backend {
  public:
    void rule(std::span<uint32_t const> head, std::span<int32_t const> body, bool choice) {
        do_rule(head, body, choice);
    }
    void show(Symbol sym, std::span<int32_t const> body) { do_show(sym, body); }
    virtual ~Backend() = default;

  private:
    virtual void do_rule(std::span<uint32_t const> head, std::span<int32_t const> body, bool choice) = 0;
    virtual void do_show(Symbol sym, std::span<int32_t const> body) = 0;
};
using UBackend = std::unique_ptr<Backend>;

//! Create an output that forwards ground statements to a backend.
//!
//! Backends accept a simpler format as provided by the grounder. This output
//! brings the statements into the required form and passes them to the
//! backend.
//!
//! @param backend the target Backend
auto make_backend_output(Backend &backend) -> UOutputStm;

} // namespace Clingo::Output
