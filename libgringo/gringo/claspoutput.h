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

#ifndef CLASPOUTPUT_H
#define CLASPOUTPUT_H

#include <gringo/gringo.h>

#ifdef WITH_CLASP
#include <gringo/smodelsconverter.h>

namespace Clasp
{
	class ProgramBuilder;
}

namespace gringo
{
	namespace NS_OUTPUT
	{
		class ClaspOutput : public SmodelsConverter
		{
		public:
			ClaspOutput(Clasp::ProgramBuilder *b, bool shift);
			virtual void initialize(GlobalStorage *g, SignatureVector *pred);
			virtual void finalize(bool last);
			bool addAtom(NS_OUTPUT::Atom *r);
			int newUid();
			void setProgramBuilder(Clasp::ProgramBuilder* api) { b_ = api; }
			~ClaspOutput();
		protected:
			void printBasicRule(int head, const IntVector &pos, const IntVector &neg);
			void printConstraintRule(int head, int bound, const IntVector &pos, const IntVector &neg);
			void printChoiceRule(const IntVector &head, const IntVector &pos, const IntVector &neg);
			void printWeightRule(int head, int bound, const IntVector &pos, const IntVector &neg, const IntVector &wPos, const IntVector &wNeg);
			void printMinimizeRule(const IntVector &pos, const IntVector &neg, const IntVector &wPos, const IntVector &wNeg);
			void printDisjunctiveRule(const IntVector &head, const IntVector &pos, const IntVector &neg);
			void printComputeRule(int models, const IntVector &pos, const IntVector &neg);
		protected:
			Clasp::ProgramBuilder *b_;
		};
	}
}
#endif

#ifdef WITH_ICLASP
namespace gringo
{
	namespace NS_OUTPUT
	{
		class IClaspOutput : public ClaspOutput
		{
		public:
			IClaspOutput(Clasp::ProgramBuilder *b, bool shift);
			void print(NS_OUTPUT::Object *o);
			void initialize(GlobalStorage *g, SignatureVector *pred);
			void finalize(bool last);
			void reinitialize();
			int getIncUid();
		private:
			int incUid_;
		};
	}
}
#endif

#endif

