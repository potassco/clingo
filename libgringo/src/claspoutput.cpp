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

#include <gringo/claspoutput.h>

//#define DEBUG_ICLASP

#ifdef DEBUG_ICLASP
#include <fstream>
std::fstream g_out("iclasp.h", std::ios_base::out | std::ios_base::trunc);
#endif

#ifdef WITH_CLASP


#include <gringo/gringoexception.h>
#include <gringo/grounder.h>
#include <clasp/program_builder.h>

using namespace gringo;
using namespace NS_OUTPUT;

ClaspOutput::ClaspOutput(Clasp::ProgramBuilder *b, bool shift) : SmodelsConverter(&std::cout, shift), b_(b)
{
}

void ClaspOutput::initialize(GlobalStorage *g, SignatureVector *pred)
{
	SmodelsConverter::initialize(g, pred);
	b_->setCompute(getFalse(), false);
#ifdef DEBUG_ICLASP
	g_out << "api.setCompute(t" << getFalse() << ", false);" << NL;
#endif
}

void ClaspOutput::printBasicRule(int head, const IntVector &pos, const IntVector &neg)
{
	b_->startRule();
	b_->addHead(head);
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++)
		b_->addToBody(*it, false);
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++)
		b_->addToBody(*it, true);
	b_->endRule();
#ifdef DEBUG_ICLASP
	g_out << "api.startRule().addHead(t" << head << ")";
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++)
		g_out << ".addToBody(t" << *it << ", false)";
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++)
		g_out << ".addToBody(t" << *it << ", true)";
	g_out << ".endRule();" << NL;
#endif

}

void ClaspOutput::printConstraintRule(int head, int bound, const IntVector &pos, const IntVector &neg)
{
	b_->startRule(Clasp::CONSTRAINTRULE, bound);
	b_->addHead(head);
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++)
		b_->addToBody(*it, false);
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++)
    b_->addToBody(*it, true);
	b_->endRule();
#ifdef DEBUG_ICLASP
	g_out << "api.startRule(Clasp::CONSTRAINTRULE, " << bound << ").addHead(t" << head << ")";
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++)
		g_out << ".addToBody(t" << *it << ", false)";
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++)
		g_out << ".addToBody(t" << *it << ", true)";
	g_out << ".endRule();" << NL;
#endif
}

void ClaspOutput::printChoiceRule(const IntVector &head, const IntVector &pos, const IntVector &neg)
{
	b_->startRule(Clasp::CHOICERULE);
	for(IntVector::const_iterator it = head.begin(); it != head.end(); it++)
		b_->addHead(*it);
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++)
		b_->addToBody(*it, false);
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++)
		b_->addToBody(*it, true);
	b_->endRule();
#ifdef DEBUG_ICLASP
	g_out << "api.startRule(Clasp::CHOICERULE)";
	for(IntVector::const_iterator it = head.begin(); it != head.end(); it++)
		g_out << ".addHead(t" << *it << ")";
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++)
		g_out << ".addToBody(t" << *it << ", false)";
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++)
		g_out << ".addToBody(t" << *it << ", true)";
	g_out << ".endRule();" << NL;
#endif
}

void ClaspOutput::printWeightRule(int head, int bound, const IntVector &pos, const IntVector &neg, const IntVector &wPos, const IntVector &wNeg)
{
	b_->startRule(Clasp::WEIGHTRULE, bound);
	b_->addHead(head);
	IntVector::const_iterator itW = wNeg.begin();
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++, itW++)
		b_->addToBody(*it, false, *itW);
	itW = wPos.begin();
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++, itW++)
		b_->addToBody(*it, true, *itW);
	b_->endRule();
#ifdef DEBUG_ICLASP
	g_out << "api.startRule(Clasp::WEIGHTRULE, " << bound << ").addHead(t" << head << ")";
	itW = wNeg.begin();
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++, itW++)
		g_out << ".addToBody(t" << *it << ", false, " << *itW << ")";
	itW = wPos.begin();
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++, itW++)
		g_out << ".addToBody(t" << *it << ", true, " << *itW << ")";
	g_out << ".endRule();" << NL;
#endif
}

