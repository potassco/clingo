#include <clingo/core/output.hh>

namespace Clingo::Output {

//! Interface connecting grounder and solver.
//!
//! The backend is repsonsible for passig grounded statements to the solver (or
//! other forms of backends).
class Backend {
  public:
    virtual ~Backend() = default;
};

//! Create an output that forwards ground statements to a backend.
//!
//! Backends accept a simpler format as provided by the grounder. This output
//! brings the statements into the required form and passes them to the
//! backend.
//!
//! @param backend the target Backend
auto make_backend_output(Backend &backend) -> UOutputStm;

} // namespace Clingo::Output
