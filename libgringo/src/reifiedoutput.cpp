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

#include <gringo/reifiedoutput.h>
#include <gringo/domain.h>
#include <gringo/storage.h>
#include <gringo/predlitrep.h>
#include <gringo/func.h>

namespace reifiedoutput_impl
{

GRINGO_EXPORT_PRINTER(DisplayPrinter)
GRINGO_EXPORT_PRINTER(ExternalPrinter)
GRINGO_EXPORT_PRINTER(RulePrinter)
GRINGO_EXPORT_PRINTER(SumAggrLitPrinter)
GRINGO_EXPORT_PRINTER(AvgAggrLitPrinter)
GRINGO_EXPORT_PRINTER(MinMaxAggrLitPrinter)
GRINGO_EXPORT_PRINTER(ParityAggrLitPrinter)
GRINGO_EXPORT_PRINTER(JunctionAggrLitPrinter)
GRINGO_EXPORT_PRINTER(OptimizePrinter)
GRINGO_EXPORT_PRINTER(ComputePrinter)
GRINGO_EXPORT_PRINTER(IncPrinter)

}

ReifiedOutput::Node::Node(size_t symbol)
	: symbol_(symbol)
	, visited_(0)
	, finished_(1)
{ }
size_t ReifiedOutput::Node::symbol() const { return symbol_; }
void ReifiedOutput::Node::visit(size_t visited) { visited_ = visited + 1; }
uint32_t ReifiedOutput::Node::visited() const { return visited_ - 1; }
void ReifiedOutput::Node::mark() { visited_ = 1; }
bool ReifiedOutput::Node::marked() const { return visited_ >= 1; }
void ReifiedOutput::Node::pop() { finished_ = 0; }
bool ReifiedOutput::Node::popped() const { return finished_ == 0; }

ReifiedOutput::Node *ReifiedOutput::Node::next()
{
	while(finished_ - 1 < children.size())
	{
		ReifiedOutput::Node *next = children[finished_++ - 1];
		if(!next->marked()) { return next; }
	}
	return 0;
}

bool ReifiedOutput::Node::root()
{
	bool root = true;
	foreach(ReifiedOutput::Node* node, children)
	{
		if(!node->popped() && node->visited() < visited())
		{
			root = false;
			visit(node->visited());
		}
	}
	return root;
}

ReifiedOutput::ReifiedOutput(std::ostream *out)
	: Output(out)
{
	initPrinters<ReifiedOutput>();
	startSet(); // compute table
}

void ReifiedOutput::startSet()
{
	setStack_.push_back(Set());
}

void ReifiedOutput::addToSet(size_t id)
{
	setStack_.back().push_back(id);
}

uint32_t ReifiedOutput::addSet()
{
	Set &back = setStack_.back();
	std::sort(back.begin(), back.end());
	back.erase(std::unique(back.begin(), back.end()), back.end());
	std::pair<SetMap::iterator, bool> res = sets_.insert(SetMap::value_type(back, sets_.size()));
	uint32_t index = res.first->second;
	if(res.second)
	{
		foreach(size_t sym, back)
		{
			out() << "set(" << index << ",";
			val(sym).print(storage(), out());
			out() << ").\n";
		}
	}
	return index;
}

void ReifiedOutput::startList()
{
	listStack_.push_back(List());
}

void ReifiedOutput::addToList(size_t id, const Val &v)
{
	listStack_.back().push_back(List::value_type(id, v));
}

namespace
{
	bool cmp(const ReifiedOutput::List::value_type &a, const ReifiedOutput::List::value_type &b)
	{
		if(a.first != b.first) { return a.first < b.first; }
		if(a.second != b.second)
		{
			if(a.second.type == Val::NUM) { return a.second.num < b.second.num; }
			else                          { return a.second.index < b.second.index; }
		}
		return false;
	}
}

uint32_t ReifiedOutput::addList()
{
	List &back = listStack_.back();
	std::sort(back.begin(), back.end(), cmp);
	std::pair<ListMap::iterator, bool> res = lists_.insert(ListMap::value_type(back, lists_.size()));
	uint32_t index = res.first->second;
	if(res.second)
	{
		uint32_t i = 0;
		foreach(List::const_reference sym, back)
		{
			out() << "wlist(" << index << "," << i++ << ",";
			val(sym.first).print(storage(), out());
			out() << ",";
			sym.second.print(storage(), out());
			out() << ").\n";
		}
	}
	return index;
}

