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

#include <gringo/output.h>
#include <gringo/domain.h>
#include <gringo/grounder.h>
#include <gringo/pilsoutput.h>
#include <gringo/gringoexception.h>

using namespace gringo;
using namespace NS_OUTPUT;

const char* END_ENTRY = " 0";
Output::Output(std::ostream *out) : uids_(1), out_(out), pred_(0), hideAll_(false), g_(0)//,
//												stats_.rules(0)//, stats_.atoms(0), stats_.count(0),
//												stats_.sum(0), stats_.max(0), stats_.min(0),
//												stats_.compute(0), stats_.optimize(0)
//TODO: Why does this not work - put this into the constructor of Stats
{
	stats_.language = Stats::UNKNOWN;
	stats_.rules    = 0;
	stats_.atoms    = 0;
	stats_.auxAtoms = 0;
	stats_.count    = 0;
	stats_.sum      = 0;
	stats_.count    = 0;
	stats_.max      = 0;
	stats_.min      = 0;
	stats_.compute  = 0;
	stats_.optimize = 0;
	
}

void Output::initialize(GlobalStorage *g, SignatureVector *pred)
{
	g_ = g;
	pred_ = pred;
	visible_.reserve(pred_->size());
	for(SignatureVector::const_iterator it = pred_->begin(); it != pred_->end(); it++)
		visible_.push_back(isVisible(it->first, it->second));
	atoms_.resize(pred_->size());
}

void Output::reinitialize()
{
}

int Output::getIncUid()
{
	throw GrinGoException("Error: output does not support getIncUid.");
}

std::string Output::atomToString(int id, const ValueVector &values) const
{
	int name = (*pred_)[id].first;
	std::stringstream ss;
	ss << *g_->getString(name);
	if(values.size() > 0)
	{
		ValueVector::const_iterator it = values.begin();
		ss << "(";
		it->print(g_, ss);
		for(it++; it != values.end(); it++)
		{
			ss << ",";
			it->print(g_, ss);
		}
		ss << ")";
	}
	return ss.str();
}

bool Output::addAtom(NS_OUTPUT::Atom *r)
{
	int id = r->predUid_;
	AtomHash::iterator res = atoms_[id].find(r->values_);
	if(res == atoms_[id].end())
	{
		res = atoms_[id].insert(std::make_pair(r->values_, newUid())).first;
		r->uid_     = res->second;
		++stats_.atoms;
		return true;
	}
	else
	{
		r->uid_     = res->second;
		return false;
	}
}

int Output::newUid()
{
	return uids_++;
}

void Output::hideAll()
{
	hideAll_ = true;
}

void Output::setVisible(int id, int arity, bool visible)
{
	hide_[std::make_pair(id, arity)] = !visible;
}

bool Output::isVisible(int uid)
{
	return visible_[uid];
}

bool Output::isVisible(int id, int arity)
{
	std::map<Signature, bool>::iterator it = hide_.find(std::make_pair(id, arity));
	if(it == hide_.end())
		return !hideAll_;
	else
		return !it->second;
}

void Output::addSignature()
{
	visible_.push_back(isVisible(pred_->back().first, pred_->back().second));
	atoms_.push_back(AtomHash());
}

Output::~Output()
{
}

// =============== NS_OUTPUT::Object ===============
Object::Object() : neg_(false) 
{
}

Object::Object(bool neg) : neg_(neg) 
{
}

int Object::getUid()
{
	return neg_ ? -uid_ : uid_;
}

Object::~Object() 
{
}

// =============== NS_OUTPUT::Atom ===============
Atom::Atom(bool neg, Domain *node, int predUid, const ValueVector &values) : Object(neg), node_(node), predUid_(predUid), values_(values)
{
}

Atom::Atom(bool neg, int predUid, const ValueVector &values) : Object(neg), node_(0), predUid_(predUid), values_(values)
{
}

Atom::Atom(bool neg, int predUid) : Object(neg), node_(0), predUid_(predUid)
{
}

void Atom::addDomain(bool fact)
{
	// atoms may occur negatively in aggregates
	// no domain has to be added if the atom is negative
	if(!neg_)
	{
		if(fact)
			node_->addFact(values_);
		node_->addDomain(values_);
	}
}

