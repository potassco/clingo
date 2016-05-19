// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SMODELSCONVERTER_H
#define SMODELSCONVERTER_H

#include <gringo/gringo.h>
#include <gringo/output.h>

namespace gringo
{
	namespace NS_OUTPUT
	{
		class SmodelsConverter : public Output
		{
		private:
			typedef std::pair<int, int> Lit;
			typedef std::vector<Lit> LitVec;
		public:
			SmodelsConverter(std::ostream *out, bool shift);
			virtual void initialize(GlobalStorage *g, SignatureVector *pred);
			virtual void print(Object *o);
			int getFalse() const;
			virtual ~SmodelsConverter();
		protected:
			virtual void printBasicRule(int head, const IntVector &pos, const IntVector &neg) = 0;
			virtual void printConstraintRule(int head, int bound, const IntVector &pos, const IntVector &neg) = 0;
			virtual void printChoiceRule(const IntVector &head, const IntVector &pos, const IntVector &neg) = 0;
			virtual void printWeightRule(int head, int bound, const IntVector &pos, const IntVector &neg, const IntVector &wPos, const IntVector &wNeg) = 0;
			virtual void printMinimizeRule(const IntVector &pos, const IntVector &neg, const IntVector &wPos, const IntVector &wNeg) = 0;
			virtual void printDisjunctiveRule(const IntVector &head, const IntVector &pos, const IntVector &neg) = 0;
			virtual void printComputeRule(int models, const IntVector &pos, const IntVector &neg) = 0;
		private:
			void print(Fact *r);
			void print(Rule *r);
			void print(Integrity *r);
			void print(Optimize *r);
			void print(Compute *r);
			void printHead(Aggregate *a);
			void printBody(Aggregate *a);
			void printRule(int head, ...);
			void handleHead(Object *o);
			void handleBody(ObjectVector &body);
			void handleAggregate(ObjectVector &lits);
			void handleAggregate(ObjectVector &lits, IntVector &weights);
			void handleCount(Aggregate *a, int &l, int &u);
			void handleSum(bool body, Aggregate *a, int &l, int &u);
			void handleAvg(bool body, Aggregate *a, int &l, int &u);
			void convertTimes(LitVec &lits, IntVector zero, IntVector neg, int bound, int &var);
			void handleTimes(bool body, Aggregate *a, int &l, int &u);
			void handleMin(Aggregate *a, int &l, int &u);
			void handleMax(Aggregate *a, int &l, int &u);
			void convertParity(const IntVector &lits, int bound, int &l);
			void handleParity(bool body, Aggregate *a, int &l);
		protected:
			int false_;
		private:
			bool negBoundsWarning_;
			bool shift_;
			IntVector compute_;
			IntVector head_;
			IntVector pos_, neg_, wPos_, wNeg_;
			IntVector posA_, negA_, wPosA_, wNegA_;
		};
	}
}

#endif

