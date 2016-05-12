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
#ifndef CLASP_MODEL_ENUMERATORS_H
#define CLASP_MODEL_ENUMERATORS_H

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/enumerator.h>
#include <clasp/clause.h>

namespace Clasp { 

//! Class for model enumeration with minimization and projection.
/*!
 * This class implements algorithms for enumerating models with or without optimization
 * and/or projection. It supports two different algorithms (strategies), first, enumeration
 * via restricted backjumping, and second, enumeration via recording of solution nogoods.
 *
 * The first strategy, strategy_backtrack, maintains a special backtracking level to
 * suppress certain backjumps that could otherwise "re-open" search spaces already visited.
 * If projection is not active, no extra nogoods are created.
 * Otherwise, the number of additional solution nogoods is linear in the number of projection atoms.
 * 
 * The second strategy, strategy_record, enumerates models by recording nogoods for found solutions.
 * In general, this strategy is exponential in space (bounded by the number of solutions).
 * On the other hand, if optimization is active, additional nogoods are not needed
 * because the optimization constraint already serves as solution nogood.
 *
 * There is also a third strategy, strategy_auto, provided for convenience. This
 * strategy automatically selects between strategy_backtrack and strategy_record
 * based on the problem at hand. It uses strategy_record, if one of the following holds:
 *  - optimization is active, or
 *  - only one model is requested, or
 *  - both parallel search as well as projection are active
 *  .
 * In all other cases, strategy_auto selects strategy_backtrack.
 * 
 * \ingroup enumerator
 */
class ModelEnumerator : public Enumerator {
public:
	//! Enumeration algorithms.
	enum Strategy {
		strategy_auto      = 0, /*!< Use strategy best suited to problem. */
		strategy_backtrack = 1, /*!< Use backtrack-based enumeration.     */
		strategy_record    = 2  /*!< Use nogood-based enumeration.        */
	};
	//! Projective solution enumeration and options.
	enum ProjectOptions {
		project_enable_simple = 1, /*!< Enable projective solution enumeration. */
		project_use_heuristic = 2, /*!< Use heuristic when selecting a literal from a projection nogood. */
		project_save_progress = 4, /*!< Enable progress saving after the first solution was found. */
		project_enable_full   = 6, /*!< Enable projective solution enumeration with heuristic and progress saving. */
		project_dom_lits      = 8, /*!< In strategy record, project only on true domain literals. */ 
	};
	/*! 
	 * \param p The printer to use for outputting results.
	 */
	explicit ModelEnumerator(Strategy st = strategy_auto);
	~ModelEnumerator();
	
	//! Configure strategy.
	/*!
	 * \params st         Enumeration algorithm to use. 
	 * \params projection The set of ProjectOptions to be applied or 0 to disable projective enumeration.
	 * \params filter     Ignore output predicates starting with filter in projective enumeration.
	 */
	void     setStrategy(Strategy st = strategy_auto, uint32 projection = 0, char filter = '_');
	bool     projectionEnabled()const { return projectOpts() != 0; }
	Strategy strategy()         const { return static_cast<Strategy>(options_ & 3u); }
protected:
	bool   supportsRestarts() const { return optimize() || strategy() == strategy_record; }
	bool   supportsParallel() const { return !projectionEnabled() || strategy() != strategy_backtrack; }
	bool   supportsSplitting(const SharedContext& problem) const { 
		return (strategy() == strategy_backtrack || (projectOpts() & project_dom_lits) == 0u) && Enumerator::supportsSplitting(problem); 
	}
	ConPtr doInit(SharedContext& ctx, SharedMinimizeData* m, int numModels);
private:
	enum { detect_strategy_flag = 4u, trivial_flag = 8u, strategy_opts_mask = 15u };
	class ModelFinder;
	class BacktrackFinder;
	class RecordFinder;
	void    initProjection(SharedContext& ctx);
	void    addProject(SharedContext& ctx, Var v);
	uint32  projectOpts()    const { return (options_ >> 4) & strategy_opts_mask; }
	bool    detectStrategy() const { return (options_ & detect_strategy_flag) == detect_strategy_flag; }
	bool    trivial()        const { return (options_ & trivial_flag) == trivial_flag; }
	LitVec  domRec_;
	uint32  options_;
};

}
#endif
