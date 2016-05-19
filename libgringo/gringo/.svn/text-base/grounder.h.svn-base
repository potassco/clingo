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

#ifndef GROUNDER_H
#define GROUNDER_H

#include <gringo/gringo.h>
#include <gringo/output.h>
#include <gringo/globalstorage.h>

namespace gringo
{
	class Grounder : public GlobalStorage
	{
	private:
		typedef std::pair<int, IntVector*> DomainPredicate;
		typedef std::vector<DomainPredicate> DomainPredicateVector;
		typedef std::map<int, int> VariableMap;
		typedef std::map<int, std::pair<bool, Term*> > ConstTerms;
		typedef std::vector<std::pair<std::pair<IncPart, Value>, int> > IncParts;
		typedef std::map<int, std::pair<bool, int> > IncShifts;
		typedef std::map<int, int> InvShifts;
	public:
		struct Options
		{
			Options() : binderSplit(true), iquery(1), ifixed(-1), ibase(false), debug(false) {}
			bool binderSplit;
			int iquery;
			int ifixed;
			bool ibase;
			bool debug;
		};
	public:
		Grounder(const Options &opts = Options());
		void setOutput(NS_OUTPUT::Output *output);
		void addStatement(Statement *rule);
		void addDomains(int id, std::vector<IntVector*>* list);
		void prepare(bool incremental);
		void ground();
		void addProgram(Program *scc);
		void addTrueNegation(int id, int arity);
		int getVar(int var);
		int createUniqueVar();
		const std::string *getVarString(int uid);
		int registerVar(int var);
		// access the current substitution
		Value getValue(int var);
		void setValue(int var, const Value &val, int binder);
		void setTempValue(int var, const Value &val);
		// access binders
		int getBinder(int var) const;
		void setConstValue(int id, Term *p);
		Value getConstValue(int id);
		void preprocess();
		NS_OUTPUT::Output *getOutput();
		Evaluator *getEvaluator();
		/// Adds a domain that never occurs in any head
		void addZeroDomain(int uid);
		void setIncPart(IncPart part, const Value &v);
		void setIncShift(const std::string &id, int arity);
		void setIncShift(const std::string &a, const std::string &b, int arity);
		std::pair<int,int> getIncShift(int uid, bool head) const;
		bool checkIncShift(int uid) const;
		bool isIncShift(int uid) const;

		int getIncStep() const;
		bool isIncGrounding() const;

		const Options &options() const;
		~Grounder();
	private:
		void buildDepGraph();
		void addDomains();
		void reset();
		void check();
		void ground_();
		void addDomains(int id, std::vector<IntVector*>::iterator pos, std::vector<IntVector*>::iterator end, IntVector &list);
	private:
		Options opts_;
		IncParts incParts_;
		bool incremental_;
		int incStep_;
		
		int internalVars_;
		ProgramVector sccs_;
		VariableMap varMap_;
		DomainPredicateVector domains_;
		StatementVector rules_;
		NS_OUTPUT::Output *output_;
		Evaluator *eval_;
		ConstTerms constTerms_;
		ValueVector substitution_;
		IntVector binder_;
		std::set<Signature> trueNegPred_;
		IncShifts shifts_;
		InvShifts invShifts_;
	};
}

#endif