void Atom::print_plain(Output *o, std::ostream &out)
{
	out << (neg_ ? "not " : "") << o->atomToString(predUid_, values_);
}

void Atom::print(Output *o, std::ostream &out)
{
	if(o->addAtom(this))
	{
		if (o->isVisible(predUid_))
			out << "4" << " 3 " << uid_ << " " << o->atomToString(predUid_, values_) << " " << "1" << END_ENTRY << NL;
		else
			out << "4" << " 2 " << uid_ << " " << o->atomToString(predUid_, values_) << END_ENTRY << NL;
	}
}
	
// =============== NS_OUTPUT::Rule ===============
Rule::Rule(Object* head, Object *body) : head_(head), body_(body) 
{
}

void Rule::print_plain(Output *o, std::ostream &out)
{
	head_->print_plain(o, out);
	out << " :- ";
	body_->print_plain(o, out);
	out << "." << NL;
	++(o->stats_).rules;
}

void Rule::print(Output *o, std::ostream &out)
{
	head_->print(o, out);
	body_->print(o, out);
	uid_ = o->newUid();
	out << "5" << " 3 " << uid_ << " " << head_->getUid() << " " << body_->getUid() << END_ENTRY << NL;
	++(o->stats_).rules;
}

Rule::~Rule()
{
	delete head_;
	delete body_;
}

void Rule::addDomain(bool fact)
{
	head_->addDomain(false);
}

// =============== NS_OUTPUT::Fact ===============
Fact::Fact(Object *head) : head_(head) 
{
}

void Fact::print_plain(Output *o, std::ostream &out)
{
	head_->print_plain(o, out);
	out << "." << NL;
	++(o->stats_).rules;
}

void Fact::print(Output *o, std::ostream &out)
{
	head_->print(o, out);
	uid_ = o->newUid();
	out << "6" << " 2 " << uid_ << " " << head_->getUid() << END_ENTRY << NL;
	++(o->stats_).rules;
}

Fact::~Fact()
{
	delete head_;
}

void Fact::addDomain(bool fact)
{
	head_->addDomain(fact);
}

// =============== NS_OUTPUT::Integrity ===============
Integrity::Integrity(Object *body) : body_(body) 
{
}

void Integrity::print_plain(Output *o, std::ostream &out)
{
	out << " :- ";
	body_->print_plain(o, out);
	out << "." << NL;
	++(o->stats_).rules;

}

void Integrity::print(Output *o, std::ostream &out)
{
	body_->print(o, out);
	uid_ = o->newUid();
	out << "7" << " 2 " << uid_ << " " << body_->getUid() << END_ENTRY << NL;
	++(o->stats_).rules;
}

void Integrity::addDomain(bool fact)
{
}

Integrity::~Integrity()
{
	delete body_;
}

// =============== NS_OUTPUT::Conjunction ===============
Conjunction::Conjunction() 
{
}

Conjunction::Conjunction(ObjectVector &lits) 
{
	std::swap(lits, lits_);
}

void Conjunction::print_plain(Output *o, std::ostream &out)
{
	bool comma = false;
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		(*it)->print_plain(o, out);
	}
}

void Conjunction::print(Output *o, std::ostream &out)
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		(*it)->print(o, out);
	uid_ = o->newUid();
	out << "8" << " " << lits_.size() + 1 << " " << uid_;
	//TODO: kann leer sein, für leeres integrity constraint
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		out << " " << (*it)->getUid();
	out << END_ENTRY << NL;
}

void Conjunction::addDomain(bool fact)
{
}

Conjunction::~Conjunction()
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		delete *it;
}

// =============== NS_OUTPUT::Disjunction ===============
Disjunction::Disjunction() 
{
}

Disjunction::Disjunction(ObjectVector &lits) 
{
	std::swap(lits, lits_);
}

void Disjunction::print_plain(Output *o, std::ostream &out)
{
	bool comma = false;
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
	{
		if(comma)
			out << " | ";
		else
			comma = true;
		(*it)->print_plain(o, out);
	}
}

