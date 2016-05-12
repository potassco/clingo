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

namespace LitDep
{
	class LitNode;
	struct LitNodeCmp
	{
		bool operator()(LitNode *a, LitNode *b);
	};

	typedef std::vector<LitNode*> LitQueue;

	class VarNode
	{
	private:
		typedef std::vector<LitNode*> LitNodeVec;
	public:
		VarNode(VarTerm *var);
		VarTerm *var() const;
		void depend(LitNode *litNode);
		void provide(LitNode *litNode);
		void reset();
		bool done();
		void propagate(LitQueue &queue);
	private:
		bool       done_;
		VarTerm    *var_;
		LitNodeVec provide_;
	};

	class LitNode
	{
	private:
		typedef std::vector<VarNode*> VarNodeVec;
	public:
		LitNode(Lit *lit);
		void depend(VarNode *varNode);
		void provide(VarNode *varNode);
		void reset();
		void check(LitQueue &queue);
		void propagate(LitQueue &queue);
		bool done();
		void score(double score) { score_ = score; }
		double score() const { return score_; }
		Lit *lit() const { return lit_; }
	private:
		Lit       *lit_;
		uint32_t   done_;
		uint32_t   depend_;
		double     score_;
		VarNodeVec provide_;
	};

	class GrdNode
	{
	private:
		typedef boost::ptr_vector<LitNode> LitNodeVec;
		typedef boost::ptr_vector<VarNode> VarNodeVec;
	public:
		GrdNode(Groundable *groundable);
		void append(LitNode *litNode);
		void append(VarNode *varNode);
		void reset();
		bool check(VarTermVec &terms);
		void order(Grounder *g, const VarSet &bound);
	private:
		Groundable *groundable_;
		LitNodeVec  litNodes_;
		VarNodeVec  varNodes_;
	};

	class Builder : public PrgVisitor
	{
	private:
		typedef std::vector<GrdNode*> GrdNodeVec;
		typedef std::vector<uint32_t> GrdNodeStack;
		typedef std::vector<LitNode*> LitNodeStack;
		typedef std::vector<VarNode*> VarNodeVec;
	public:
		Builder(uint32_t vars);
		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(Lit *lit, bool domain);
		void visit(Groundable *groundable, bool choice);
		bool check(VarTermVec &terms);
		~Builder();
	private:
		double       score_;
		LitNodeStack litStack_;
		GrdNodeVec   grdNodes_;
		GrdNodeStack grdStack_;
		VarNodeVec   varNodes_;
	};
}

namespace boost
{
	template <>
	inline LitDep::GrdNode* new_clone(const LitDep::GrdNode& a)
	{
		(void)a;
		return 0;
	}
}
