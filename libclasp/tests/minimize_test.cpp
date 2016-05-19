#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <algorithm>
#include <clasp/smodels_constraints.h>
#include <clasp/solver.h>

namespace Clasp { namespace Test {
class MinimizeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MinimizeTest);
	CPPUNIT_TEST(testEmpty);
	CPPUNIT_TEST(testInit);
	CPPUNIT_TEST(testOrder);
	CPPUNIT_TEST(testConflict);
	CPPUNIT_TEST(testLess);
	CPPUNIT_TEST(testLessEqual);
	CPPUNIT_TEST(testSetModelMayBacktrackMultiLevels);
	CPPUNIT_TEST(testPriorityBug);
	CPPUNIT_TEST(testReasonBug);
	CPPUNIT_TEST(testSmartBacktrack);

	CPPUNIT_TEST(testBacktrackToTrue);
	CPPUNIT_TEST(testMultiAssignment);

	CPPUNIT_TEST(testBugBacktrackFromFalse);
	CPPUNIT_TEST(testBugBacktrackToTrue);

	CPPUNIT_TEST(testBacktrackFromModel);
	CPPUNIT_TEST(testWeightNullBug);

	CPPUNIT_TEST(testAdjust);
	CPPUNIT_TEST_SUITE_END(); 
public:
	MinimizeTest() {
		a     = posLit(solver.addVar(Var_t::atom_var));
		b     = posLit(solver.addVar(Var_t::atom_var));
		c     = posLit(solver.addVar(Var_t::atom_var));
		d     = posLit(solver.addVar(Var_t::atom_var));
		e     = posLit(solver.addVar(Var_t::atom_var));
		f     = posLit(solver.addVar(Var_t::atom_var));
		x     = posLit(solver.addVar(Var_t::atom_var));
		y     = posLit(solver.addVar(Var_t::atom_var));
		solver.startAddConstraints();
	}
	void testEmpty() {
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
	}
	
	void testInit() {
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(a, 1) );
		aMin.push_back( WeightLiteral(b, 2) );
		aMin.push_back( WeightLiteral(c, 1) );
		bMin.push_back( WeightLiteral(a, 1) );
		bMin.push_back( WeightLiteral(d, 2) );

