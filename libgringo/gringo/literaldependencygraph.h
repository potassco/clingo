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

#ifndef LITERALDEPENDENCYGRAPH_H
#define LITERALDEPENDENCYGRAPH_H

#include <gringo/gringo.h>

namespace gringo
{
	class LDG
	{
		friend class LDGBuilder;
	//private:
	public:
		struct LiteralNode;
		struct VarNode;

		typedef std::vector<VarNode*>     VarNodeVector;
		typedef std::vector<LiteralNode*> LiteralNodeVector;

		struct LiteralNode
		{
			LiteralNode(Literal *l, bool head);
			Literal *l_;
			int done_;
			int in_;
			bool head_;
			VarNodeVector out_;
			LDGVector sub_;
		};
		struct VarNode
		{
			VarNode(int var);
			bool done_;
			int var_;
			LiteralNodeVector out_;
		};
		struct LiteralData
		{
			LiteralData(LiteralNode *n);
			LiteralNode *n_;
			VarVector needed_;
			VarVector provided_;
			LiteralList::iterator pos;
		};

		typedef std::map<Literal*, LiteralData*> LiteralDataMap;
	public:
		LDG();

		const VarVector &getGlobalVars() const;
		const VarVector &getParentVars() const;

		void start(LiteralList &list);
		void propagate(Literal *l, LiteralList &list);

		void sortLiterals(LiteralVector *lits);

		const VarVector &getNeededVars(Literal *l) const;
		const VarVector &getProvidedVars(Literal *l) const;
		
		bool hasVarNodes() const;

		~LDG();
	protected:
		VarVector         parentVars_;
		VarVector         globalVars_;
		LiteralNodeVector litNodes_;
		VarNodeVector     varNodes_;
		LiteralDataMap    litMap_;
		bool              sorted_;
	};

	class LDGBuilder
	{
	protected:
		typedef std::map<int, LDG::VarNode*> VarNodeMap;
		typedef std::map<int, LDG::LiteralNode*> LiteralNodeMap;

		struct GraphNode
		{
			LDG::LiteralNode *n_;
			LDGBuilderVector sub_;
			GraphNode(LDG::LiteralNode *l);
		};
		typedef std::vector<GraphNode*> GraphNodeVector;
	public:
		LDGBuilder(LDG *dg);
		void addHead(Literal *l);
		void addToBody(Literal *l);
		void addGraph(LDGBuilder *dg);

		void create();
		void createNode(Literal *l, bool head, const VarSet &needed, const VarSet &provided, bool graph = false);

		~LDGBuilder();

	protected:
		LDG::LiteralNode *createLiteralNode(Literal *l, bool head);
		LDG::VarNode *createVarNode(int var);
		void createSubGraph(LDGBuilder *parent, LDG::LiteralNode *l);

	protected:
		LDGBuilder *parent_;
		LDG::LiteralNode *parentNode_;
		LDG *dg_;
		LiteralVector head_;
		LiteralVector body_;
		VarNodeMap varNodes_;
		GraphNodeVector graphNodes_;
	};

}

#endif

