// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <gringo/gringo.h>
#include <gringo/func.h>

class Storage
{
private:
	typedef boost::multi_index::multi_index_container
	<
		std::string, boost::multi_index::indexed_by
		<
			boost::multi_index::random_access<>,
			boost::multi_index::hashed_unique<boost::multi_index::identity<std::string> >
		>
	> StringSet;
	typedef boost::multi_index::multi_index_container
	<
		Func, boost::multi_index::indexed_by
		<
			boost::multi_index::random_access<>,
			boost::multi_index::hashed_unique<boost::multi_index::identity<Func> >
		>
	> FuncSet;
public:
	std::string quote(const std::string &str);
	std::string unquote(const std::string &str);
	Storage(Output *output);
	uint32_t index(const Func &f);
	const Func &func(uint32_t i);
	uint32_t index(const std::string &s);
	const std::string &string(uint32_t i);
	Domain *domain(uint32_t nameId, uint32_t arity);
	const DomainMap &domains() const { return doms_; }
	Output *output() const { return output_; }
	virtual void print(std::ostream &out, uint32_t type, uint32_t index);
	virtual int compare(uint32_t type, uint32_t a, uint32_t b);
	~Storage();
private:
	StringSet strings_;
	FuncSet   funcs_;
	DomainMap doms_;
	Output   *output_;
};

inline void Storage::print(std::ostream &, uint32_t, uint32_t) { assert(false); }
inline int Storage::compare(uint32_t, uint32_t, uint32_t) { assert(false); return 0; }