void Disjunction::print(Output *o, std::ostream &out)
{
	
	unsigned int normalForm = static_cast<PilsOutput*>(o)->getNormalForm();
	if (normalForm == 1 || normalForm == 3)
		throw GrinGoException("Disjunction not allowed on this normal form, please choose normal form 2 or 4-7.");

	static_cast<PilsOutput*>(o)->addOptimizedID(uid_);
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		(*it)->print(o, out);
	uid_ = o->newUid();
	out << "9" << " " << lits_.size() + 1 << " " << uid_;
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		out << " " << (*it)->getUid();
	out << END_ENTRY << NL;
}

void Disjunction::addDomain(bool fact)
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		(*it)->addDomain(false);
}

Disjunction::~Disjunction()
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		delete *it;
}

// =============== NS_OUTPUT::Aggregate ===============
Aggregate::Aggregate(bool neg, Type type, int lower, ObjectVector lits, IntVector weights, int upper) : 
	Object(neg), type_(type), bounds_(LU), lower_(lower), upper_(upper)
{
	std::swap(lits, lits_);
	std::swap(weights, weights_);
}

Aggregate::Aggregate(bool neg, Type type, int lower, ObjectVector lits, IntVector weights) : 
	Object(neg), type_(type), bounds_(L), lower_(lower), upper_(0)
{
	std::swap(lits, lits_);
	std::swap(weights, weights_);
}

Aggregate::Aggregate(bool neg, Type type, ObjectVector lits, IntVector weights, int upper) : 
	Object(neg), type_(type), bounds_(U), lower_(0), upper_(upper)
{
	std::swap(lits, lits_);
	std::swap(weights, weights_);
}

Aggregate::Aggregate(bool neg, Type type, ObjectVector lits, IntVector weights) : 
	Object(neg), type_(type), bounds_(N), lower_(0), upper_(0)
{
	std::swap(lits, lits_);
	std::swap(weights, weights_);
}

Aggregate::Aggregate(bool neg, Type type) : Object(neg), type_(type), bounds_(N), lower_(0), upper_(0)
{
}

void Aggregate::print_plain(Output *o, std::ostream &out)
{
	if(neg_)
		out << "not ";
	if((bounds_ == L || bounds_ == LU) && type_ != PARITY)
		out << lower_;
	bool comma = false;
	switch(type_)
	{
		case SUM:
			out << " [";
			++(o->stats_).sum;
			break;
		case COUNT:
			out << " {";
			++(o->stats_).count;
			break;
		case MAX:
			out << " #max [";
			++(o->stats_).max;
			break;
		case MIN:
			out << " #min [";
			++(o->stats_).min;
			break;
		case AVG:
			out << " #avg [";
			break;
		case TIMES:
			out << " #times [";
			break;
		case PARITY:
			assert(bounds_ == LU);
			out << (lower_ == 0 ? " #even {" : " #odd {");
			break;
	}
	IntVector::iterator itWeight = weights_.begin();
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		(*it)->print_plain(o, out);
		if(type_ != COUNT && type_ != PARITY)
		{
			out << " = ";
			out << *itWeight++;
		}
	}
	if(type_ == COUNT || type_ == PARITY)
		out << "} ";
	else
		out << "] ";
	if((bounds_ == U || bounds_ == LU) && type_ != PARITY)
		out << upper_;
}