		minimizer.minimize(solver, aMin);
		minimizer.minimize(solver, bMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numWatches(a));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numWatches(b));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numWatches(c));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numWatches(d));
	}

	void testOrder() {
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(a, 1) );
		bMin.push_back( WeightLiteral(b, 1) );
		minimizer.minimize(solver, aMin);
		minimizer.minimize(solver, bMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		solver.assume(b);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagate());
		CPPUNIT_ASSERT_EQUAL(uint32(0), minimizer.setModel(solver));
		solver.undoUntil(0);
		solver.assume(a);
		CPPUNIT_ASSERT_EQUAL(false, solver.propagate());
	}

	void testConflict() {
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(c, 2) );
		aMin.push_back( WeightLiteral(d, 1) );
		aMin.push_back( WeightLiteral(e, 2) );
		
		bMin.push_back( WeightLiteral(a, 1) );
		bMin.push_back( WeightLiteral(b, 1) );

		minimizer.minimize(solver, aMin);
		minimizer.minimize(solver, bMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
				
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(c) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(uint32(2), minimizer.setModel(solver));
		solver.backtrack();
		solver.force(d, 0);
		solver.force(e, 0);
		CPPUNIT_ASSERT_EQUAL(false, solver.propagate());
		const LitVec& cfl = solver.conflict();
		CPPUNIT_ASSERT(LitVec::size_type(2) == cfl.size());
		CPPUNIT_ASSERT(d == cfl[0]);
		CPPUNIT_ASSERT(e == cfl[1]);
	} 

	void testLess() {
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(a, 1) );
		aMin.push_back( WeightLiteral(b, 1) );
		bMin.push_back( WeightLiteral(c, 1) );
		bMin.push_back( WeightLiteral(d, 1) );
		
		minimizer.minimize(solver, aMin);
		minimizer.minimize(solver, bMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(c) && solver.propagate());

		CPPUNIT_ASSERT_EQUAL(1u, minimizer.setModel(solver));
		solver.undoUntil(0);
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(d) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~a));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~b));
	}

	void testLessEqual() {
		minimizer.setMode(MinimizeConstraint::compare_less_equal);
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(a, 1) );
		aMin.push_back( WeightLiteral(b, 1) );
		bMin.push_back( WeightLiteral(c, 1) );
		bMin.push_back( WeightLiteral(d, 1) );
		
		minimizer.minimize(solver, aMin);
		minimizer.minimize(solver, bMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(c) && solver.propagate());

		CPPUNIT_ASSERT_EQUAL(1u, minimizer.setModel(solver));
		solver.undoUntil(0);
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(d) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
	}

	void testSetModelMayBacktrackMultiLevels() {
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(a, 1) );
		minimizer.minimize(solver, aMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(c) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(0u, minimizer.setModel(solver));
	}

	void testPriorityBug() {
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(a, 1) );
		aMin.push_back( WeightLiteral(b, 1) );
		aMin.push_back( WeightLiteral(c, 1) );
		bMin.push_back( WeightLiteral(d, 1) );
		bMin.push_back( WeightLiteral(e, 1) );
		bMin.push_back( WeightLiteral(f, 1) );
		
		minimizer.minimize(solver, aMin);
		minimizer.minimize(solver, bMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(e) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(f) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(2u, minimizer.setModel(solver));
		
		solver.backtrack();
		solver.backtrack();
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(d) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~b));
		LitVec r;
		solver.reason(~b).reason(~b, r);
		CPPUNIT_ASSERT(LitVec::size_type(1) == r.size());
		CPPUNIT_ASSERT(a == r[0]);
	}

	void testReasonBug() {
		WeightLitVec aMin, bMin;
		aMin.push_back( WeightLiteral(a, 1) );
		aMin.push_back( WeightLiteral(b, 1) );
		bMin.push_back( WeightLiteral(d, 2) );
		bMin.push_back( WeightLiteral(e, 1) );
		bMin.push_back( WeightLiteral(f, 3) );
		
		minimizer.minimize(solver, aMin);
		minimizer.minimize(solver, bMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(d) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(2u, minimizer.setModel(solver));
		solver.backtrack();
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(e) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~f));
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(c) && solver.propagate());
		solver.backtrack();
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~f));
		LitVec r;
		solver.reason(~f).reason(~f, r);
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), e) != r.end());
	}

	void testSmartBacktrack() {
		WeightLitVec aMin;
		aMin.push_back( WeightLiteral(a, 1) );
		aMin.push_back( WeightLiteral(b, 1) );
		aMin.push_back( WeightLiteral(c, 1) );
		minimizer.minimize(solver, aMin);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~c) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~b));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~c));
	}

	void testBacktrackToTrue() {
		WeightLitVec min1, min2;
		min1.push_back( WeightLiteral(a, 1) );
		min2.push_back( WeightLiteral(b, 1) );
		minimizer.minimize(solver, min1);
		minimizer.minimize(solver, min2);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.force(b, 0) && solver.propagate());
		CPPUNIT_ASSERT(minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT_EQUAL(false, solver.propagate());
	}

	void testMultiAssignment() {
		WeightLitVec min1, min2;
		min1.push_back( WeightLiteral(a, 1) );
		min1.push_back( WeightLiteral(b, 1) );
		min1.push_back( WeightLiteral(c, 1) );
		min1.push_back( WeightLiteral(d, 1) );
		min1.push_back( WeightLiteral(e, 1) );

		min2.push_back( WeightLiteral(f, 1) );
		minimizer.minimize(solver, min1);
		minimizer.minimize(solver, min2);
		minimizer.setOptimum(0, 3);
		minimizer.setOptimum(1, 0);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
	
		solver.assume(f);
		solver.force(a, 0);
		solver.force(b, 0);
		solver.force(c, 0);
		
		CPPUNIT_ASSERT_EQUAL(false, solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~d));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~e));
		
	}
	
	void testBugBacktrackFromFalse() {
		WeightLitVec min1, min2;
		min1.push_back( WeightLiteral(a, 1) );
		min1.push_back( WeightLiteral(b, 1) );
		min1.push_back( WeightLiteral(c, 1) );
		min2.push_back( WeightLiteral(d, 1) );
		min2.push_back( WeightLiteral(e, 1) );
		min2.push_back( WeightLiteral(f, 1) );
		minimizer.minimize(solver, min1);
		minimizer.minimize(solver, min2);
		CPPUNIT_ASSERT_EQUAL(false, minimizer.simplify(solver));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(x) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.force(~c,0) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(y) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.force(d,0) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.force(e,0) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.force(~f,0) && solver.propagate());
		
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT(3 == solver.decisionLevel());
		CPPUNIT_ASSERT_EQUAL(true, solver.force(f,0) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~d));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~e));
		
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~x));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~f));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~d));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~e));
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(~c));
	}
	
	void testBugBacktrackToTrue() {
		WeightLitVec min1, min2;
		min1.push_back( WeightLiteral(a, 1) );
		min1.push_back( WeightLiteral(b, 1) );
		min1.push_back( WeightLiteral(~b, 2) );
		min2.push_back( WeightLiteral(a, 1) );
		min2.push_back( WeightLiteral(b, 1) );
		min2.push_back( WeightLiteral(c, 1) );
		minimizer.minimize(solver, min1);
		minimizer.minimize(solver, min2);
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(c) && solver.propagate());

		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT_EQUAL(false, solver.propagate());
	}

	void testBacktrackFromModel() {
		WeightLitVec min1, min2;
		min1.push_back( WeightLiteral(a, 1) );
		min1.push_back( WeightLiteral(b, 1) );
		min1.push_back( WeightLiteral(c, 1) );
		minimizer.minimize(solver, min1);
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(d) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.force(~c,0) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT_EQUAL(2u, solver.decisionLevel());
		CPPUNIT_ASSERT_EQUAL(true, solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT_EQUAL(0u, solver.decisionLevel());
	}

	void testWeightNullBug() {
		WeightLitVec min1;
		min1.push_back( WeightLiteral(a, 1) );
		min1.push_back( WeightLiteral(b, 0) );
		minimizer.minimize(solver, min1);
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.force(~c,0) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT_EQUAL(0u, solver.decisionLevel());
	}

	void testAdjust() {
		WeightLitVec min1;
		min1.push_back( WeightLiteral(a, 1) );
		min1.push_back( WeightLiteral(b, 1) );
		minimizer.minimize(solver, min1);
		minimizer.adjustSum(0, -2);
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT(0 == minimizer.getOptimum(0));
		solver.clearAssumptions();
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~a) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~b) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(false, minimizer.backtrackFromModel(solver));
		CPPUNIT_ASSERT(-2 == minimizer.getOptimum(0));

	}
private:
	Solver solver;
	MinimizeConstraint minimizer;
	Literal a, b, c, d, e, f, x, y;
};
CPPUNIT_TEST_SUITE_REGISTRATION(MinimizeTest);
} } 
