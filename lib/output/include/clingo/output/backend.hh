#include <clingo/core/output.hh>

namespace Clingo::Output {

using lit_t = int32_t;
using atom_t = uint32_t;

using LitSpan = std::span<lit_t const>;
using LitVec = std::vector<lit_t>;
using AtomSpan = std::span<atom_t const>;
using AtomVec = std::vector<atom_t>;

//! Interface connecting grounder and solver.
//!
//! The backend is repsonsible for passig grounded statements to the solver (or
//! other forms of backends).
class Backend {
  public:
    void rule(AtomSpan head, LitSpan body, bool choice) { do_rule(head, body, choice); }
    void show(Symbol sym, LitSpan body) { do_show(sym, body); }
    virtual ~Backend() = default;

  private:
    virtual void do_rule(AtomSpan head, LitSpan body, bool choice) = 0;
    virtual void do_show(Symbol sym, LitSpan body) = 0;
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
