#pragma once

#include <gringo/util/record.hh>

namespace Gringo::Input {

constexpr auto a_anonymous = Util::Record::AttributeName<1>{};
constexpr auto a_elems = Util::Record::AttributeName<2>{};
constexpr auto a_exteral = Util::Record::AttributeName<3>{};
constexpr auto a_lhs = Util::Record::AttributeName<4>{};
constexpr auto a_loc = Util::Record::AttributeName<5>{};
constexpr auto a_name = Util::Record::AttributeName<6>{};
constexpr auto a_op = Util::Record::AttributeName<7>{};
constexpr auto a_pool = Util::Record::AttributeName<8>{};
constexpr auto a_rhs = Util::Record::AttributeName<9>{};
constexpr auto a_value = Util::Record::AttributeName<10>{};

} // namespace Gringo::Input