void ClaspOutput::printMinimizeRule(const IntVector &pos, const IntVector &neg, const IntVector &wPos, const IntVector &wNeg)
{
	b_->startRule(Clasp::OPTIMIZERULE);
	IntVector::const_iterator itW = wNeg.begin();
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++, itW++)
		b_->addToBody(*it, false, *itW);
	itW = wPos.begin();
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++, itW++)
		b_->addToBody(*it, true, *itW);
	b_->endRule();
}

void ClaspOutput::printDisjunctiveRule(const IntVector &head, const IntVector &pos, const IntVector &neg)
{
	throw GrinGoException("Error: sorry clasp cannot handle disjunctive rules use option --shift!");
}

void ClaspOutput::printComputeRule(int models, const IntVector &pos, const IntVector &neg)
{
	for(IntVector::const_iterator it = neg.begin(); it != neg.end(); it++)
		b_->setCompute(*it, false);
	for(IntVector::const_iterator it = pos.begin(); it != pos.end(); it++)
		b_->setCompute(*it, true);
}

void ClaspOutput::finalize(bool)
{

}

int ClaspOutput::newUid()
{
	uids_++;
	int uid = b_->newAtom();
#ifdef DEBUG_ICLASP
	g_out << "int t" << uid << " = api.newAtom();" << NL;
#endif
	return uid;
}


bool ClaspOutput::addAtom(NS_OUTPUT::Atom *r)
{
	bool ret = Output::addAtom(r);
	if(ret && isVisible(r->predUid_))
	{
		b_->setAtomName(r->uid_, atomToString(r->predUid_, r->values_).c_str());
#ifdef DEBUG_ICLASP
		g_out << "api.setAtomName(t" << r->uid_ << ", \"" << atomToString(r->predUid_, r->values_) << "\");" << NL;
#endif
	}
	return ret;
}

ClaspOutput::~ClaspOutput()
{
}

#endif

#ifdef WITH_ICLASP

IClaspOutput::IClaspOutput(Clasp::ProgramBuilder *b, bool shift) : ClaspOutput(b, shift), incUid_(0)
{
}

void IClaspOutput::initialize(GlobalStorage *g, SignatureVector *pred)
{
#ifdef DEBUG_ICLASP
	g_out << "LitVec assumptions;" << NL << NL;
	g_out << "solver.undoUntil(0);" << NL;
	g_out << "api.updateProgram();" << NL;
#endif
	ClaspOutput::initialize(g, pred);
	incUid_ = 0;
	reinitialize();
}

void IClaspOutput::reinitialize()
{
#ifdef DEBUG_ICLASP
	if(incUid_)
	{
		g_out << "solver.undoUntil(0);" << NL;
		g_out << "api.updateProgram();" << NL;
		g_out << "api.unfreeze(t" << incUid_ << ");" << NL;
	}
#endif
	if(incUid_)
	{
		b_->unfreeze(incUid_);
	}

	// create a new uid
	incUid_ = newUid();
	b_->setAtomName(incUid_, "");
	b_->freeze(incUid_);
#ifdef DEBUG_ICLASP
	g_out << "api.setAtomName(t" << incUid_ << ", \"\");" << NL;
	g_out << "// delta(" << incUid_ << ")." << NL;
	g_out << "api.freeze(t" << incUid_ << ");" << NL;
#endif
}

void IClaspOutput::finalize(bool last)
{
#ifdef DEBUG_ICLASP
	if(!last)
	{
		static int its = 0;
		its++;
		g_out << "std::cout << \"============= solving " << its << " =============\" << std::endl;" << NL;
		g_out << "api.endProgram(solver, options.initialLookahead);" << NL << NL;
		g_out << "assumptions.clear();" << NL;
		g_out << "assumptions.push_back(api.stats.index[t" << incUid_ << "].lit);" << NL;
		g_out << "Clasp::solve(solver, assumptions, options.numModels, options.solveParams);" << NL << NL;
	}
#endif
	ClaspOutput::finalize(last);
}

int IClaspOutput::getIncUid()
{
	return incUid_;
}

void IClaspOutput::print(NS_OUTPUT::Object *o)
{
#ifdef DEBUG_ICLASP
	g_out << "// ";
	o->print_plain(this, g_out);
#endif
	SmodelsConverter::print(o);
}

#endif

