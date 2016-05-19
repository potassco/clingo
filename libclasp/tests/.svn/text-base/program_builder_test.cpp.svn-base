#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include "common.h"
#include <clasp/solver.h>
#include <clasp/program_builder.h>
#include <clasp/unfounded_check.h>
#include <clasp/smodels_constraints.h>
#include <sstream>
using namespace std;
namespace Clasp { namespace Test {
	struct ClauseObserver : public DecisionHeuristic {
		Literal doSelect(Solver&){return Literal();}
		void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType) {
			Clause c;
			for (LitVec::size_type i = 0; i < size; ++i, ++first) {
				c.push_back(*first);
			}
			std::sort(c.begin(), c.end());
			clauses_.push_back(c);
		}
		typedef std::vector<Literal> Clause;
		typedef std::vector<Clause> Clauses;
		Clauses clauses_;
	};

class ProgramBuilderTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ProgramBuilderTest);
	CPPUNIT_TEST(testIgnoreRules);
	CPPUNIT_TEST(testNotAChoice);
	CPPUNIT_TEST(testMergeToSelfblocker);
	CPPUNIT_TEST(testMergeToSelfblocker2);
	CPPUNIT_TEST(testDerivedTAUT);
	CPPUNIT_TEST(testOneLoop);
	CPPUNIT_TEST(testZeroLoop);
	CPPUNIT_TEST(testEqSuccs);
	CPPUNIT_TEST(testEqCompute);
	CPPUNIT_TEST(testFactsAreAsserted);
	CPPUNIT_TEST(testSelfblockersAreAsserted);
	CPPUNIT_TEST(testConstraintsAreAsserted);
	CPPUNIT_TEST(testConflictingCompute);
	CPPUNIT_TEST(testForceUnsuppAtomFails);
	CPPUNIT_TEST(testTrivialConflictsAreDeteced);
	CPPUNIT_TEST(testBuildEmpty);
	CPPUNIT_TEST(testAddOneFact);
	CPPUNIT_TEST(testTwoFactsOnlyOneVar);
	CPPUNIT_TEST(testDontAddOnePredsThatAreNotHeads);
	CPPUNIT_TEST(testDontAddDuplicateBodies);
	CPPUNIT_TEST(testDontAddUnsupported);
	CPPUNIT_TEST(testDontAddUnsupportedNoEq);
	CPPUNIT_TEST(testDontAddUnsupportedExtNoEq);
	CPPUNIT_TEST(testAssertSelfblockers);
	

	CPPUNIT_TEST(testBug);
	CPPUNIT_TEST(testSatBodyBug);
	CPPUNIT_TEST(testAddUnknownAtomToMinimize);
	CPPUNIT_TEST(testWriteWeakTrue);

	CPPUNIT_TEST(testAssertEqSelfblocker);
	CPPUNIT_TEST(testAddClauses);
	CPPUNIT_TEST(testAddCardConstraint);
	CPPUNIT_TEST(testAddWeightConstraint);
	CPPUNIT_TEST(testAddMinimizeConstraint);
	CPPUNIT_TEST(testNonTight);

	CPPUNIT_TEST(testIgnoreCondFactsInLoops);
	CPPUNIT_TEST(testCrEqBug);
	CPPUNIT_TEST(testEqOverChoiceRule);
	CPPUNIT_TEST(testEqOverBodiesOfDiffType);
	CPPUNIT_TEST(testEqOverComp);
	CPPUNIT_TEST(testNoBodyUnification);
	CPPUNIT_TEST(testNoEqAtomReplacement);
	CPPUNIT_TEST(testAllBodiesSameLit);

	CPPUNIT_TEST(testCompLit);
	CPPUNIT_TEST(testFunnySelfblockerOverEqByTwo);

	CPPUNIT_TEST(testRemoveKnownAtomFromWeightRule);
	CPPUNIT_TEST(testMergeEquivalentAtomsInConstraintRule);
	CPPUNIT_TEST(testMergeEquivalentAtomsInWeightRule);
	CPPUNIT_TEST(testBothLitsInConstraintRule);
	CPPUNIT_TEST(testBothLitsInWeightRule);
	CPPUNIT_TEST(testWeightlessAtomsInWeightRule);
	CPPUNIT_TEST(testSimplifyToNormal);
	CPPUNIT_TEST(testSimplifyToCardBug);

	CPPUNIT_TEST(testBPWeight);

	CPPUNIT_TEST(testExtLitsAreFrozen);	
	CPPUNIT_TEST(writeIntegrityConstraint);

	CPPUNIT_TEST(testSimpleIncremental);
	CPPUNIT_TEST(testIncrementalFreeze);
	CPPUNIT_TEST(testIncrementalUnfreezeUnsupp);
	CPPUNIT_TEST(testIncrementalUnfreezeCompute);
	CPPUNIT_TEST(testIncrementalEq);

	CPPUNIT_TEST(testIncrementalCompute);
	CPPUNIT_TEST(testComputeTrueBug);
	CPPUNIT_TEST(testIncrementalStats);
	CPPUNIT_TEST(testIncrementalTransform);

	CPPUNIT_TEST(testBackprop);
	CPPUNIT_TEST(testMergeValue);

	CPPUNIT_TEST_SUITE_END();	
