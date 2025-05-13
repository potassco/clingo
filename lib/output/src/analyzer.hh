#pragma once

#include <clingo/core/core.hh>

#include <cstdint>

namespace CppClingo::Output {

enum AggregateMonotonicity : uint8_t { monotone, antimonotone, convex, nonmonotone };
enum AggregateWeightType : uint8_t { mixed, positive, negative };
enum AggregateTruth : uint8_t { true_, false_, unknown };

// tasks
// - analyze
//   - to add edges, the monotonicity of the aggregate must be known
//   - the value range, aggregate function, bound, weights, and signs of literals are important.
//   - positive edges have to be added for non-antimonotone aggregates
//   - it might be possible to simplify away some guards
//   - the simplified aggregate has to be stored for later translation
// - translate
//   - handle conditions based on monotonicity
//     (nonmonotone aggregates need equivalences)
//   - apply Marios translation
class AggregateAnalyzer {
  public:
    // TODO:
    // - elements, guards
    // -
    AggregateAnalyzer(Sign sign, AggregateFunction fun);

  private:
    AggregateMonotonicity monotonicity_ = AggregateMonotonicity::nonmonotone;
    AggregateWeightType weight_type_ = AggregateWeightType::mixed;
    AggregateTruth truth_ = AggregateTruth::unknown;
};

} // namespace CppClingo::Output
