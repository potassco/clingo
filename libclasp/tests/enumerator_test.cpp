#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include "common.h"
#include <clasp/solver.h>
#include <clasp/program_builder.h>
#include <clasp/unfounded_check.h>
#include <clasp/smodels_constraints.h>
#include <clasp/model_enumerators.h>
#include <sstream>
using namespace std;
namespace Clasp { namespace Test {
	
class EnumeratorTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(EnumeratorTest);
	CPPUNIT_TEST(testMiniProject);
	CPPUNIT_TEST(testProjectBug);
	CPPUNIT_TEST(testTerminateRemovesWatches);
	CPPUNIT_TEST_SUITE_END();	
public:
	void testMiniProject() {
		solver.strategies().symTab.reset(new AtomIndex);
		builder.startProgram(*solver.strategies().symTab, 0, 0)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "_x")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).endRule()
			.startRule().addHead(3).addToBody(1, false).endRule()
			.startRule().addHead(3).addToBody(2, false).endRule()
			.startRule(OPTIMIZERULE).addToBody(3, true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		MinimizeConstraint* m = builder.createMinimize(solver, false);
		BacktrackEnumerator enumerator(0);
		enumerator.setMinimize(m);
		enumerator.setEnableProjection(true);
		enumerator.init(solver, 0);
		AtomIndex& index = *solver.strategies().symTab;
		solver.assume(index[1].lit);
		solver.propagate();
		solver.assume(index[2].lit);
		solver.propagate();
		solver.assume(index[3].lit);
		solver.propagate();
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(false, enumerator.backtrackFromModel(solver));
	}

	void testProjectBug() {
		solver.strategies().symTab.reset(new AtomIndex);
		builder.startProgram(*solver.strategies().symTab, 0, 0)
			.setAtomName(1, "x").setAtomName(2, "y").setAtomName(3, "z").setAtomName(4, "_p").setAtomName(5, "_q").setAtomName(6, "_r")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(4).endRule() // {x,y,_p}
			.startRule().addHead(5).addToBody(1, true).addToBody(4, true).endRule() // _q :- x,_p.
			.startRule().addHead(6).addToBody(2, true).addToBody(4, true).endRule() // _r :- y,_p.
			.startRule().addHead(3).addToBody(5, true).addToBody(6, true).endRule() // z :- _q,_r.
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));

		BacktrackEnumerator enumerator(7,0);
		enumerator.setEnableProjection(true);
		enumerator.init(solver, 0);
		AtomIndex& index = *solver.strategies().symTab;
		solver.assume(index[1].lit);
		solver.propagate();
		solver.assume(index[2].lit);
		solver.propagate();
		solver.assume(index[4].lit);
		solver.propagate();
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		solver.stats.solve.addModel(solver.decisionLevel());
		CPPUNIT_ASSERT_EQUAL(true, enumerator.backtrackFromModel(solver));

		solver.undoUntil(0);
		uint32 numT = 0;
		if (solver.value(index[1].lit.var()) == value_free) {
			solver.assume(index[1].lit) && solver.propagate();
			++numT;
		}
		else if (solver.isTrue(index[1].lit)) {
			++numT;
		}
		if (solver.value(index[2].lit.var()) == value_free) {
			solver.assume(index[2].lit) && solver.propagate();
			++numT;
		}
		else if (solver.isTrue(index[2].lit)) {
			++numT;
		}
		if (solver.value(index[4].lit.var()) == value_free) {
			solver.assume(index[4].lit) && solver.propagate();
		}
		if (solver.isTrue(index[3].lit)) {
			++numT;
		}
		CPPUNIT_ASSERT(numT < 3);
		enumerator.endSearch(solver, false);
	}

	void testTerminateRemovesWatches() {
		solver.strategies().symTab.reset(new AtomIndex);
		builder.startProgram(*solver.strategies().symTab, 0, 0)
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).addHead(4).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		RecordEnumerator enumerator(0);
		enumerator.init(solver, 0);
		AtomIndex& index = *solver.strategies().symTab;
		solver.assume(index[1].lit) && solver.propagate();
		solver.assume(index[2].lit) && solver.propagate();
		solver.assume(index[3].lit) && solver.propagate();
		solver.assume(index[4].lit) && solver.propagate();
		solver.stats.solve.addModel(solver.decisionLevel());
		CPPUNIT_ASSERT_EQUAL(uint32(0), solver.numFreeVars());
		CPPUNIT_ASSERT_EQUAL(true, enumerator.backtrackFromModel(solver));
		uint32 numW = solver.numWatches(index[1].lit) + solver.numWatches(index[2].lit) + solver.numWatches(index[3].lit) + solver.numWatches(index[4].lit);
		CPPUNIT_ASSERT(numW > 0);
		enumerator.endSearch(solver, false);
		numW = solver.numWatches(index[1].lit) + solver.numWatches(index[2].lit) + solver.numWatches(index[3].lit) + solver.numWatches(index[4].lit);
		CPPUNIT_ASSERT(numW == 0);
	}
private:
	Solver solver;
	ProgramBuilder builder;
	stringstream str;
};
CPPUNIT_TEST_SUITE_REGISTRATION(EnumeratorTest);
 } } 
