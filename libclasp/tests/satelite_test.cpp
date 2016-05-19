#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <algorithm>
#include <clasp/satelite.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/reader.h>
#ifdef _MSC_VER
#pragma warning (disable : 4267) //  conversion from 'size_t' to unsigned int
#pragma once
#endif


namespace Clasp { namespace Test {

class SatEliteTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SatEliteTest);
	
	CPPUNIT_TEST(testDontAddSatClauses);
	CPPUNIT_TEST(testSimpleSubsume);
	CPPUNIT_TEST(testSimpleStrengthen);
	
	CPPUNIT_TEST(testClauseCreatorAddsToPreprocessor);
	
	CPPUNIT_TEST(testDimacs);
	CPPUNIT_TEST(testFreeze);

	CPPUNIT_TEST(testElimPureLits);
	CPPUNIT_TEST(testDontElimPureLits);
	CPPUNIT_TEST(testExtendModel);
	CPPUNIT_TEST_SUITE_END();	
public:
	SatEliteTest(){
		for (int i = 0; i < 10; ++i) {
			solver.addVar(Var_t::atom_var);
		}
		solver.startAddConstraints();
		pre.setSolver(solver);
	}
	
	void testDontAddSatClauses() {
		LitVec cl;
		cl.push_back(posLit(1)); cl.push_back(posLit(2));
		pre.addClause(cl);
		solver.force(posLit(1), 0);
		CPPUNIT_ASSERT_EQUAL(true, pre.preprocess());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numConstraints());
	}

	void testSimpleSubsume() {
		LitVec cl;
		cl.push_back(posLit(1)); cl.push_back(posLit(2));
		pre.addClause(cl);
		cl.push_back(posLit(3));
		pre.addClause(cl);
		CPPUNIT_ASSERT_EQUAL(true, pre.preprocess());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numConstraints());
	}

	void testSimpleStrengthen() {
		LitVec cl;
		cl.push_back(posLit(1)); cl.push_back(posLit(2));
		pre.addClause(cl);
		cl[0] = negLit(1);
		pre.addClause(cl);
		CPPUNIT_ASSERT_EQUAL(true, pre.preprocess());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numConstraints());
	}

	void testClauseCreatorAddsToPreprocessor() {
		SatElite::SatElite* p = new SatElite::SatElite();
		p->setSolver(solver);
		solver.strategies().satPrePro.reset(p);
		p->options.elimPure = false;
		ClauseCreator nc(&solver);
		nc.start().add(posLit(1)).add(posLit(2)).end();
		CPPUNIT_ASSERT_EQUAL(0u, solver.numConstraints());
		solver.endAddConstraints();
		CPPUNIT_ASSERT_EQUAL(1u, solver.numConstraints());
		CPPUNIT_ASSERT_EQUAL(1u, solver.numBinaryConstraints());
	}

	void testDimacs() {
		std::stringstream prg;
		Solver s;
		s.strategies().satPrePro.reset(new SatElite::SatElite(&s));
		prg << "c simple test case\n"
				<< "p cnf 2 4\n"
			  << "1 2 2 1 0\n"
				<< "1 2 1 2 0\n"
				<< "-1 -2 -1 0\n"
				<< "-2 -1 -2 0\n";
		CPPUNIT_ASSERT(parseDimacs(prg, s, true));
		CPPUNIT_ASSERT(s.numVars() == 2 && s.numAssignedVars() == 0);
		CPPUNIT_ASSERT(s.endAddConstraints());
		CPPUNIT_ASSERT(s.eliminated(2));
	}

	
	void testFreeze() {
		std::stringstream prg;
		Solver s;
		s.strategies().satPrePro.reset(new SatElite::SatElite(&s));
		
		prg << "c simple test case\n"
				<< "p cnf 2 2\n"
			  << "1 2 0\n"
				<< "-1 -2 0\n";
		CPPUNIT_ASSERT(parseDimacs(prg, s, true));
		s.setFrozen(1, true);
		s.setFrozen(2, true);
		CPPUNIT_ASSERT(s.numVars() == 2);
		CPPUNIT_ASSERT(s.endAddConstraints());
		CPPUNIT_ASSERT(s.numConstraints() == 2);
		CPPUNIT_ASSERT(s.numBinaryConstraints() == 2);
	}

	void testElimPureLits() {
		LitVec cl;
		cl.push_back(posLit(1)); cl.push_back(posLit(2));
		pre.addClause(cl);
		pre.options.elimPure	= true;
		pre.preprocess();
		CPPUNIT_ASSERT(0u == solver.numConstraints());
		CPPUNIT_ASSERT(solver.eliminated(1) == true);
	}
	void testDontElimPureLits() {
		LitVec cl;
		cl.push_back(posLit(1)); cl.push_back(posLit(2));
		pre.addClause(cl);
		pre.options.elimPure	= false;
		pre.preprocess();
		CPPUNIT_ASSERT(1u == solver.numConstraints());
		CPPUNIT_ASSERT(solver.eliminated(1) == false);
	}

	void testExtendModel() {
		Solver s; 
		s.strategies().satPrePro.reset(new SatElite::SatElite(&s));
		s.addVar(Var_t::atom_var);
		s.addVar(Var_t::atom_var);
		s.startAddConstraints();
		ClauseCreator nc(&s);
		nc.start().add(negLit(1)).add(posLit(2)).end();
		s.endAddConstraints();
		CPPUNIT_ASSERT(0u == s.numConstraints());
		CPPUNIT_ASSERT(s.eliminated(1) == true && s.numEliminatedVars() == 1);
		// Eliminated vars are initially true
		CPPUNIT_ASSERT_EQUAL(value_true, s.value(1));
		s.assume(negLit(2)) && s.propagate();
		s.search(-1, -1, 1.0);
		// negLit(2) -> negLit(1) -> 1 == F
		CPPUNIT_ASSERT_EQUAL(value_false, s.value(1));
	}
private:
	Solver							solver;
	SatElite::SatElite	pre;
};
CPPUNIT_TEST_SUITE_REGISTRATION(SatEliteTest);
} } 