public:
	void setUp()	{ ufs = new DefaultUnfoundedCheck; }
	
	void testIgnoreRules() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(1, true).endRule()  // a :- a.
			.startRule().addHead(2).addToBody(1, true).endRule()  // b :- a.
		;
		CPPUNIT_ASSERT_EQUAL(1u, builder.stats.rules[0]);
	}

	void testNotAChoice() {
		// {b}.
		// {a} :- not b.
		// a :- not b.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule(CHOICERULE).addHead(2).endRule()
			.startRule(CHOICERULE).addHead(1).addToBody(2, false).endRule()
			.startRule().addHead(1).addToBody(2, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		solver.assume(~index[2].lit) && solver.propagate();
		// if b is false, a must be true because a :- not b. is stronger than {a} :- not b.
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[1].lit));
	}
	
	void testMergeToSelfblocker() {
		// a :- not b.
		// b :- a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(2, false).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));
	}
	
	void testMergeToSelfblocker2() {
		// a :- not z.
		// a :- not x.
		// q :- not x.
		// x :- a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "q").setAtomName(3, "x").setAtomName(4, "z")
			.startRule().addHead(1).addToBody(4, false).endRule()
			.startRule().addHead(1).addToBody(3, false).endRule()
			.startRule().addHead(2).addToBody(3, false).endRule()
			.startRule().addHead(3).addToBody(1, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[1].lit));
		CPPUNIT_ASSERT(solver.numVars() == 0);
	}

	void testDerivedTAUT() {
		// {y, z}.
		// a :- not z.
		// x :- a.
		// a :- x, y.
		builder.startProgram(index, ufs)
			.setAtomName(1, "y").setAtomName(2, "z").setAtomName(3, "a").setAtomName(4, "x")
			.startRule(CHOICERULE).addHead(1).addHead(2).endRule()
			.startRule().addHead(3).addToBody(2, false).endRule()
			.startRule().addHead(4).addToBody(3, true).endRule()
			.startRule().addHead(3).addToBody(1, true).addToBody(4, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(solver.numVars() == 2);
	}

	void testOneLoop() {
		// a :- not b.
		// b :- not a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(2, false).endRule()
			.startRule().addHead(2).addToBody(1, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL( 1u, solver.numVars() );
		CPPUNIT_ASSERT_EQUAL( 0u, solver.numConstraints() );
		CPPUNIT_ASSERT( index[1].lit == ~index[2].lit );
	}

	void testZeroLoop() {
		// a :- b.
		// b :- a.
		// a :- not x.
		// x :- not a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "x")
			.startRule().addHead(1).addToBody(2, true).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule().addHead(1).addToBody(3, false).endRule()
			.startRule().addHead(3).addToBody(1, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL( 1u, solver.numVars() );
		CPPUNIT_ASSERT_EQUAL( 0u, solver.numConstraints() );
		CPPUNIT_ASSERT( index[1].lit == index[2].lit );
	}

	void testEqSuccs() {
		// {a,b}.
		// c :- a, b.
		// d :- a, b.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			.startRule(CHOICERULE).addHead(1).addHead(2).endRule()
			.startRule().addHead(3).addToBody(1, true).addToBody(2, true).endRule()
			.startRule().addHead(4).addToBody(1, true).addToBody(2, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL( 3u, solver.numVars() );
		CPPUNIT_ASSERT_EQUAL( 3u, solver.numConstraints() );
		CPPUNIT_ASSERT( index[3].lit == index[4].lit );
	}

	void testEqCompute() {
		// {x}.
		// a :- not x.
		// a :- b.
		// b :- a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "x")
			.startRule(CHOICERULE).addHead(3).endRule()
			.startRule().addHead(1).addToBody(3, false).endRule()
			.startRule().addHead(1).addToBody(2, true).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.setCompute(2, true);
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[1].lit));
	}

	void testFactsAreAsserted() {
		// a :- not x.
		// y.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "x").setAtomName(3, "y")
			.startRule().addHead(1).addToBody(2, false).endRule()	// dynamic fact
			.startRule().addHead(3).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[1].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[3].lit));
	}
	void testSelfblockersAreAsserted() {
		// a :- not a.
		// b :- not a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(1, false).endRule()
			.startRule().addHead(2).addToBody(1, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));
	}
	void testConstraintsAreAsserted() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(2, false).endRule()	// a :- not b.
			.startRule().addHead(2).addToBody(1, false).endRule()	// b :- not a.
			.setCompute(1, false)	// force not a
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[1].lit));
	}
	
	void testConflictingCompute() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(2, false).endRule()	// a :- not b.
			.startRule().addHead(2).addToBody(1, false).endRule()	// b :- not a.
			.setCompute(1, false)	// force not a
			.setCompute(1, true)	// force a
		;
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));
	}
	void testForceUnsuppAtomFails() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(2, false).endRule()	// a :- not b.
			.setCompute(2, true)	// force b
		;
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));
	}

	void testTrivialConflictsAreDeteced() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a")
			.startRule().addHead(1).addToBody(1, false).endRule()	// a :- not a.
			.setCompute(1, true)
		;
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));

	}
	void testBuildEmpty() {
		builder.startProgram(index, ufs);
		builder.endProgram(solver, false);
		builder.writeProgram(str);
		CPPUNIT_ASSERT_EQUAL(0u, solver.numVars());
		CPPUNIT_ASSERT(str.str() == "0\n0\nB+\n0\nB-\n0\n1\n");
	}
	void testAddOneFact() {
		builder.startProgram(index, ufs);
		builder.startRule().addHead(1).endRule().setAtomName(1, "A");
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(0u, solver.numVars());
		builder.writeProgram(str);
		std::string lp = "1 1 0 0 \n0\n1 A\n0\nB+\n1\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(lp, str.str());

		// a fact does not introduce a constraint, it is just a top-level assignment
		CPPUNIT_ASSERT_EQUAL(0u, solver.numConstraints());
	}
	
	void testTwoFactsOnlyOneVar() {
		builder.startProgram(index, ufs)
			.startRule().addHead(1).endRule()
			.startRule().addHead(2).endRule()
			.setAtomName(1, "A").setAtomName(2, "B")
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(0u, solver.numVars());
		builder.writeProgram(str);
		std::string lp = "1 1 0 0 \n1 2 1 0 1 \n0\n1 A\n2 B\n0\nB+\n1\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(lp, str.str());
	}

	void testDontAddOnePredsThatAreNotHeads() {
		// a :- not b, not c.
		// c.
		builder.startProgram(index, ufs)
			.startRule().addHead(1).addToBody(2, false).addToBody(3, false).endRule()
			.startRule().addHead(3).endRule()
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(0u, solver.numVars());
		builder.writeProgram(str);
		std::string lp = "1 3 0 0 \n0\n3 c\n0\nB+\n3\n0\nB-\n0\n1\n"; 
		CPPUNIT_ASSERT_EQUAL(lp, str.str());
	}

	void testDontAddDuplicateBodies() {
		// a :- b, not c.
		// d :- b, not c.
		// b.
		// c.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			.startRule().addHead(1).addToBody(2, true).addToBody(3, false).endRule()
			.startRule().addHead(4).addToBody(2, true).addToBody(3, false).endRule()
			.startRule().addHead(2).addHead(3).endRule()		
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(0u, solver.numVars());
		builder.writeProgram(str);
		std::string lp = "1 2 0 0 \n1 3 1 0 2 \n0\n2 b\n3 c\n0\nB+\n2\n0\nB-\n0\n1\n";
		
		CPPUNIT_ASSERT_EQUAL(lp, str.str());
	}

	void testDontAddUnsupported() {
		// a :- c, b.
		// c :- not d.
		// b :- a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			.startRule().addHead(1).addToBody(3, true).addToBody(2, true).endRule()
			.startRule().addHead(3).addToBody(4, false).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()		
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		std::string lp = "1 3 0 0 \n0";
		CPPUNIT_ASSERT(str.str().find(lp) != std::string::npos);
	}

	void testDontAddUnsupportedNoEq() {
		// a :- c, b.
		// c :- not d.
		// b :- a.
		builder.startProgram(index, ufs,0)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			.startRule().addHead(1).addToBody(3, true).addToBody(2, true).endRule()
			.startRule().addHead(3).addToBody(4, false).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()		
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(2u, solver.numVars());
		builder.writeProgram(str);
		std::string lp = "1 3 0 0 \n0\n3 c\n0\nB+\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(lp, str.str());
	}

	void testDontAddUnsupportedExtNoEq() {
		// a :- not x.
		// c :- a, x.
		// b :- 2 {a, c, not x}.
		// -> 2 {a, c, not x} -> {a}
		builder.startProgram(index, ufs, 0)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x")
			.startRule().addHead(1).addToBody(4, false).endRule() // a :- not x
			.startRule().addHead(3).addToBody(1, true).addToBody(4, true).endRule() // c :- a, x
			.startRule(CONSTRAINTRULE, 2).addHead(2).addToBody(1, true).addToBody(3, true).addToBody(4, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[3].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[1].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[2].lit));
	}

	void testAssertSelfblockers() {
		// a :- b, not c.
		// c :- b, not c.
		// b.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
			.startRule().addHead(1).addToBody(2, true).addToBody(3, false).endRule()
			.startRule().addHead(3).addToBody(2, true).addToBody(3, false).endRule()
			.startRule().addHead(2).endRule()		
		;
		// Program is unsat because b must be true and false at the same time.
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));
	}

	void testBug() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "d").setAtomName(2, "c").setAtomName(3, "b").setAtomName(4, "a")
			.startRule().addHead(1).addToBody(2, true).endRule()								// d :- c
			.startRule().addHead(2).addToBody(3, true).addToBody(1, true).endRule() // c :- b, d.
			.startRule().addHead(3).addToBody(4, true).endRule()								// b:-a.
			.startRule().addHead(4).addToBody(3, false).endRule()								// a:-not b.
		;
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));
	}

	void testSatBodyBug() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			.startRule(CHOICERULE).addHead(3).addHead(2).addHead(1).endRule()
			.startRule().addHead(1).endRule()		// a.
			.startRule(CONSTRAINTRULE).setBound(1).addHead(2).addToBody(1, true).addToBody(3, true).endRule() // b :- 1 {a, c}.
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(value_free, solver.value(index[3].lit.var()));
	}

	void testAddUnknownAtomToMinimize() {
		builder.startProgram(index, ufs)
			.startRule(OPTIMIZERULE).addToBody(1, true, 1).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(true, builder.hasMinimize());
	}

	void testWriteWeakTrue() {
		// {z}.
		// x :- not y, z.
		// y :- not x.
		// compute{x}.
		builder.startProgram(index, ufs,-1)
			.setAtomName(1, "x").setAtomName(2, "y").setAtomName(3, "z")
			.startRule(CHOICERULE).addHead(3).endRule()
			.startRule().addHead(1).addToBody(2, false).addToBody(3, true).endRule()
			.startRule().addHead(2).addToBody(1, false).endRule()
			.setCompute(1, true)
		; 
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		builder.writeProgram(str);
		std::string bp("B+\n1\n");
		CPPUNIT_ASSERT(str.str().find(bp) != std::string::npos);
	}

	void testAssertEqSelfblocker() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "x").setAtomName(3, "y").setAtomName(4, "q").setAtomName(5, "r")
			.startRule().addHead(1).addToBody(2, false).addToBody(3, false).endRule()	// a :- not x, not y.
			.startRule().addHead(1).addToBody(4, false).addToBody(5, false).endRule()	// a :- not q, not r.
			.startRule().addHead(2).addToBody(3, false).endRule() // x :- not y.
			.startRule().addHead(3).addToBody(2, false).endRule()	// y :- not x.
			.startRule().addHead(4).addToBody(5, false).endRule() // q :- not r.
			.startRule().addHead(5).addToBody(4, false).endRule()	// r :- not q.								
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(2u, solver.numVars());
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[1].lit));
	}

	void testAddClauses() {
		ClauseObserver* o = new ClauseObserver;
		solver.strategies().heuristic.reset(o);
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
			.startRule().addHead(1).addToBody(2, false).endRule()								// a :- not b.
			.startRule().addHead(1).addToBody(2, true).addToBody(3, true).endRule() // a :- b, c.
			.startRule().addHead(2).addToBody(1, false).endRule()								// b :- not a.
			.startRule().addHead(3).addToBody(2, false).endRule()								// c :- not b.
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(3u, solver.numVars());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numAssignedVars());
	
		CPPUNIT_ASSERT_EQUAL(8u, solver.numConstraints());

		Var bodyNotB = 1;
		Var bodyBC = 3;
		CPPUNIT_ASSERT_EQUAL(Var_t::body_var, solver.type(3));
		Literal a = index[1].lit;
		CPPUNIT_ASSERT(index[2].lit == ~index[1].lit);

		// a - HeadClauses
		ClauseObserver::Clause ac;
		ac.push_back(~a);
		ac.push_back(posLit(bodyNotB));
		ac.push_back(posLit(bodyBC));
		std::sort(ac.begin(), ac.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), ac) != o->clauses_.end());
		
		// bodyNotB - Body clauses
		ClauseObserver::Clause cl;
		cl.push_back(negLit(bodyNotB)); cl.push_back(~index[2].lit);
		std::sort(cl.begin(), cl.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), cl) != o->clauses_.end());
		cl.clear();
		cl.push_back(posLit(bodyNotB)); cl.push_back(index[2].lit);
		std::sort(cl.begin(), cl.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), cl) != o->clauses_.end());
		cl.clear();
		cl.push_back(negLit(bodyNotB)); cl.push_back(a);
		std::sort(cl.begin(), cl.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), cl) != o->clauses_.end());
		
		// bodyBC - Body clauses
		cl.clear();
		cl.push_back(negLit(bodyBC)); cl.push_back(index[2].lit);
		std::sort(cl.begin(), cl.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), cl) != o->clauses_.end());
		cl.clear();
		cl.push_back(negLit(bodyBC)); cl.push_back(index[3].lit);
		std::sort(cl.begin(), cl.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), cl) != o->clauses_.end());
		cl.clear();
		cl.push_back(posLit(bodyBC)); cl.push_back(~index[2].lit); cl.push_back(~index[3].lit);
		std::sort(cl.begin(), cl.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), cl) != o->clauses_.end());
		cl.clear();
		cl.push_back(negLit(bodyBC)); cl.push_back(a);
		std::sort(cl.begin(), cl.end());
		CPPUNIT_ASSERT(std::find(o->clauses_.begin(), o->clauses_.end(), cl) != o->clauses_.end());
	}

	void testAddCardConstraint() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			// a :- 1 { not b, c, d }
			// {b,c}.
			.startRule(CONSTRAINTRULE).setBound(1).addHead(1).addToBody(2, false).addToBody(3, true).addToBody(4, true).endRule()
			.startRule(CHOICERULE).addHead(2).addHead(3).endRule();
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(3u, solver.numVars());

		builder.writeProgram(str);
		std::string exp = "2 1 2 1 1 2 3 \n3 2 2 3 0 0 \n0\n1 a\n2 b\n3 c\n0\nB+\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(exp, str.str());		
	}

	void testAddWeightConstraint() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			// a :- 2 [not b=1, c=2, d=2 ]
			.startRule(WEIGHTRULE).setBound(2).addHead(1).addToBody(2, false, 1).addToBody(3, true, 2).addToBody(4, true, 2).endRule()
			.startRule(CHOICERULE).addHead(2).addHead(3).endRule();
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(3u, solver.numVars());

		builder.writeProgram(str);
		std::string exp = "5 1 2 2 1 2 3 1 2 \n3 2 2 3 0 0 \n0\n1 a\n2 b\n3 c\n0\nB+\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(exp, str.str());		
	}
	void testAddMinimizeConstraint() {
		delete ufs; ufs = 0;
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule(BASICRULE).addHead(1).addToBody(2, false).endRule()
			.startRule(BASICRULE).addHead(2).addToBody(1, false).endRule()
			.startRule(OPTIMIZERULE).addToBody(1, true).endRule()
			.startRule(OPTIMIZERULE).addToBody(2, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		MinimizeConstraint* c1 = builder.createMinimize(solver);
		Solver solver2;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver2, false));
		MinimizeConstraint* c2 = builder.createMinimize(solver2);

		CPPUNIT_ASSERT(c1 != 0 && c2 != 0 && c1 != c2);
		c1->destroy();
		c2->destroy();
		builder.writeProgram(str);
		std::stringstream exp; 
		exp
			<< "6 0 1 0 1 1 \n"
			<< "6 0 1 0 2 1 \n"
			<< "1 1 1 1 2 \n"
			<< "1 2 1 1 1 \n"
			<< "0\n1 a\n2 b\n0\nB+\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(exp.str(), str.str());
	}

	void testNonTight() {
		// p :- q.
		// q :- p.
		// p :- not a.
		// q :- not a.
		// a :- not p.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "p").setAtomName(3, "q")
			.startRule().addHead(2).addToBody(3, true).endRule()
			.startRule().addHead(3).addToBody(2, true).endRule()
			.startRule().addHead(2).addToBody(1, false).endRule()
			.startRule().addHead(3).addToBody(1, false).endRule()
			.startRule().addHead(1).addToBody(2, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT( builder.stats.sccs != 0 );
	}

	void testIgnoreCondFactsInLoops() {
		// {a}.
		// b :- a.
		// a :- b.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule(CHOICERULE).addHead(1).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule().addHead(1).addToBody(2, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT( builder.stats.sccs == 0);
	}

	void testCrEqBug() {
		// a.
		// {b}.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).endRule()
			.startRule(CHOICERULE).addHead(2).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numVars());
		CPPUNIT_ASSERT_EQUAL(value_free, solver.value(index[2].lit.var()));
	}

	void testEqOverChoiceRule() {
		// {a}.
		// b :- a.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule(CHOICERULE).addHead(1).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()	
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numVars());
		builder.writeProgram(str);
		std::stringstream exp; 
		exp
			<< "3 1 1 0 0 \n"
			<< "1 2 1 0 1 \n"
			<< "0\n1 a\n2 b\n0\nB+\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(exp.str(), str.str());
	}

	void testEqOverComp() {
		// x1 :- not x2.
		// x1 :- x2.
		// x2 :- not x3.
		// x3 :- not x1.
		builder.startProgram(index, ufs)
			.setAtomName(1, "x1").setAtomName(2, "x2").setAtomName(3, "x3")
			.startRule().addHead(1).addToBody(2, false).endRule()
			.startRule().addHead(1).addToBody(2, true).endRule()
			.startRule().addHead(2).addToBody(3, false).endRule()
			.startRule().addHead(3).addToBody(1, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(index[1].lit, index[2].lit);
		CPPUNIT_ASSERT(solver.numFreeVars() == 0 && solver.isTrue(index[1].lit));
	}

	void testEqOverBodiesOfDiffType() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "z").setAtomName(2, "y").setAtomName(3, "x").setAtomName(4, "t")
			.startRule(CHOICERULE).addHead(1).addHead(2).endRule() // {z,y}
			.startRule(CONSTRAINTRULE,2).addHead(4).addToBody(1,true).addToBody(2,true).addToBody(3,true).endRule()
			.startRule().addHead(3).addToBody(4,true).endRule()
			.setCompute(2, false)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(3u >= solver.numVars());
		CPPUNIT_ASSERT(solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT(solver.isFalse(index[3].lit));
		CPPUNIT_ASSERT(solver.isFalse(index[4].lit));
	}


	void testNoBodyUnification() {
		// {x, y, z}.
		// p :- x, s.
		// p :- y.
		// q :- not p.
		// r :- not q.
		// s :- p.
		// s :- z. 
		builder.startProgram(index, ufs)
			.setAtomName(1, "x").setAtomName(2, "y").setAtomName(3, "z")
			.setAtomName(4, "p").setAtomName(5, "q").setAtomName(6, "r").setAtomName(7, "s")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).endRule()
			.startRule().addHead(4).addToBody(1, true).addToBody(7,true).endRule()
			.startRule().addHead(4).addToBody(2, true).endRule()
			.startRule().addHead(5).addToBody(4, false).endRule()
			.startRule().addHead(6).addToBody(5, false).endRule()
			.startRule().addHead(7).addToBody(4, true).endRule()
			.startRule().addHead(7).addToBody(3, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		solver.assume(~index[2].lit);	// ~y
		solver.propagate();
		solver.assume(~index[3].lit);	// ~z
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[7].lit));
	}

	void testNoEqAtomReplacement() {
		// {x, y}.
		// p :- x.
		// p :- y.
		// q :- not p.
		// r :- not q.
		builder.startProgram(index, ufs)
			.setAtomName(1, "x").setAtomName(2, "y").setAtomName(3, "p")
			.setAtomName(4, "q").setAtomName(5, "r")
			.startRule(CHOICERULE).addHead(1).addHead(2).endRule()
			.startRule().addHead(3).addToBody(1, true).endRule()
			.startRule().addHead(3).addToBody(2, true).endRule()
			.startRule().addHead(4).addToBody(3, false).endRule()
			.startRule().addHead(5).addToBody(4, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		solver.assume(~index[1].lit);	// ~x
		solver.propagate();
		solver.assume(~index[2].lit);	// ~y
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[3].lit));
	}

	void testAllBodiesSameLit() {
		// {z}.
		// r :- not z.
		// q :- not r.
		// s :- r.
		// s :- not q.
		// r :- s.
		builder.startProgram(index, ufs)
			.setAtomName(1, "z").setAtomName(2, "r").setAtomName(3, "q").setAtomName(4, "s")
			.startRule(CHOICERULE).addHead(1).endRule()
			.startRule().addHead(2).addToBody(1, false).endRule()
			.startRule().addHead(3).addToBody(2, false).endRule()
			.startRule().addHead(4).addToBody(2, true).endRule()
			.startRule().addHead(4).addToBody(3, false).endRule()
			.startRule().addHead(2).addToBody(4, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(index[2].lit, index[4].lit);
		CPPUNIT_ASSERT(index[1].lit != index[3].lit);
		solver.assume(index[1].lit) && solver.propagate();
		CPPUNIT_ASSERT(solver.value(index[3].lit.var()) == value_free);
		solver.assume(~index[3].lit) && solver.propagate();
		CPPUNIT_ASSERT(solver.numFreeVars() == 0 && solver.isTrue(index[2].lit));
	}
	
	void testCompLit() {
		// {y}.
		// a :- not x.
		// x :- not a.
		// b :- a, x.
		// c :- a, y, not x
		// -> a == ~x -> {a,x} = F -> {a, not x} = {a, y}
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "x").setAtomName(4, "y").setAtomName(5, "c")
			.startRule(CHOICERULE).addHead(4).endRule()
			.startRule().addHead(1).addToBody(3, false).endRule()
			.startRule().addHead(3).addToBody(1, false).endRule()
			.startRule().addHead(2).addToBody(1, true).addToBody(3, true).endRule()
			.startRule().addHead(5).addToBody(1, true).addToBody(4, true).addToBody(3, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(3u, solver.numVars());
		CPPUNIT_ASSERT(solver.isFalse(index[2].lit));
	}

	void testFunnySelfblockerOverEqByTwo() {
		// {x,y,z}.
		// q :- x, y.
		// d :- x, y.
		// c :- y, z.
		// a :- y, z.
		// c :- q, not c.
		// r :- d, not a.
		// s :- x, r.
		// -> q == d, c == a -> {d, not a} == {q, not c} -> F
		// -> r == s are both unsupported!
		builder.startProgram(index, ufs)
			.setAtomName(1, "x").setAtomName(2, "y").setAtomName(3, "z").setAtomName(4, "q")
			.setAtomName(5, "d").setAtomName(6, "c").setAtomName(7, "a").setAtomName(8, "r").setAtomName(9, "s")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).endRule()
			.startRule().addHead(4).addToBody(1, true).addToBody(2,true).endRule()
			.startRule().addHead(5).addToBody(1, true).addToBody(2,true).endRule()
			.startRule().addHead(6).addToBody(2, true).addToBody(3,true).endRule()
			.startRule().addHead(7).addToBody(2, true).addToBody(3,true).endRule()
			.startRule().addHead(6).addToBody(4, true).addToBody(6, false).endRule()
			.startRule().addHead(8).addToBody(5, true).addToBody(7, false).endRule()
			.startRule().addHead(9).addToBody(1, true).addToBody(8, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(solver.isFalse(index[8].lit));
		CPPUNIT_ASSERT(solver.isFalse(index[9].lit));
	}
	
	void testRemoveKnownAtomFromWeightRule() {
		// {q, r}.
		// a.
		// x :- 5 [a = 2, not b = 2, q = 1, r = 1].
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "q").setAtomName(4, "r").setAtomName(5, "x")
			.startRule(CHOICERULE).addHead(3).addHead(4).endRule()
			.startRule().addHead(1).endRule()
			.startRule(WEIGHTRULE).addHead(5).setBound(5).addToBody(1, true,2).addToBody(2, false,2).addToBody(3, true).addToBody(4, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		// {q, r}.
		// a. 
		// x :- 1 [ q=1, r=1 ].
		std::stringstream exp; 
		exp
			<< "3 2 3 4 0 0 \n"
			<< "1 1 0 0 \n"
			<< "5 5 1 2 0 3 4 1 1 \n"
			<< "0\n1 a\n3 q\n4 r\n5 x\n0\nB+\n1\n0\nB-\n0\n1\n";
		CPPUNIT_ASSERT_EQUAL(exp.str(), str.str());
	}

	void testMergeEquivalentAtomsInConstraintRule() {
		// {a, c}.
		// a :- b.
		// b :- a.
		// x :-  2 {a, b, c}.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x")
			.startRule(CHOICERULE).addHead(1).addHead(3).endRule()
			.startRule().addHead(1).addToBody(2, true).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule(CONSTRAINTRULE).addHead(4).setBound(2).addToBody(1, true).addToBody(2, true).addToBody(3, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		std::string x = str.str();
		// x :-  2 [a=2, c].
		CPPUNIT_ASSERT(x.find("5 4 2 2 0 1 3 2 1") != std::string::npos);
	}

	void testMergeEquivalentAtomsInWeightRule() {
		// {a, c, d}.
		// a :- b.
		// b :- a.
		// x :-  3 [a = 1, c = 4, b = 2, d=1].
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x").setAtomName(5, "d")
			.startRule(CHOICERULE).addHead(1).addHead(3).addHead(5).endRule()
			.startRule().addHead(1).addToBody(2, true).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule(WEIGHTRULE).addHead(4).setBound(3).addToBody(1, true,1).addToBody(3, true,4).addToBody(2, true,2).addToBody(5, true, 1).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		std::string x = str.str();
		// x :-  3 [a=3, c=3,d=1].
		CPPUNIT_ASSERT(x.find("5 4 3 3 0 1 3 5 3 3 1") != std::string::npos);
	}


	void testBothLitsInConstraintRule() {
		// {a}.
		// b :- a.
		// c :- b.
		// x :-  1 {a, b, not b, not c}.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x")
			.startRule(CHOICERULE).addHead(1).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule().addHead(3).addToBody(2, true).endRule()
			.startRule(CONSTRAINTRULE).addHead(4).setBound(1).addToBody(1, true).addToBody(2, false).addToBody(2, true,2).addToBody(3,false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		std::string x = str.str();
		// x :-  1 [a=2, not a=2].
		CPPUNIT_ASSERT(x.find("5 4 1 2 1 1 1 2 2") != std::string::npos);
	}

	void testBothLitsInWeightRule() {
		// {a, d}.
		// b :- a.
		// c :- b.
		// x :-  3 [a=3, not b=1, not c=3, d=2].
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x").setAtomName(5, "d")
			.startRule(CHOICERULE).addHead(1).addHead(5).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule().addHead(3).addToBody(2, true).endRule()
			.startRule(WEIGHTRULE).addHead(4).setBound(3).addToBody(1, true,3).addToBody(2, false,1).addToBody(3,false,3).addToBody(5, true, 2).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		std::string x = str.str();
		// x :-  3 [a=3, not a=4, d=2].
		CPPUNIT_ASSERT(x.find("5 4 3 3 1 1 1 5 4 3 2") != std::string::npos);
	}

	void testWeightlessAtomsInWeightRule() {
		// {a, b, c}.
		// x :-  1 [a=1, b=1, c=0].
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).endRule()
			.startRule(WEIGHTRULE).addHead(4).setBound(1).addToBody(1, true,1).addToBody(2, true,1).addToBody(3,true,0).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		std::string x = str.str();
		// x :-  1 {a, b}.
		CPPUNIT_ASSERT(x.find("2 4 2 0 1 1 2 ") != std::string::npos);
	}

	void testSimplifyToNormal() {
		// {a}.
		// b :-  2 [a=2,not c=1].
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
			.startRule(CHOICERULE).addHead(1).endRule()
			.startRule(WEIGHTRULE).addHead(2).setBound(2).addToBody(1, true,2).addToBody(3, false,1).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		builder.writeProgram(str);
		std::string x = str.str();
		// b :-  a.
		CPPUNIT_ASSERT(x.find("1 2 1 0 1 ") != std::string::npos);
	}

	void testSimplifyToCardBug() {
		// x1.
		// x2.
		// t :- 168 [not x1 = 576, not x2=381].
		// compute { not t }.
		builder.startProgram(index, ufs)
			.setAtomName(1, "x1").setAtomName(2, "x2").setAtomName(3, "t")
			.startRule().addHead(1).addHead(2).endRule()
			.startRule(WEIGHTRULE).addHead(3).setBound(168).addToBody(1,false,576).addToBody(2,false,381).endRule()
			.setCompute(3, false)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT(solver.numFreeVars() == 0);
	}

	void testBPWeight() {
		// {a, b, c, d}.
		// x :-  2 [a=1, b=2, not c=2, d=1].
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d").setAtomName(5, "x")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).addHead(4).endRule()
			.startRule(WEIGHTRULE).addHead(5).setBound(2).addToBody(1, true,1).addToBody(2, true,2).addToBody(3,false,2).addToBody(4, true, 1).endRule()
			.setCompute(5, false)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[3].lit));
	}

	void testExtLitsAreFrozen() {
		// {a, b, c, d, e, f, g}.
		// x :- 1 {b, c}.
		// y :- 2 [c=1, d=2, e=1].
		// minimize {f, g}.
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
			.setAtomName(4, "d").setAtomName(5, "e").setAtomName(6, "f")
			.setAtomName(7, "g").setAtomName(8, "x").setAtomName(9, "y")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).addHead(4).addHead(5).addHead(6).addHead(7).endRule()
			.startRule(CONSTRAINTRULE,1).addHead(8).addToBody(2, true).addToBody(3,true).endRule()
			.startRule(WEIGHTRULE,2).addHead(9).addToBody(3, true,1).addToBody(4,true,2).addToBody(5, true,1).endRule()
			.startRule(OPTIMIZERULE).addToBody(6,true).addToBody(7,true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		CPPUNIT_ASSERT_EQUAL(false, solver.frozen(index[1].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[2].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[3].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[4].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[5].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[6].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[7].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[8].lit.var()));
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(index[9].lit.var()));

	}

	void writeIntegrityConstraint() {
		builder.startProgram(index, ufs)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "x")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).endRule()
			.startRule(BASICRULE).addHead(1).addToBody(3, true).addToBody(2, false).endRule()
			.startRule(BASICRULE).addHead(2).addToBody(3, true).addToBody(2, false).endRule();
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, false));
		
		builder.writeProgram(str);
		
		// falseAtom :- x, not b.
		CPPUNIT_ASSERT(str.str().find("1 4 2 1 2 3") != std::string::npos);
		// compute {not falseAtom}.
		CPPUNIT_ASSERT(str.str().find("B-\n4") != std::string::npos);
	}

	void testSimpleIncremental() {
		builder.startProgram(index, ufs);
		// I1: 
		// a :- not b.
		// b :- not a.
		builder.updateProgram()
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(2, false).endRule()
			.startRule().addHead(2).addToBody(1, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(solver.numVars() == 1);
		CPPUNIT_ASSERT(index[1].lit == ~index[2].lit);
	
		// I2: 
		// c :- a, not d.
		// d :- b, not c.
		// x :- d.
		builder.updateProgram()
			.setAtomName(3, "c").setAtomName(4, "d").setAtomName(5, "x")
			.startRule().addHead(3).addToBody(1, true).addToBody(4, false).endRule()
			.startRule().addHead(4).addToBody(2, true).addToBody(3, false).endRule()
			.startRule().addHead(5).addToBody(4, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(solver.numVars() == 3);
		CPPUNIT_ASSERT(index[1].lit == ~index[2].lit);
		CPPUNIT_ASSERT(index[5].lit == index[4].lit);
	}

	void testIncrementalFreeze() {
		builder.startProgram(index, ufs, 0);	// no-eq preprocessing
		// I1:
		// {y}.
		// a :- not x.
		// b :- a, y.
		// freeze(x)
		builder.updateProgram()
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "x").setAtomName(4, "y")
			.startRule(CHOICERULE).addHead(4).endRule()
			.startRule().addHead(1).addToBody(3, false).endRule()
			.startRule().addHead(2).addToBody(1, true).addToBody(4, true).endRule()
			.freeze(3)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(index[3].lit != negLit(0));
		solver.assume(index[3].lit);
		solver.propagate();
		CPPUNIT_ASSERT(solver.isFalse(index[2].lit));
		solver.undoUntil(0);
		// I2: 
		// {z}.
		// x :- z.
		// unfreeze(x)
		builder.updateProgram()
			.setAtomName(5, "z")
			.startRule(CHOICERULE).addHead(5).endRule()
			.startRule().addHead(3).addToBody(5, true).endRule()
			.unfreeze(3)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		solver.assume(index[5].lit);
		solver.propagate();
		CPPUNIT_ASSERT(solver.isFalse(index[2].lit));
		solver.undoUntil(0);
		solver.assume(~index[5].lit);	// ~z
		solver.propagate();
		CPPUNIT_ASSERT(solver.isFalse(index[3].lit));
		solver.assume(index[4].lit);	// y
		solver.propagate();
		CPPUNIT_ASSERT(solver.isTrue(index[2].lit));
	}

	void testIncrementalUnfreezeUnsupp() {
		builder.startProgram(index, ufs, 0);	// no-eq preprocessing
		// I1:
		// a :- not x.
		// freeze(x)
		builder.updateProgram()
			.setAtomName(1, "a").setAtomName(2, "x")
			.startRule().addHead(1).addToBody(2, false).endRule()
			.freeze(2)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		// I2: 
		// x :- y.
		// y :- x.
		// -> unfreeze(x)
		builder.updateProgram()
			.setAtomName(3, "y")
			.startRule().addHead(3).addToBody(2, true).endRule()
			.startRule().addHead(2).addToBody(3, true).endRule()
			.unfreeze(2)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(solver.isTrue(index[1].lit));
		CPPUNIT_ASSERT(solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT(solver.isFalse(index[3].lit));
	}

	void testIncrementalUnfreezeCompute() {
		builder.startProgram(index, ufs, 0);	// no-eq preprocessing
		// I1:
		// {z}.
		// a :- x, y.
		// x :- z.
		// freeze(y)
		builder.updateProgram()
			.setAtomName(1, "a").setAtomName(2, "x").setAtomName(3, "y").setAtomName(4, "z")
			.startRule(CHOICERULE).addHead(4).endRule()
			.startRule().addHead(1).addToBody(2,true).addToBody(3, true).endRule()
			.startRule().addHead(2).addToBody(4,true).endRule()
			.freeze(3)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(9u, solver.numConstraints());
		
		builder.updateProgram();
		builder.unfreeze(3);
		builder.setCompute(3, false);
		
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT_EQUAL(4u, solver.numConstraints());
	}

	void testIncrementalCompute() {
		builder.startProgram(index, ufs, 0);	// no-eq preprocessing
		// I1: 
		// {a, b}.
		// FALSE :- a, b.
		builder.updateProgram()
			.setAtomName(1, "FALSE").setAtomName(2, "a").setAtomName(3, "b")
			.startRule(CHOICERULE).addHead(2).addHead(3).endRule()
			.startRule().addHead(1).addToBody(2, true).addToBody(3, true).endRule()
			.setCompute(1, false)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		// I2:
		// {c}.
		// FALSE :- a, c.
		builder.updateProgram()
			.setAtomName(4, "c")
			.startRule(CHOICERULE).addHead(4).endRule()
			.startRule().addHead(1).addToBody(2, true).addToBody(4, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		solver.assume(index[2].lit);
		solver.propagate();
		CPPUNIT_ASSERT(solver.isFalse(index[3].lit));
		CPPUNIT_ASSERT(solver.isFalse(index[4].lit));
	}

	void testIncrementalEq() {
		builder.startProgram(index, ufs, -1);
		builder.updateProgram()
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3,"x").setAtomName(4, "y")
			.startRule(CHOICERULE).addHead(3).addHead(4).endRule() // {x, y}
			.startRule().addHead(1).addToBody(3, true).endRule() // a :- x.
			.startRule().addHead(1).addToBody(4, true).endRule() // a :- y.
			.startRule().addHead(2).addToBody(1, true).endRule() // b :- a.
		;
		// b == a
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		builder.writeProgram(str);
		CPPUNIT_ASSERT(str.str().find("1 2 1 0 1") != std::string::npos);
		builder.updateProgram()
			.setAtomName(5, "c")
			.startRule().addHead(5).addToBody(1, true).addToBody(2, true).endRule() // c :- a,b --> c :- a
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		builder.writeProgram(str);
		CPPUNIT_ASSERT(str.str().find("1 5 1 0 1") != std::string::npos);

	}

	void testComputeTrueBug() {
		// a :- not b.
		// b :- a.
		// a :- y.
		// y :- a.
		// compute{a}.
		builder.startProgram(index, ufs, -1)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "y")
			.startRule().addHead(1).addToBody(2, false).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule().addHead(1).addToBody(3, true).endRule()
			.startRule().addHead(3).addToBody(1, true).endRule()
			.setCompute(1, true)
		;
		CPPUNIT_ASSERT_EQUAL(false, builder.endProgram(solver, true));
	}

	void testIncrementalStats() {
		PreproStats incStats;
		builder.startProgram(index, ufs, 0);
		// I1:
		// a :- not b.
		// b :- not a.
		// freeze(c).
		builder.updateProgram()
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
			.startRule().addHead(1).addToBody(2, false).endRule()
			.startRule().addHead(2).addToBody(1, false).endRule()
			.freeze(3)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		incStats = builder.stats;
		CPPUNIT_ASSERT_EQUAL(uint32(3), incStats.atoms);
		CPPUNIT_ASSERT_EQUAL(uint32(2), incStats.bodies);
		CPPUNIT_ASSERT_EQUAL(uint32(2), incStats.rules[0]);
		CPPUNIT_ASSERT_EQUAL(uint32(0), incStats.ufsNodes);
		CPPUNIT_ASSERT_EQUAL(uint32(0), incStats.sccs);
		
		// I2:
		// {c, z}
		// x :- not c.
		// x :- y, z.
		// y :- x, z.
		builder.updateProgram()
			.setAtomName(4, "x").setAtomName(5, "y").setAtomName(6, "z")
			.startRule(CHOICERULE).addHead(3).addHead(6).endRule()
			.startRule().addHead(4).addToBody(3, false).endRule()
			.startRule().addHead(4).addToBody(5, true).addToBody(6, true).endRule()
			.startRule().addHead(5).addToBody(4, true).addToBody(6, true).endRule()
			.unfreeze(3)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		incStats.accu(builder.stats);
		CPPUNIT_ASSERT_EQUAL(uint32(6), incStats.atoms);
		CPPUNIT_ASSERT_EQUAL(uint32(6), incStats.bodies);
		CPPUNIT_ASSERT_EQUAL(uint32(6), incStats.rules[0]);
		CPPUNIT_ASSERT_EQUAL(uint32(1), incStats.sccs);
		// I3:
		// a :- x, not r.
		// r :- not a.
		// a :- b.
		// b :- a, not z.
		builder.updateProgram()
			.setAtomName(7, "a").setAtomName(8, "b").setAtomName(9, "r")
			.startRule().addHead(7).addToBody(4, true).addToBody(9, false).endRule()
			.startRule().addHead(9).addToBody(7, false).endRule()
			.startRule().addHead(7).addToBody(8, true).endRule()
			.startRule().addHead(8).addToBody(7, true).addToBody(6, false).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		incStats.accu(builder.stats);
		CPPUNIT_ASSERT_EQUAL(uint32(9), incStats.atoms);
		CPPUNIT_ASSERT_EQUAL(uint32(10), incStats.bodies);
		CPPUNIT_ASSERT_EQUAL(uint32(10), incStats.rules[0]);
		CPPUNIT_ASSERT_EQUAL(uint32(2), incStats.sccs);
	}

	void testIncrementalTransform() {
		builder.setExtendedRuleMode(ProgramBuilder::mode_transform);
		builder.startProgram(index, ufs, 0);
		// I1:
		// {a}.
		// --> 
		// a :- not a'
		// a':- not a.
		builder.updateProgram()
			.setAtomName(1, "a")
			.startRule(CHOICERULE).addHead(1).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(ufs->nodes() == 0);
		// I2:
		// b :- a.
		// b :- c.
		// c :- b.
		// NOTE: b must not have the same id as a'
		builder.updateProgram()
			.setAtomName(2, "b").setAtomName(3, "c")
			.startRule().addHead(2).addToBody(1, true).endRule()
			.startRule().addHead(2).addToBody(3, true).endRule()
			.startRule().addHead(3).addToBody(2, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		CPPUNIT_ASSERT(ufs->nodes() != 0);
	}

	void testBackprop() {
		builder.startProgram(index, ufs, -1)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			.setAtomName(5, "x").setAtomName(6, "y").setAtomName(7, "z").setAtomName(8, "_false")
			.startRule(CHOICERULE).addHead(5).addHead(6).addHead(7).endRule()       // {x,y,z}.
			.startRule().addHead(4).addToBody(5, true).addToBody(1, true).endRule() // d :- x,a
			.startRule().addHead(1).addToBody(6, true).addToBody(4, true).endRule() // a :- y,d
			.startRule().addHead(2).addToBody(5, true).addToBody(7, true).endRule() // b :- x,z
			.startRule().addHead(3).addToBody(6, true).addToBody(7, true).endRule() // c :- y,z
			.startRule().addHead(8).addToBody(5, true).addToBody(4, false).endRule() //  :- x,not d
			.startRule().addHead(8).addToBody(6, true).addToBody(2, false).endRule() //  :- y,not b
			.startRule().addHead(8).addToBody(7, true).addToBody(3, false).endRule() //  :- z,not c
			.setCompute(8, false)
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true, true));
		CPPUNIT_ASSERT(solver.numVars() == 0);
	}

	void testMergeValue() {
		PrgNode lhs, rhs;
		ValueRep ok[15] = {
			value_free, value_free, value_free,
			value_free, value_true, value_true,
			value_free, value_false, value_false,
			value_free, value_weak_true, value_weak_true,
			value_true, value_weak_true, value_true
		};
		ValueRep fail[4] = { value_true, value_false, value_weak_true, value_false };
		for (uint32 x = 0; x != 2; ++x) {
			for (uint32 i = 0; i != 15; i += 3) {
				lhs.clearLiteral(true);
				rhs.clearLiteral(true);
				CPPUNIT_ASSERT(lhs.setValue(ok[i+x]));
				CPPUNIT_ASSERT(rhs.setValue(ok[i+(!x)]));
				CPPUNIT_ASSERT(lhs.mergeValue(&rhs));
				CPPUNIT_ASSERT(lhs.value() == ok[i+2] && rhs.value() == ok[i+2]);
			}
		}
		for (uint32 x = 0; x != 2; ++x) {
			for (uint32 i = 0; i != 4; i+=2) {
				lhs.clearLiteral(true);
				rhs.clearLiteral(true);
				CPPUNIT_ASSERT(lhs.setValue(fail[i+x]));
				CPPUNIT_ASSERT(rhs.setValue(fail[i+(!x)]));
				CPPUNIT_ASSERT(!lhs.mergeValue(&rhs));
			}
		}
	}
private:
	Solver solver;
	ProgramBuilder builder;
	AtomIndex index;
	DefaultUnfoundedCheck* ufs;
	stringstream str;
};
CPPUNIT_TEST_SUITE_REGISTRATION(ProgramBuilderTest);
 } } 
