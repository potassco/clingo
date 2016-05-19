#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include "common.h"
#include <algorithm>
#include <clasp/clause.h>
#include <clasp/solver.h>

namespace Clasp { namespace Test {


class ClauseCreatorTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ClauseCreatorTest);
	CPPUNIT_TEST(testEmptyClauseIsFalse);	
	CPPUNIT_TEST(testFactsAreAsserted);
	CPPUNIT_TEST(testTopLevelSATClausesAreNotAdded);
	CPPUNIT_TEST(testTopLevelFalseLitsAreRemoved);
	CPPUNIT_TEST(testAddBinaryClause);
	CPPUNIT_TEST(testAddTernaryClause);
	CPPUNIT_TEST(testAddGenericClause);
	CPPUNIT_TEST(testCreatorAssertsFirstLit);
	CPPUNIT_TEST(testCreatorInitsWatches);
	CPPUNIT_TEST(testCreatorNotifiesHeuristic);

	CPPUNIT_TEST(testCreatorSimplifyRemovesDuplicates);
	CPPUNIT_TEST(testCreatorSimplifyFindsTauts);
	CPPUNIT_TEST(testCreatorSimplifyMovesWatch);

	CPPUNIT_TEST(testCreateNonAssertingLearntClause);
	CPPUNIT_TEST(testCreateNonAssertingLearntClauseConflict);
	CPPUNIT_TEST(testCreateNonAssertingLearntClauseAsserting);

	CPPUNIT_TEST(testFactAreRemovedFromLearnt);
	CPPUNIT_TEST_SUITE_END();	
public:
	ClauseCreatorTest() {
		creator.setSolver(solver);
		a = posLit(solver.addVar(Var_t::atom_var));
		b = posLit(solver.addVar(Var_t::atom_var));
		c = posLit(solver.addVar(Var_t::atom_var));
		d = posLit(solver.addVar(Var_t::atom_var));
		e = posLit(solver.addVar(Var_t::atom_var));
		f = posLit(solver.addVar(Var_t::atom_var));
		solver.startAddConstraints();
	}
	void testEmptyClauseIsFalse() {
		CPPUNIT_ASSERT_EQUAL(false, creator.end());
	}
	