void Aggregate::print(Output *o, std::ostream &out)
{
	unsigned int normalForm = static_cast<PilsOutput*>(o)->getNormalForm();
	bool negativeWeights = false;
	uid_ = o->newUid();
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		(*it)->print(o, out);

	// at first, create weighted literals for all weight using aggregates
	// not for count, and not for weight constraints with non trivial bound in Normalforms 3,4,5
	IntVector uids;
	if((type_ != COUNT) && !(type_ == SUM && (normalForm == 3 || normalForm == 4 || normalForm == 5) && bounds_ != N) )
	{
		ObjectVector::iterator lit = lits_.begin();
		for(IntVector::iterator it = weights_.begin(); it != weights_.end(); it++, ++lit)
		{
			if (*it < 0)
				negativeWeights = true;
			unsigned int id = o->newUid();
			out << "d" << " 3 " << id << " " << (*lit)->getUid() << " " << *it << END_ENTRY << NL;
			uids.push_back(id);
		}
	}
	else
	{
		for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
			uids.push_back((*it)->getUid());
	}



	switch (type_)
	{
		case COUNT:
			{
				if (normalForm == 1 || normalForm == 2)
					throw GrinGoException("Count aggregate/cardinality constraint not allowed in this normal form, please choose normal form 3-7.");
				if (normalForm == 3 || normalForm == 4)
				{
					if (bounds_ == U || bounds_ == LU)
						throw GrinGoException("Count aggregate/cardinality constraint not allowed with non trivial upper bound in this normal form, please choose normal form 5-7.");
					if (lower_ < 0)
						throw GrinGoException("Count aggregate/cardinality constraint not allowed with negative lower bound in this normal form, please choose normal form 5-7.");

				}
//				out << "e";
				break;
			}
		case SUM:
			 {
				if (normalForm == 1 || normalForm == 2)
					throw GrinGoException("Sum aggregate/weight constraint not allowed in this normal form, please choose another output, please choose normal form 3-7");
				if (normalForm == 3 || normalForm == 4)
				{
					if (bounds_ == U || bounds_ == LU)
						throw GrinGoException("Sum aggregate/weight constraint not allowed with non trivial upper bound in this normal form, please choose normal form 5-7.");
					if (lower_ < 0)
						throw GrinGoException("Sum aggregate/weight constraint not allowed with negative lower bound in this normal form, please choose normal form 5-7.");
					if (negativeWeights)
						throw GrinGoException("Sum aggregate/weight constraint not allowed with negative weights in this normal form, please choose normal form 5 or 7.");
				}
				if (normalForm == 6 && negativeWeights)
					throw GrinGoException("Sum aggregate/weight constraint not allowed with negative weights in this normal form, please choose normal form 5 or 7.");
//				out << "f";
				break;
			 }
		case MAX:
			 {
				if (normalForm >=1 && normalForm <= 5)
					throw GrinGoException("Max aggregate not allowed in this normal form, please choose normal form 6 or 7.");
				if (normalForm == 6 && negativeWeights)
					throw GrinGoException("Max aggregate not allowed with negative weights in this normal form, please choose normal form 7.");
//				out << "10";
				break;
			 }
		case MIN:
			 {
				if (normalForm >=1 && normalForm <= 5)
					throw GrinGoException("Min aggregate not allowed in this normal form, please choose normal form 6 or 7.");
				if (normalForm == 6 && negativeWeights)
					throw GrinGoException("Min aggregate not allowed with negative weights in this normal form, please choose normal form 7.");
//				out << "11";
				break;
			 }
		case TIMES:
			 {
				if (normalForm >=1 && normalForm <= 5)
					throw GrinGoException("Times aggregate not allowed in this normal form, please choose normal form 6 or 7.");
				if (normalForm == 6 && negativeWeights)
					throw GrinGoException("Times aggregate not allowed with negative weights in this normal form, please choose normal form 7.");
//				out << "12";
				break;
			 }
		default:
			 {
				 FAIL(true);
			 }
	}

	switch (type_)
	{
		case COUNT:
			{
				// do write a cardinality constraint
				if ((normalForm == 3 || normalForm == 4 || normalForm == 5) && bounds_ != N)
				{
					//if (bounds_ == U || bounds_ == LU) // already checked
					out << "b" << " " << uids.size() + 3<< " " << uid_ << " ";
					unsigned int lower = 0;
					unsigned int upper = 0;
					if (bounds_ == L)
						lower = lower_;
					// with trivial upper bound for Normalform 3 and 4
					// or Normalform 5 without upper bound
					if (normalForm < 5 || (normalForm == 5 && bounds_ == L))
						upper = uids.size();
					else
						upper = upper_;
					out << lower << " " << upper;

					for(IntVector::iterator it = uids.begin(); it != uids.end(); it++)
						out << " " << *it;
					out << END_ENTRY << NL;
					++(o->stats_).count;
				}
				else //write a count aggregate
				{
					out << "e" << " " << uids.size() + 1 << " " << uid_;
					for(IntVector::iterator it = uids.begin(); it != uids.end(); it++)
						out << " " << *it;
					out << END_ENTRY << NL;

					//write bounds with operators

					//lower
					if (bounds_ == L || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "1b" << " " << 3 << " " << uid << " " << uid_ << " " << lower_;
						out << END_ENTRY << NL;
					}

					if (bounds_ == U || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "19" << " " << 3 << " " << uid << " " << uid_ << " " << upper_;
						out << END_ENTRY << NL;
					}
					++(o->stats_).count;
				}
				break;
			}
		case SUM:
			{
				// do write a weight constraint
				if ((normalForm == 3 || normalForm == 4 || normalForm == 5) && bounds_ != N)
				{
					//if (bounds_ == U || bounds_ == LU) // already checked
					out << "c" << " " << uids.size()*2 + 3<< " " << uid_ << " ";
					unsigned int lower = 0;
					unsigned int upper = 0;
					if (bounds_ == L) // else 0
						lower = lower_;
					// with trivial upper bound for Normalform 3 and 4
					// or Normalform 5 without upper bound
					if (normalForm < 5 || (normalForm == 5 && bounds_ == L))
					{
						//trivial upper bound is summing up all positive weights
						for(IntVector::iterator it = weights_.begin(); it != weights_.end(); it++)
							if (*it > 0)
								upper += *it;
					}
					else
						upper = upper_;
					out << lower << " " << upper;

					for(IntVector::iterator it = uids.begin(); it != uids.end(); it++)
						out << " " << *it;
					for(IntVector::iterator it = weights_.begin(); it != weights_.end(); it++)
						out << " " << *it;

					out << END_ENTRY << NL;
					++(o->stats_).sum;
				}
				else //write a sum aggregate
				{
					out << "f" << " " << uids.size() + 1 << " " << uid_;
					for(IntVector::iterator it = uids.begin(); it != uids.end(); it++)
						out << " " << *it;
					out << END_ENTRY << NL;

					//write bounds with operators

					//lower
					if (bounds_ == L || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "1b" << " " << 3 << " " << uid << " " << uid_ << " " << lower_;
						out << END_ENTRY << NL;
					}

					if (bounds_ == U || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "19" << " " << 3 << " " << uid << " " << uid_ << " " << upper_;
						out << END_ENTRY << NL;
					}
					++(o->stats_).sum;
				}
				break;
			}
		case MAX:
			{
					//write a max aggregate
					out << "10" << " " << uids.size() + 1 << " " << uid_;
					for(IntVector::iterator it = uids.begin(); it != uids.end(); it++)
						out << " " << *it;
					out << END_ENTRY << NL;

					//write bounds with operators

					//lower
					if (bounds_ == L || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "1b" << " " << 3 << " " << uid << " " << uid_ << " " << lower_;
						out << END_ENTRY << NL;
					}

					if (bounds_ == U || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "19" << " " << 3 << " " << uid << " " << uid_ << " " << upper_;
						out << END_ENTRY << NL;
					}
				++(o->stats_).max;
				break;
			}
		case MIN:
			{
					//write a min aggregate
					out << "11" << " " << uids.size() + 1 << " " << uid_;
					for(IntVector::iterator it = uids.begin(); it != uids.end(); it++)
						out << " " << *it;
					out << END_ENTRY << NL;

					//write bounds with operators

					//lower
					if (bounds_ == L || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "1b" << " " << 3 << " " << uid << " " << uid_ << " " << lower_;
						out << END_ENTRY << NL;
					}

					if (bounds_ == U || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "19" << " " << 3 << " " << uid << " " << uid_ << " " << upper_;
						out << END_ENTRY << NL;
					}
				++(o->stats_).min;
				break;
			}
		case TIMES:
			{
					//write a times aggregate
					out << "11" << " " << uids.size() + 1 << " " << uid_;
					for(IntVector::iterator it = uids.begin(); it != uids.end(); it++)
						out << " " << *it;
					out << END_ENTRY << NL;

					//write bounds with operators

					//lower
					if (bounds_ == L || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "1b" << " " << 3 << " " << uid << " " << uid_ << " " << lower_;
						out << END_ENTRY << NL;
					}

					if (bounds_ == U || bounds_ == LU)
					{
						unsigned int uid = o->newUid();

						out << "19" << " " << 3 << " " << uid << " " << uid_ << " " << upper_;
						out << END_ENTRY << NL;
					}
			}
	}
}

