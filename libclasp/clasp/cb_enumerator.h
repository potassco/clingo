// 
// Copyright (c) 2006-2011, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef CLASP_CB_ENUMERATOR_H
#define CLASP_CB_ENUMERATOR_H

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/enumerator.h>

namespace Clasp {

//! Enumerator for computing the brave/cautious consequences of a logic program.
/*!
 * \ingroup enumerator
 */
class CBConsequences : public Enumerator {
public:
	enum Consequences_t { 
		brave_consequences    = (Model::max_value << 1) | Model::model_cons,
		cautious_consequences = (Model::max_value << 2) | Model::model_cons,
	};
	/*!
	 * \param type Type of consequences to compute.
	 */
	explicit CBConsequences(Consequences_t type);
	~CBConsequences();
	int  modelType() const { return type_; }
	bool exhaustive()const { return true; }
private:
	class  CBFinder;
	class  SharedConstraint;
	ConPtr doInit(SharedContext& ctx, SharedMinimizeData* m, int numModels);
	void   addCurrent(Solver& s, LitVec& con, ValueVec& m);
	LitVec            cons_;
	SharedConstraint* shared_;
	Consequences_t    type_;
};

}
#endif
