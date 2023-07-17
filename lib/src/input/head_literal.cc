#include <input/head_literal.hh>

[[nodiscard]] auto HeadLiteral::print_empty() const -> bool { return false; }

////////// Disjunction //////////

auto Disjunction::print_empty() const -> bool { return elems_.empty(); }
