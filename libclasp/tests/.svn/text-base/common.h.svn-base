#ifndef TEST_COMMON_H_INCLUDED
#define TEST_COMMON_H_INCLUDED

#include <clasp/literal.h>

namespace Clasp {
inline std::ostream& operator<<(std::ostream& os, Literal p) {
	if (p.sign()) os << "-";
	os << p.var();
	return os;
}
}

#endif