void ReifiedOutput::popDep(bool add, size_t n)
{
	for(size_t i = 0; i < n; i++)
	{
		if(add) { addDep(); }
		dep_.pop_back();
	}
}

void ReifiedOutput::addDep()
{
	assert(dep_.size() > 1);
	foreach(const size_t &head, *(dep_.end() - 2))
	{
		Graph::iterator headIt = graph_.find(head);
		if(headIt == graph_.end()) { headIt = graph_.insert(Graph::value_type(head, Node(head))).first; }
		foreach(const size_t &body, dep_.back())
		{
			Graph::iterator bodyIt = graph_.find(body);
			if(bodyIt == graph_.end()) { bodyIt = graph_.insert(Graph::value_type(body, Node(body))).first; }
			headIt->second.children.push_back(&bodyIt->second);
		}
	}
}

const Val &ReifiedOutput::val(size_t symbol) const
{
	return symbols_[symbol];
}

size_t ReifiedOutput::symbol(const Val &val)
{
	SymbolMap::iterator it = symbols_.push_back(val).first;
	return it - symbols_.begin();
}

size_t ReifiedOutput::symbol(PredLitRep *pred)
{
	Val val;
	if(pred->dom()->arity() > 0)
	{
		val = Val::create(Val::FUNC, storage()->index(Func(storage(), pred->dom()->nameId(), ValVec(pred->vals().begin(), pred->vals().end()))));
	}
	else { val = Val::create(Val::ID, pred->dom()->nameId()); }
	val = Val::create(Val::FUNC, storage()->index(Func(storage(), storage()->index("atom"), ValVec(1, val))));
	val = Val::create(Val::FUNC, storage()->index(Func(storage(), storage()->index(pred->sign() ? "neg" : "pos"), ValVec(1, val))));
	return symbol(val);
}

void ReifiedOutput::minimize(size_t sym, int weight, int prio)
{
	List & list = minimize_[prio];
	if(weight != 0) { list.push_back(List::value_type(sym, Val::create(Val::NUM, weight))); }
}

void ReifiedOutput::tarjan()
{
	uint32_t scc = 0;
	std::vector<Node*> s;
	std::vector<Node*> t;

	foreach(Graph::reference ref, graph_)
	{
		if(!ref.second.popped())
		{
			uint32_t visited = 0;
			s.push_back(&ref.second);
			ref.second.mark();

			while(!s.empty())
			{
				Node *x = s.back();
				if(x->visited() == 0)
				{
					visited++;
					x->visit(visited);
					t.push_back(x);
				}
				Node *y = x->next();
				if(y != 0)
				{
					s.push_back(y);
					y->mark();
				}
				else
				{
					s.pop_back();
					if(x->root())
					{
						if(t.back() != x)
						{
							do
							{
								y = t.back();
								t.pop_back();
								y->pop();
								out() << "scc(" << scc << ",";
								val(y->symbol()).print(storage(), out());
								out() << ").\n";
							}
							while(y != x);
							scc++;
						}
						else
						{
							t.pop_back();
							x->pop();
						}
					}
				}
			}
		}
	}
}

void ReifiedOutput::finalize()
{
	tarjan();
	foreach(MiniMap::const_reference ref, minimize_)
	{
		listStack_.push_back(ref.second);
		uint32_t list = addList();
		popList();
		out() << "minimize(" << ref.first << "," << list << ").\n";
	}
	foreach(const Signature &sig, external_)
	{
		DomainMap::const_iterator i = s_->domains().find(sig);
		if(i != s_->domains().end())
		{
			const std::string &name  = s_->string(i->second->nameId());
			uint32_t           arity = i->second->arity();
			*out_ << "external(sig(" << name << "," << arity << ")).\n";
		}
	}
	if(!getSet().empty()) *out_ << "compute(" << addSet() << ").\n";
	popSet();
}

void ReifiedOutput::doShow(bool s)
{
	*out_ << (s ? "show" : "hide") << ".\n";
}

void ReifiedOutput::doShow(uint32_t nameId, uint32_t arity, bool s)
{
	*out_ << (s ? "show(" : "hide(") << "sig(" << storage()->string(nameId) << "," << arity << ")).\n";
}

void ReifiedOutput::addCompute(PredLitRep *l)
{
	addToSet(symbol(l));
}

ReifiedOutput::~ReifiedOutput()
{
}

