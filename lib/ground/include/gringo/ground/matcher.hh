#include <gringo/ground/base.hh>
#include <gringo/ground/instantiator.hh>
#include <gringo/ground/term.hh>

#include <gringo/core/core.hh>

namespace Gringo::Ground {

auto make_once_matcher() -> UMatcher;

auto make_interval_matcher(std::vector<bool> const &bound, Term const &lhs, Term const &lower,
                           Term const &upper) -> UMatcher;

auto make_comp_matcher(std::vector<bool> const &bound, Term const &lhs, Relation rel, Term const &rhs) -> UMatcher;

auto make_non_fact_matcher(Base const &base, Term const &term) -> UMatcher;

auto make_atom_matcher(std::vector<bool> const &bound, Base &base, Term const &atom, MatcherType type) -> UMatcher;

} // namespace Gringo::Ground