	void testFactsAreAsserted() {
		CPPUNIT_ASSERT_EQUAL(true, creator.start().add(a).end());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(a));
	}
	void testTopLevelSATClausesAreNotAdded() {
		solver.force(a, 0);
		CPPUNIT_ASSERT_EQUAL(true, creator.start().add(a).add(b).end());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numConstraints());
	}
	void testTopLevelFalseLitsAreRemoved() {
		solver.force(~a, 0);
		CPPUNIT_ASSERT_EQUAL(true, creator.start().add(a).add(b).end());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numConstraints());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(b));
	}
	void testAddBinaryClause() {
		CPPUNIT_ASSERT_EQUAL(true, creator.start().add(a).add(b).end());
		CPPUNIT_ASSERT_EQUAL(1u, solver.numConstraints());
		CPPUNIT_ASSERT_EQUAL(1u, solver.numBinaryConstraints());
	}
	void testAddTernaryClause() {
		CPPUNIT_ASSERT_EQUAL(true, creator.start().add(a).add(b).add(c).end());
		CPPUNIT_ASSERT_EQUAL(1u, solver.numConstraints());
		CPPUNIT_ASSERT_EQUAL(1u, solver.numTernaryConstraints());
	}
	void testAddGenericClause() {
		CPPUNIT_ASSERT_EQUAL(true, creator.start().add(a).add(b).add(c).add(d).end());		
		CPPUNIT_ASSERT_EQUAL(1u, solver.numConstraints());
	}

	void testCreatorAssertsFirstLit() {
		solver.endAddConstraints();
		solver.assume(~b);
		solver.assume(~c);
		solver.assume(~d);
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, creator.startAsserting(Constraint_t::learnt_conflict, a)
			.add(b).add(c).add(d).end());

		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(a));
		CPPUNIT_ASSERT_EQUAL(false, solver.hasConflict());
		CPPUNIT_ASSERT_EQUAL(1u, solver.numLearntConstraints());
	}

	void testCreatorInitsWatches() {
		solver.endAddConstraints();
		solver.assume(~b);
		solver.assume(~c);
		solver.assume(~d);
		
		creator.startAsserting(Constraint_t::learnt_conflict,a).add(b).add(d).add(c);
		CPPUNIT_ASSERT_EQUAL(2u, creator.secondWatch());
		CPPUNIT_ASSERT_EQUAL(true, creator.end());	// asserts a
		solver.undoUntil(2);	// clear a and d
		solver.assume(~d);		// hopefully d was watched.
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL( value_true, solver.value(a.var()) );
	}
	void testCreatorNotifiesHeuristic() {
		struct FakeHeu : public SelectFirst {
			void newConstraint(const Solver&, const Literal*, LitVec::size_type size, ConstraintType t) {
				clSizes_.push_back(size);
				clTypes_.push_back(t);
			}
			std::vector<LitVec::size_type> clSizes_;
			std::vector<ConstraintType> clTypes_;
		}*heu = new FakeHeu;
		solver.strategies().heuristic.reset(heu);
		CPPUNIT_ASSERT_EQUAL(true, creator.start().add(a).add(b).add(c).add(d).end());
		solver.endAddConstraints();
		solver.assume(a);
		solver.assume(b);
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL(true, creator.startAsserting(Constraint_t::learnt_conflict, c)
			.add(~a).add(~b).end());

		CPPUNIT_ASSERT_EQUAL(true, creator.startAsserting(Constraint_t::learnt_loop, c)
			.add(~a).add(~b).end());

		CPPUNIT_ASSERT_EQUAL(uint32(3), (uint32)heu->clSizes_.size());
		CPPUNIT_ASSERT_EQUAL(uint32(4), (uint32)heu->clSizes_[0]);
		CPPUNIT_ASSERT_EQUAL(uint32(3), (uint32)heu->clSizes_[1]);
		CPPUNIT_ASSERT_EQUAL(uint32(3), (uint32)heu->clSizes_[2]);

		CPPUNIT_ASSERT_EQUAL(Constraint_t::native_constraint, heu->clTypes_[0]);
		CPPUNIT_ASSERT_EQUAL(Constraint_t::learnt_conflict, heu->clTypes_[1]);
		CPPUNIT_ASSERT_EQUAL(Constraint_t::learnt_loop, heu->clTypes_[2]);
	}

	void testCreatorSimplifyRemovesDuplicates() {
		creator.start().add(a).add(b).add(c).add(a).add(b).add(d).simplify();
		CPPUNIT_ASSERT(creator.size() == 4);
		CPPUNIT_ASSERT(creator[0] == a);
		CPPUNIT_ASSERT(creator[1] == b);
		CPPUNIT_ASSERT(creator[2] == c);
		CPPUNIT_ASSERT(creator[3] == d);
	}
	void testCreatorSimplifyFindsTauts() {
		creator.start().add(a).add(b).add(c).add(a).add(b).add(~a).simplify();
		CPPUNIT_ASSERT(creator.size() == 4);
		CPPUNIT_ASSERT(creator[0] == a);
		CPPUNIT_ASSERT(creator[1] == b);
		CPPUNIT_ASSERT(creator[2] == c);
		CPPUNIT_ASSERT(creator[3] == ~a);
		creator.end();
		CPPUNIT_ASSERT(0 == solver.numConstraints());
	}
	void testCreatorSimplifyMovesWatch() {
		solver.endAddConstraints();
		solver.assume(a); solver.propagate();
		solver.assume(b); solver.propagate();
		creator.start(Constraint_t::learnt_loop);
		creator.add(~a).add(~b).add(~b).add(~a).add(c).add(d).simplify();
		CPPUNIT_ASSERT(creator.size() == 4);
		CPPUNIT_ASSERT_EQUAL(c, creator[0]);
		CPPUNIT_ASSERT_EQUAL(d, creator[creator.secondWatch()]);
	}

	void testCreateNonAssertingLearntClause() {
		solver.endAddConstraints();
		solver.assume(a); solver.propagate();
		solver.assume(b); solver.propagate();
		creator.start(Constraint_t::learnt_loop);
		creator.add(~a).add(~b).add(c).add(d);
		CPPUNIT_ASSERT_EQUAL(c, creator[0]);
		CPPUNIT_ASSERT_EQUAL(d, creator[creator.secondWatch()]);
		CPPUNIT_ASSERT(creator.end());
		CPPUNIT_ASSERT(solver.numLearntConstraints() == 1);

		solver.undoUntil(0);
		// test with a short clause
		solver.assume(a); solver.propagate();
		creator.start(Constraint_t::learnt_loop);
		creator.add(~a).add(b).add(c);
		CPPUNIT_ASSERT_EQUAL(b, creator[0]);
		CPPUNIT_ASSERT_EQUAL(c, creator[creator.secondWatch()]);
		CPPUNIT_ASSERT(creator.end());
		CPPUNIT_ASSERT(solver.numTernaryConstraints() == 1);
	}
	void testCreateNonAssertingLearntClauseConflict() {
		solver.endAddConstraints();
		solver.assume(a); solver.force(b,0); solver.propagate();// level 1
		solver.assume(c); solver.propagate();										// level 2
		solver.assume(d); solver.propagate();										// level 3

		creator.start(Constraint_t::learnt_loop);
		creator.add(~c).add(~a).add(~d).add(~b);								// 2 1 3 1
		// make sure we watch highest levels, i.e. 3 and 2
		CPPUNIT_ASSERT_EQUAL(~d, creator[0]);
		CPPUNIT_ASSERT_EQUAL(~c, creator[creator.secondWatch()]);
		CPPUNIT_ASSERT_EQUAL(false, creator.end());
		CPPUNIT_ASSERT(solver.numLearntConstraints() == 1);
		
		solver.undoUntil(0);
		// test with a short clause
		solver.assume(a); solver.propagate();// level 1
		solver.assume(c); solver.propagate();// level 2
		creator.start(Constraint_t::learnt_loop);
		creator.add(~a).add(~c);
		CPPUNIT_ASSERT_EQUAL(~c, creator[0]);
		CPPUNIT_ASSERT_EQUAL(~a, creator[creator.secondWatch()]);
		CPPUNIT_ASSERT_EQUAL(false, creator.end());
		CPPUNIT_ASSERT(solver.numBinaryConstraints() == 1);
	}

	void testCreateNonAssertingLearntClauseAsserting() {
		solver.endAddConstraints();
		solver.assume(a); solver.force(b,0); solver.propagate();// level 1
		solver.assume(c); solver.propagate();										// level 2
		creator.start(Constraint_t::learnt_loop);
		creator.add(~c).add(~a).add(d).add(~b);									// 2 1 Free 1
		// make sure we watch the right lits, i.e. d (free) and ~c (highest DL)
		CPPUNIT_ASSERT_EQUAL(d, creator[0]);
		CPPUNIT_ASSERT_EQUAL(~c, creator[creator.secondWatch()]);
		CPPUNIT_ASSERT_EQUAL(true, creator.end());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(d));
		CPPUNIT_ASSERT(solver.numLearntConstraints() == 1);

		// test with a short clause
		solver.undoUntil(0);
		solver.assume(a); solver.force(b,0); solver.propagate();// level 1
		solver.assume(c); solver.propagate();										// level 2
		creator.start(Constraint_t::learnt_loop);
		creator.add(~c).add(~a).add(d);													// 2 1 Free
		// make sure we watch the right lits, i.e. d (free) and ~c (highest DL)
		CPPUNIT_ASSERT_EQUAL(d, creator[0]);
		CPPUNIT_ASSERT_EQUAL(~c, creator[creator.secondWatch()]);
		CPPUNIT_ASSERT_EQUAL(true, creator.end());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(d));
		CPPUNIT_ASSERT(solver.numTernaryConstraints() == 1);
	}

	void testFactAreRemovedFromLearnt() {
		solver.addUnary(a);
		solver.endAddConstraints();
		solver.assume(~b) && solver.propagate();
		creator.start(Constraint_t::learnt_conflict);
		creator.add(b).add(c).add(~a).end();

		CPPUNIT_ASSERT(1u == solver.numConstraints());
		CPPUNIT_ASSERT(1u == solver.numBinaryConstraints());
	}
private:
	Solver solver;
	ClauseCreator creator;
	Literal a,b,c,d,e,f;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ClauseCreatorTest);
} } 