void Aggregate::addDomain(bool fact)
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		(*it)->addDomain(false);
}

Aggregate::~Aggregate()
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		delete *it;
}

// =============== NS_OUTPUT::Compute ===============
Compute::Compute(ObjectVector &lits, int models) : models_(models)
{
	std::swap(lits, lits_);
}

void Compute::print_plain(Output *o, std::ostream &out)
{
	out << "compute " << models_ << " { ";
	bool comma = false;
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		(*it)->print_plain(o, out);
	}
	out << " }." << NL;
	++(o->stats_).compute;
}

void Compute::print(Output *o, std::ostream &out)
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		(*it)->print(o, out);
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
	{
		out << "7" << " 2 " << o->newUid() << " " << -(*it)->getUid() << END_ENTRY << NL;
	}
	++(o->stats_).compute;
}

Compute::~Compute()
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		delete *it;
}

void Compute::addDomain(bool fact)
{
}

// =============== NS_OUTPUT::Optimize ===============
Optimize::Optimize(Type type, ObjectVector &lits, IntVector &weights) : type_(type)
{
	std::swap(lits, lits_);
	std::swap(weights, weights_);
}

void Optimize::print_plain(Output *o, std::ostream &out)
{
	switch(type_)
	{
		case MINIMIZE:
			out << "minimize [ ";
			break;
		case MAXIMIZE:
			out << "maximize [ ";
			break;
	}
	bool comma = false;
	IntVector::iterator itWeights = weights_.begin();
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++, itWeights++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		(*it)->print_plain(o, out);
		out << " = " << *itWeights;
	}
	out << " ]." << NL;
	++(o->stats_).optimize;
}

