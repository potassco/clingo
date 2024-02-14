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
constexpr auto a_sign = Util::Record::AttributeName<11>{};
constexpr auto a_term = Util::Record::AttributeName<12>{};
constexpr auto a_lit = Util::Record::AttributeName<13>{};
constexpr auto a_cond = Util::Record::AttributeName<14>{};
constexpr auto a_type = Util::Record::AttributeName<15>{};
constexpr auto a_args = Util::Record::AttributeName<16>{};
constexpr auto a_tuple = Util::Record::AttributeName<17>{};
constexpr auto a_fun = Util::Record::AttributeName<18>{};
constexpr auto a_head = Util::Record::AttributeName<19>{};
constexpr auto a_body = Util::Record::AttributeName<20>{};
constexpr auto a_prio = Util::Record::AttributeName<21>{};
constexpr auto a_op_defs = Util::Record::AttributeName<21>{};
constexpr auto a_term_defs = Util::Record::AttributeName<22>{};
constexpr auto a_atom_defs = Util::Record::AttributeName<23>{};
constexpr auto a_arity = Util::Record::AttributeName<24>{};

} // namespace Gringo::Input
