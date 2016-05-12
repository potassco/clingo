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
#include <gringo/prgvisitor.h>
#include <gringo/stats.h>

namespace StmDep
{
	/*
	 * NOTE:
	 * negative: a literal is negative if it is preceded by a not or inside a negative literal
	 * aggregates: for the sake of simplicity all aggregates are considered to be negative
	 * exception choice semantics: any aggregate in the head not preceeded by not is not negative
	 */
	class Node
	{
	public:
		enum Type { DOM, POS, NEG };
	public:
		Node();
		virtual Node *node(uint32_t i) = 0;
		virtual Type type(uint32_t i) = 0;
		virtual Node *next() = 0;
		virtual bool root() = 0;
		virtual void addToComponent(Grounder *g) { (void)g; }
		virtual void print(Storage *sto, std::ostream &out) const = 0;
		virtual const Loc &loc() const = 0;
		void mark() { visited_ = 1; }
		bool marked() const { return visited_ >= 1; }
		void visited(uint32_t visited) { visited_ = visited + 1; }
		uint32_t visited() const { return visited_ - 1; }
		bool hasComponent() { return component_ > 0; }
		void component(uint32_t component) { component_ = component + 1; }
		uint32_t component() const { return component_ - 1; }
	private:
		uint32_t visited_;
		uint32_t component_;
	};

	class StmNode;
	class PredNode : public Node
	{
		friend class Builder;
	private:
		typedef std::vector<StmNode*> DependVec;
	public:
		PredNode();
		Node *node(uint32_t i);
		Type type(uint32_t i) { (void)i; return POS; }
		Node *next();
		bool root();
		void pred(PredLit *pred) { pred_ = pred; }
		void depend(StmNode *stm);
		bool edbFact() const;
		bool empty() const;
		bool complete();
		bool isNull() const { return pred_ == 0; }
		PredLit *pred() const { return pred_; }
		void print(Storage *sto, std::ostream &out) const;
		const Loc &loc() const;
		virtual ~PredNode() { }
	private:
		PredLit  *pred_;
		DependVec depend_;
		uint32_t  next_;
		uint32_t  complete_;
	};

	class StmNode : public Node
	{
		friend class Builder;
	private:
		typedef std::vector<std::pair<PredNode*, Type> > DependVec;
		typedef std::vector<PredNode*> ProvideVec;
	public:
		StmNode(Statement *stm);
		Node *node(uint32_t i);
		Type type(uint32_t i);
		Node *next();
		bool root();
		void depend(PredNode *pred, Type t);
		void provide(PredNode *pred);
		void addToComponent(Grounder *g);
		void print(Storage *g, std::ostream &out) const;
		const Loc &loc() const;
		virtual ~StmNode() { }
	private:
		Statement *stm_;
		DependVec  depend_;
		ProvideVec provide_;
		uint32_t   next_;
	};

	class Builder : public PrgVisitor
	{
	private:
		typedef boost::ptr_vector<StmNode> StmNodeVec;
		typedef boost::ptr_vector<PredNode> PredNodeVec;
	public:
		using PrgVisitor::visit;
		Builder();
		void visit(PredLit *pred);
		void visit(Lit *lit, bool domain);
		void visit(Groundable *grd, bool choice);
		void visit(Statement *stm);
		void analyze(Grounder *g);
		void toDot(Grounder *g, std::ostream &out);
		~Builder();
	private:
		bool        domain_;
		uint32_t    monotonicity_;
		bool        head_;
		bool        choice_;
		StmNodeVec  stmNodes_;
		PredNodeVec predNodes_;
	};

}

