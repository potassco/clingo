#include <input/algo/safety.hh>

namespace Gringo::Input {

// TODO:
// - check whether statements are safe
// - maybe even order rule bodies
//   - maybe try to stay close to the given rule
//   - maybe give preference to comparisons (or at least assignments)
// - checking:
//   - [(literal, provide, depend)]
//   - pick if depend <= provided
//   - set provided = provide + provided
//   - it only makes sense to add assignments once!
//     - they should be added in both directions to the check list
//     - their "second direction" should not be added to a ordered body

}