void Optimize::print(Output *o, std::ostream &out)
{
	unsigned int normalForm = static_cast<PilsOutput*>(o)->getNormalForm();
	if (normalForm == 1 || normalForm == 2)
		throw GrinGoException("Optimize statement not allowed in this normal form, please choose normal form 3-7.");
	IntVector uids;
	ObjectVector::iterator lit = lits_.begin();
	for(IntVector::iterator it = weights_.begin(); it != weights_.end(); it++, ++lit)
	{
		(*lit)->print(o, out);
		unsigned int id = o->newUid();
		out << "d" << " 3 " << id << " " << (*lit)->getUid() << " ";
	   if(type_ == MAXIMIZE)
			out << -*it;
		else
			out << *it;
		out << END_ENTRY << NL;
		uids.push_back(id);
	}

	uid_ = o->newUid();
	out << "f" << " " << uids.size() + 1 << " " << uid_;
	for (IntVector::const_iterator i = uids.begin(); i != uids.end(); ++i)
	{
		out << " " << *i;
	}
	out << END_ENTRY << NL;
	static_cast<PilsOutput*>(o)->addOptimizedID(uid_);
	++(o->stats_).optimize;
}

Optimize::~Optimize()
{
	for(ObjectVector::iterator it = lits_.begin(); it != lits_.end(); it++)
		delete *it;
}

void Optimize::addDomain(bool fact)
{
}

// =============== NS_OUTPUT::DeltaObject ===============
DeltaObject::DeltaObject()
{
}

void DeltaObject::print(NS_OUTPUT::Output *o, std::ostream &out)
{
	FAIL(true);
}

void DeltaObject::print_plain(NS_OUTPUT::Output *o, std::ostream &out)
{
	out << "delta(" << o->getIncUid() << ")";
}

void DeltaObject::addDomain(bool fact)
{
	FAIL(true);
}

DeltaObject::~DeltaObject()
{
}

