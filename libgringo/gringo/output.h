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

#ifndef OUTPUT_H
#define OUTPUT_H

#include <gringo/gringo.h>
#include <gringo/value.h>

namespace gringo
{
	namespace NS_OUTPUT
	{
		typedef std::vector<Object*> ObjectVector;

		class Output
		{
		protected:
			typedef HashMap<ValueVector, int, Value::VectorHash, Value::VectorEqual>::type AtomHash;
			typedef std::vector<AtomHash> AtomLookUp;
		public:
			Output(std::ostream *out);
			virtual void initialize(GlobalStorage *g, SignatureVector *pred);
			virtual void reinitialize();
			virtual int getIncUid();
			virtual void print(NS_OUTPUT::Object *o) = 0;
			virtual void finalize(bool last) = 0;
			std::string atomToString(int id, const ValueVector &values) const;
			virtual bool addAtom(NS_OUTPUT::Atom *r);
			virtual int newUid();
			virtual ~Output();

			void hideAll();
			void setVisible(int id, int arity, bool visible);
			bool isVisible(int uid);
			bool isVisible(int id, int arity);
			
			// must be called if predicates are added after initialize has been called
			void addSignature();
			struct Stats
			{
				enum Language
				{
					UNKNOWN,
					SMODELS,
					TEXT,
					ASPILS
				};

				Language language;
				unsigned int rules;
				unsigned int atoms;
				unsigned int auxAtoms;
				unsigned int count;
				unsigned int sum;
				unsigned int max;
				unsigned int min;
				unsigned int compute;
				unsigned int optimize;
			};
		protected:
			int uids_;
			std::ostream *out_;
			AtomLookUp atoms_;
			SignatureVector *pred_;
			bool hideAll_;
			std::map<Signature, bool> hide_;
			std::vector<bool> visible_;
			GlobalStorage *g_;
		public:
			Stats stats_;
		};
	
		struct Object
		{
			Object();
			Object(bool neg);
			virtual void print(Output *o, std::ostream &out) = 0;
			virtual void print_plain(Output *o, std::ostream &out) = 0;
			virtual void addDomain(bool fact = true) = 0;
			int getUid();
			virtual ~Object();
			
			bool neg_;
			int uid_;
		};

		struct Atom : public Object
		{
			Atom(bool neg, Domain *node, int predUid, const ValueVector &values);
			Atom(bool neg, int predUid, const ValueVector &values);
			Atom(bool neg, int predUid);
			void addDomain(bool fact);
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			
			// TODO: change sth here!
			Domain *node_;
			int  predUid_;
			ValueVector values_;
		};

		struct Rule : public Object
		{
			Rule(Object* head, Object *body);
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			~Rule();
			void addDomain(bool fact);

			Object *head_;
			Object *body_;
		};

		struct Fact : public Object
		{
			Fact(Object *head);
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			~Fact();
			void addDomain(bool fact);

			Object *head_;
		};
		
		struct Integrity : public Object
		{
			Integrity(Object *body);
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			void addDomain(bool fact);
			~Integrity();

			Object *body_;
		};

		struct Conjunction : public Object
		{
			Conjunction(ObjectVector &lits);
			Conjunction();
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			~Conjunction();
			void addDomain(bool fact);

			ObjectVector lits_;
		};

		struct Disjunction : public Object
		{
			Disjunction(ObjectVector &lits);
			Disjunction();
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			~Disjunction();
			void addDomain(bool fact);

			ObjectVector lits_;
		};

		struct Aggregate : public Object
		{
			enum Type   { SUM = 0xf, COUNT = 0xe, MAX = 0x10, MIN=0x11, TIMES=0x12, AVG=0x13, PARITY=0x14 };
			enum Bounds { LU = 3, U = 2, L = 1, N = 0 };
			Aggregate(bool neg, Type type, int lower, ObjectVector lits, IntVector weights, int upper);
			Aggregate(bool neg, Type type, int lower, ObjectVector lits, IntVector weights);
			Aggregate(bool neg, Type type, ObjectVector lits, IntVector weights, int upper);
			Aggregate(bool neg, Type type, ObjectVector lits, IntVector weights);
			Aggregate(bool neg, Type type);
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			void addDomain(bool fact);
			~Aggregate();

			int          type_;
			ObjectVector lits_;
			IntVector    weights_;
			Bounds       bounds_;
			int          lower_;
			int          upper_;
		};
		
		struct Compute : public Object
		{
			Compute(ObjectVector &lits, int models);
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			~Compute();
			void addDomain(bool fact);

			ObjectVector lits_;
			int models_;
		};	
		
		struct Optimize : public Object
		{
			enum Type { MINIMIZE, MAXIMIZE };
			Optimize(Type type, ObjectVector &lits, IntVector &weights);
			void print_plain(Output *o, std::ostream &out);
			void print(Output *o, std::ostream &out);
			~Optimize();
			void addDomain(bool fact);

			int          type_;
			ObjectVector lits_;
			IntVector    weights_;
		};

		struct DeltaObject : public NS_OUTPUT::Object
		{
			DeltaObject();
			void print(NS_OUTPUT::Output *o, std::ostream &out);
			void print_plain(NS_OUTPUT::Output *o, std::ostream &out);
			void addDomain(bool fact = true);
			virtual ~DeltaObject();
		};
	}
}

#endif

