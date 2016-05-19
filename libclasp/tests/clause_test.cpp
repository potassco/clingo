#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <algorithm>
#include <clasp/clause.h>
#include <clasp/solver.h>

#ifdef _MSC_VER
#pragma warning (disable : 4267) //  conversion from 'size_t' to unsigned int
#pragma once
#endif


namespace Clasp { namespace Test {

class ClauseTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ClauseTest);
	CPPUNIT_TEST(testClauseCtorAddsWatches);
	CPPUNIT_TEST(testClauseTypes);
	CPPUNIT_TEST(testClauseActivity);

	CPPUNIT_TEST(testPropGenericClause);
	CPPUNIT_TEST(testPropGenericClauseConflict);
	CPPUNIT_TEST(testPropAlreadySatisfied);
	CPPUNIT_TEST(testPropRandomClauses);

	CPPUNIT_TEST(testReasonBumpsActivityIfLearnt);

	CPPUNIT_TEST(testSimplifySAT);

	CPPUNIT_TEST(testSimplifyUnitButNotLocked);
	CPPUNIT_TEST(testSimplifyRemovesFalseLitsBeg);
	CPPUNIT_TEST(testSimplifyRemovesFalseLitsMid);
	CPPUNIT_TEST(testSimplifyRemovesFalseLitsEnd);

	CPPUNIT_TEST(testClauseSatisfied);
	CPPUNIT_TEST(testContraction);
	CPPUNIT_TEST(testNewContractedClause);

	CPPUNIT_TEST(testBug);

	CPPUNIT_TEST(testLoopFormulaInitialWatches);
	CPPUNIT_TEST(testSimplifyLFIfOneBodyTrue);
	CPPUNIT_TEST(testSimplifyLFIfAllAtomsFalse);
	CPPUNIT_TEST(testSimplifyLFRemovesFalseBodies);
	CPPUNIT_TEST(testSimplifyLFRemovesFalseAtoms);
	CPPUNIT_TEST(testSimplifyLFRemovesTrueAtoms);

	CPPUNIT_TEST(testLoopFormulaPropagateBody);
	CPPUNIT_TEST(testLoopFormulaPropagateBody2);
	CPPUNIT_TEST(testLoopFormulaPropagateAtoms);
	CPPUNIT_TEST(testLoopFormulaPropagateAtoms2);
	CPPUNIT_TEST(testLoopFormulaBodyConflict);
	CPPUNIT_TEST(testLoopFormulaAtomConflict);
	CPPUNIT_TEST(testLoopFormulaDontChangeSat);
	CPPUNIT_TEST(testLoopFormulaPropTrueAtomInSatClause);

	CPPUNIT_TEST(testLoopFormulaSatisfied);

	CPPUNIT_TEST(testLoopFormulaBugEq);

	CPPUNIT_TEST_SUITE_END(); 
public:
	ClauseTest() {
		a1 = posLit(solver.addVar(Var_t::atom_var));
		a2 = posLit(solver.addVar(Var_t::atom_var));
		a3 = posLit(solver.addVar(Var_t::atom_var));
		b1 = posLit(solver.addVar(Var_t::body_var));
		b2 = posLit(solver.addVar(Var_t::body_var));
		b3 = posLit(solver.addVar(Var_t::body_var));

		for (int i = 6; i < 10; ++i) {
			solver.addVar(Var_t::atom_var);
		}
		solver.startAddConstraints();
	}
	void testClauseCtorAddsWatches() {
		Clause* cl1;
		solver.add(cl1 = createClause(2,2));
		CPPUNIT_ASSERT_EQUAL(2, countWatches(cl1, solver) );
		LearntConstraint* cl2 = Clause::newLearntClause(solver, clLits, Constraint_t::learnt_conflict,1);
		solver.add(cl2);
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[0], cl2));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[1], cl2));
	}

	void testClauseTypes() {
		Clause* cl1 = createClause(2, 2);
		LearntConstraint* cl2 = Clause::newLearntClause(solver, clLits, Constraint_t::learnt_conflict,1);
		LearntConstraint* cl3 = Clause::newLearntClause(solver, clLits, Constraint_t::learnt_loop,1);
		CPPUNIT_ASSERT_EQUAL(Constraint_t::native_constraint, cl1->type());
		CPPUNIT_ASSERT_EQUAL(Constraint_t::learnt_conflict, cl2->type());
		CPPUNIT_ASSERT_EQUAL(Constraint_t::learnt_loop, cl3->type());
		cl1->destroy();
		cl2->destroy();
		cl3->destroy();
	}

	void testClauseActivity() {
		LitVec lit;
		lit.push_back(posLit(1));
		lit.push_back(posLit(2));
		lit.push_back(posLit(3));
		lit.push_back(posLit(4));
		solver.stats.solve.restarts = 257;
		uint32 exp = 258;
		LearntConstraint* cl1 = Clause::newLearntClause(solver, lit, Constraint_t::learnt_conflict,1);
		LearntConstraint* cl2 = Clause::newLearntClause(solver, lit, Constraint_t::learnt_loop,1);
		solver.add(cl1);
		solver.add(cl2);
		while ( exp != 0 ) {
			CPPUNIT_ASSERT(cl1->activity() == cl2->activity() && cl1->activity() == exp);
			exp >>= 1;
			cl1->decreaseActivity();
			cl2->decreaseActivity();
		}
		CPPUNIT_ASSERT(cl1->activity() == cl2->activity() && cl1->activity() == exp);
	}

	void testPropGenericClause() {
		Clause* c;
		solver.add(c = createClause(2, 2));
		CPPUNIT_ASSERT_EQUAL(2, countWatches(c, solver));
		solver.assume(~clLits[0]);
		solver.propagate();
		solver.assume( ~clLits[c->size()-1] );
		solver.propagate();
		
		solver.assume(~clLits[1]);
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue( clLits[2] ) );
		CPPUNIT_ASSERT_EQUAL(true, c->locked(solver));
		Antecedent reason = solver.reason(clLits[2]);
		CPPUNIT_ASSERT(reason == c);
		
		LitVec r;
		reason.reason(clLits[2], r);
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~clLits[0]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~clLits[1]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~clLits[3]) != r.end());
	}

	void testPropGenericClauseConflict() {
		Clause* c;
		solver.add(c = createClause(2, 2));
		solver.assume( ~clLits[0] );
		solver.force( ~clLits[1], 0 );
		solver.force( ~clLits[2], 0 );
		solver.force( ~clLits[3], 0 );
		
		CPPUNIT_ASSERT_EQUAL(false, solver.propagate());
		const LitVec& r = solver.conflict();
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~clLits[0]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~clLits[1]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~clLits[2]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~clLits[3]) != r.end());
	}

	void testPropAlreadySatisfied() {
		Clause* c;
		solver.add(c = createClause(2, 2));

		// satisfy the clause...
		solver.force(clLits[2], 0);
		solver.propagate();

		// ...make all but one literal false
		solver.force(~clLits[0], 0);
		solver.force(~clLits[1], 0);
		solver.propagate();

		// ...and assert that the last literal is still unassigned
		CPPUNIT_ASSERT(value_free == solver.value(clLits[3].var()));
	}
	void testPropRandomClauses() {
		for (int i = 0; i != 100; ++i) {
			solver.reset();
			solver.strategies().randomWatches = true;
			for (int j = 0; j < 10; ++j) { solver.addVar(Var_t::atom_var); }
			solver.startAddConstraints();
			Clause* c;
			solver.add( c = createRandomClause( (rand() % 8) + 2 ) );
			check(*c);
		}
	}
	
	void testReasonBumpsActivityIfLearnt() {
		clLits.push_back(posLit(1));
		clLits.push_back(posLit(2));
		clLits.push_back(posLit(3));
		clLits.push_back(posLit(4));
		solver.endAddConstraints();
		LearntConstraint* cl1 = Clause::newLearntClause(solver, clLits, Constraint_t::learnt_conflict,1);
		solver.addLearnt(cl1, (uint32)clLits.size());
		solver.assume(~clLits[0]);
		solver.propagate();
		solver.assume(~clLits[1]);
		solver.propagate();
		solver.assume(~clLits[2]);
		solver.propagate();
		
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue( clLits[3] ) );
		uint32 a = cl1->activity();
		LitVec r;
		solver.reason(clLits[3]).reason(clLits[3], r);
		CPPUNIT_ASSERT_EQUAL(a+1, cl1->activity());
	}

	void testSimplifySAT() {
		Clause* c;
		solver.add(c = createClause(3, 2));
		solver.force( ~clLits[1], 0);
		solver.force( clLits[2], 0 );
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL(true, c->simplify(solver, false));
		CPPUNIT_ASSERT_EQUAL(false, solver.hasWatch( ~clLits[0], c ) );
		CPPUNIT_ASSERT_EQUAL(false, solver.hasWatch( ~clLits[c->size() - 1], c ) );
	}
	void testSimplifyUnitButNotLocked() {
		Clause* c;
		solver.add(c = createClause(2, 2));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[0], c));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[3], c));
		solver.force( clLits[0], 0);  // SAT clause
		solver.force( ~clLits[1], 0 );
		solver.force( ~clLits[2], 0 );
		solver.force( ~clLits[3], 0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, c->simplify(solver, false));
	}

	void testSimplifyRemovesFalseLitsBeg() {
		Clause* c;
		
		solver.add(c = createClause(3, 3));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(6), c->size());
		
		solver.force(~clLits[0], 0);
		solver.force(~clLits[1], 0);
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL(false, c->simplify(solver));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(4), c->size());

		CPPUNIT_ASSERT_EQUAL(2, countWatches(c, solver));
	}

	void testSimplifyRemovesFalseLitsMid() {
		Clause* c;
		
		solver.add(c = createClause(3, 3));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(6), c->size());
		
		solver.force(~clLits[1], 0);
		solver.force(~clLits[2], 0);
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL(false, c->simplify(solver));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(4), c->size());

		CPPUNIT_ASSERT_EQUAL(2, countWatches(c, solver));
	}

	void testSimplifyRemovesFalseLitsEnd() {
		Clause* c;
		solver.add(c = createClause(3, 3));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(6), c->size());
		
		solver.force(~clLits[4], 0);
		solver.force(~clLits[5], 0);
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL(false, c->simplify(solver));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(4), c->size());

		CPPUNIT_ASSERT_EQUAL(2, countWatches(c, solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[0], c));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[3], c));
	}

	void testClauseSatisfied() {
		Clause* c;
		solver.add(c = createClause(2, 2));
		LitVec free;
		CPPUNIT_ASSERT_EQUAL(false, c->isSatisfied(solver, free));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(4), free.size());

		solver.assume( clLits[2] );
		solver.propagate();
		free.clear();
		CPPUNIT_ASSERT_EQUAL(true, c->isSatisfied(solver, free));
		solver.undoUntil(0);
		solver.assume( ~clLits[1] );
		solver.assume( ~clLits[2] );
		solver.propagate();
		free.clear();
		CPPUNIT_ASSERT_EQUAL(false, c->isSatisfied(solver, free));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(2), free.size());
	}
	
	void testContraction() {
		solver.strategies().setCompressionStrategy(6);
		solver.endAddConstraints();
		solver.assume(~a2);
		solver.assume(~a3);
		solver.assume(~b1);
		solver.assume(~b2);
		solver.assume(~b3);
		ClauseCreator creator(&solver);
		Constraint* con;
		creator.startAsserting(Constraint_t::learnt_conflict, a1)
			.add(a2).add(a3).add(b1).add(b2).add(b3).end(&con);
		Clause* cl = (Clause*)con;
		
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~a1, cl));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~b3, cl));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(2), cl->size());

		LitVec r;
		solver.reason(a1).reason(a1, r);
		CPPUNIT_ASSERT( r.size() == 5 );
		
		solver.undoUntil(solver.decisionLevel()-3);
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(4), cl->size());
		
		solver.undoUntil(solver.decisionLevel()-2);
		CPPUNIT_ASSERT(LitVec::size_type(5) <= cl->size());
		solver.undoUntil(0);
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(6), cl->size());
	}

	void testNewContractedClause() {
		solver.endAddConstraints();
		solver.assume(~b1);
		solver.assume(~b2);
		solver.assume(~b3);
		// head
		clLits.push_back(a1);
		clLits.push_back(a2);
		clLits.push_back(a3);
		// (false) tail
		clLits.push_back(b1);
		clLits.push_back(b2);
		clLits.push_back(b3);
		
		Clause* c = (Clause*) Clause::newContractedClause(solver, clLits, 1, 3);
		solver.add(c);
		CPPUNIT_ASSERT_EQUAL(3u, c->size());

		solver.assume(~a1) && solver.propagate();
		solver.assume(~a3) && solver.propagate();
		CPPUNIT_ASSERT(solver.isTrue(a2));
		Antecedent x = solver.reason(a2);
		CPPUNIT_ASSERT(x == c);
		LitVec r;
		c->reason(a2, r);
		CPPUNIT_ASSERT(r.size() == 5);
	}
	void testBug() {
		Clause* c;
		solver.add(c = createClause(3, 3));
		solver.assume(~clLits[1]);
		solver.propagate();
		solver.assume(~clLits[2]);
		solver.propagate();
		solver.assume(~clLits[3]);
		solver.propagate();
		solver.assume(~clLits[0]);
		solver.propagate();
		uint32 exp = solver.numAssignedVars();
		solver.undoUntil(0);
		solver.assume(~clLits[1]);
		solver.propagate();
		solver.assume(~clLits[2]);
		solver.propagate();
		solver.assume(~clLits[3]);
		solver.propagate();
		solver.assume(~clLits[4]);
		solver.propagate();
		
		CPPUNIT_ASSERT(exp == solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[0], c));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~clLits[5], c));
	}

	void testLoopFormulaInitialWatches() {
		LoopFormula* lf = lfTestInit();
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(a1, lf));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(a2, lf));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(a3, lf));
		CPPUNIT_ASSERT_EQUAL(true, solver.hasWatch(~b3, lf));   
	}
	
	void testSimplifyLFIfOneBodyTrue() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.force( b2, 0 );
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL(true, lf->simplify(solver));
		CPPUNIT_ASSERT_EQUAL(false, solver.hasWatch(a1, lf));
		CPPUNIT_ASSERT_EQUAL(false, solver.hasWatch(~b3, lf));
	}

	void testSimplifyLFIfAllAtomsFalse() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.force( ~a1, 0 );
		solver.force( ~a2, 0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(false, lf->simplify(solver));
		solver.force( ~a3, 0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, lf->simplify(solver));
		CPPUNIT_ASSERT_EQUAL(false, solver.hasWatch(~b3, lf));
	}

	void testSimplifyLFRemovesFalseBodies() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		
		solver.force( ~b1, 0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, lf->simplify(solver));
		CPPUNIT_ASSERT(3u == solver.numTernaryConstraints());
	}

	void testSimplifyLFRemovesFalseAtoms() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.force( ~a1,0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(false, lf->simplify(solver));
		CPPUNIT_ASSERT(5 == lf->size());

		solver.force( ~a3,0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(false, lf->simplify(solver));
		CPPUNIT_ASSERT(4 == lf->size());

		solver.force( ~a2,0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, lf->simplify(solver));
	}

	void testSimplifyLFRemovesTrueAtoms() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.force( a1,0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(false, lf->simplify(solver));
		CPPUNIT_ASSERT(5 == lf->size());

		solver.force( a3,0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(false, lf->simplify(solver));
		CPPUNIT_ASSERT(4 == lf->size());

		solver.force( a2,0 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, lf->simplify(solver));
	}

	void testLoopFormulaPropagateBody() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.assume( ~b1 );
		solver.propagate();
		solver.assume( ~b3 );
		solver.propagate();
		solver.assume( a3 );
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(b2) );
		Antecedent a = solver.reason(b2);
		CPPUNIT_ASSERT_EQUAL( Antecedent::generic_constraint, a.type() );
		LitVec r;
		a.reason(b2, r);
		CPPUNIT_ASSERT_EQUAL( LitVec::size_type(3), r.size() );
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), a3) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b3) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b1) != r.end());

		CPPUNIT_ASSERT_EQUAL(true, lf->locked(solver));
	}

	void testLoopFormulaPropagateBody2() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.assume( a1 );
		solver.propagate();
		solver.undoUntil(0);
		
		solver.assume( ~b1 );
		solver.propagate();
		solver.assume( a2 );
		solver.propagate();
		solver.assume( ~b2 );
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(b3) );
		
		Antecedent a = solver.reason(b3);
		CPPUNIT_ASSERT_EQUAL( Antecedent::generic_constraint, a.type() );
		LitVec r;
		a.reason(b3, r);
		CPPUNIT_ASSERT_EQUAL( LitVec::size_type(3), r.size() );
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b1) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b2) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), a2) != r.end());

		CPPUNIT_ASSERT_EQUAL(true, lf->locked(solver));
	}

	void testLoopFormulaPropagateAtoms() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.assume( ~b3 );
		solver.propagate();
		solver.assume( ~b1 );
		solver.propagate();
		solver.assume( ~b2 );
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(~a1) );
		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(~a2) );
		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(~a3) );
		
		Antecedent a = solver.reason(~a1);
		CPPUNIT_ASSERT_EQUAL( Antecedent::generic_constraint, a.type() );
		LitVec r;
		a.reason(~a1, r);
		CPPUNIT_ASSERT_EQUAL( LitVec::size_type(3), r.size() );
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b1) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b2) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b3) != r.end());

		CPPUNIT_ASSERT_EQUAL(true, lf->locked(solver));
	}

	void testLoopFormulaPropagateAtoms2() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.assume( a1 );
		solver.force( a2, 0 );
		solver.propagate();
		solver.undoUntil(0);
		
		solver.assume( ~b3 );
		solver.propagate();
		solver.assume( ~b1 );
		solver.propagate();
		solver.assume( ~b2 );
		solver.propagate();

		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(~a1) );
		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(~a2) );
		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(~a3) );
		
		
		Antecedent a = solver.reason(~a1);
		CPPUNIT_ASSERT_EQUAL( Antecedent::generic_constraint, a.type() );
		LitVec r;
		a.reason(~a1, r);
		CPPUNIT_ASSERT_EQUAL( LitVec::size_type(3), r.size() );
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b1) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b2) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b3) != r.end());

		CPPUNIT_ASSERT_EQUAL(true, lf->locked(solver));
	}

	void testLoopFormulaBodyConflict() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		
		solver.assume( ~b3 );
		solver.propagate();
		solver.assume( ~b2 );
		solver.propagate();
		solver.force(a3, 0);
		solver.force(~b1, 0);
		
		CPPUNIT_ASSERT_EQUAL( false, solver.propagate() );
		const LitVec& r = solver.conflict();
		
		CPPUNIT_ASSERT_EQUAL( LitVec::size_type(4), r.size() );
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), a3) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b3) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b1) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b2) != r.end());
	}
	void testLoopFormulaAtomConflict() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.assume( ~b3 );
		solver.propagate();
		solver.assume(~b1);
		solver.propagate();
		solver.force(~b2, 0);
		solver.force(a2, 0);
		CPPUNIT_ASSERT_EQUAL( false, solver.propagate() );
		
		
		LitVec r = solver.conflict();
		
		CPPUNIT_ASSERT_EQUAL( LitVec::size_type(4), r.size() );
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), a2) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b3) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b1) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b2) != r.end());

		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(~a1));
		Antecedent a = solver.reason(~a1);
		r.clear();
		a.reason(~a1, r);
		CPPUNIT_ASSERT_EQUAL( LitVec::size_type(3), r.size() );
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b3) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b1) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~b2) != r.end());
	}

	void testLoopFormulaDontChangeSat() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		CPPUNIT_ASSERT_EQUAL(true, solver.assume( ~b1 ) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume( ~b3 ) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume( a2 ) && solver.propagate());

		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue( b2 ));
		LitVec rold;
		solver.reason(b2).reason(b2, rold);
		
		CPPUNIT_ASSERT_EQUAL(true, solver.assume( a1 ) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue( b2 ));
		LitVec rnew;
		solver.reason(b2).reason(b2, rnew);
		CPPUNIT_ASSERT(rold == rnew);
	}

	void testLoopFormulaSatisfied() {
		LoopFormula* lf = lfTestInit();
		LitVec free;
		CPPUNIT_ASSERT_EQUAL(true, lf->isSatisfied(solver, free));
		solver.undoUntil(0);
		free.clear();
		CPPUNIT_ASSERT_EQUAL(false, lf->isSatisfied(solver, free));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(6), free.size());
		solver.assume( a1 );
		solver.assume( ~b2 );
		solver.propagate();
		free.clear();
		CPPUNIT_ASSERT_EQUAL(false, lf->isSatisfied(solver, free));
		CPPUNIT_ASSERT_EQUAL(LitVec::size_type(4), free.size());
		solver.assume( ~b1 );
		solver.propagate();
		CPPUNIT_ASSERT_EQUAL(true, lf->isSatisfied(solver, free));
	}

	void testLoopFormulaBugEq() {
		solver.endAddConstraints();
		Literal body1 = b1; 
		Literal body2 = b2;
		Literal body3 = ~b3; // assume body3 is equivalent to some literal ~xy
		solver.assume(~body1);
		solver.assume(~body2);
		solver.assume(~body3);
		solver.propagate();
		Literal bodies[3] = {body1, body2, body3 };
		LoopFormula* lf = LoopFormula::newLoopFormula(solver, &bodies[0], 3, 2, 1);
		solver.addLearnt(lf, lf->size());
		solver.force(~a1, lf);
		lf->addAtom(~a1, solver);
		solver.propagate();
		solver.undoUntil(solver.decisionLevel()-2);
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~body3) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(a1) && solver.propagate());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(body2));
	}

	void testLoopFormulaPropTrueAtomInSatClause() {
		LoopFormula* lf = lfTestInit();
		solver.undoUntil(0);
		solver.assume( ~a1 );
		solver.propagate();
		
		solver.assume( a2 );
		solver.force( ~b3, 0 );
		solver.force( ~b2, 0 );
		solver.propagate();   
		
		CPPUNIT_ASSERT_EQUAL( true, solver.isTrue(b1) );
	}
private:
	Solver solver;
	int countWatches(Clause* c, const Solver& s) {
		int w = 0;
		for (LitVec::size_type i = 0; i < c->size(); ++i) {
			if (s.hasWatch( ~(*c)[i], c )) {
				++w;
			}
		}
		return w;
	}
	void check(Clause& c) {
		std::string s = toString(c);
		std::random_shuffle(clLits.begin(), clLits.end());
		CPPUNIT_ASSERT( value_free == solver.value(clLits.back().var()) );
		for (LitVec::size_type i = 0; i != clLits.size() - 1; ++i) {
			CPPUNIT_ASSERT( value_free == solver.value(clLits[i].var()) );
			solver.force(~clLits[i], 0);
			solver.propagate();
		}
		CPPUNIT_ASSERT_EQUAL_MESSAGE(s.c_str(), true, solver.isTrue(clLits.back()));

		Antecedent reason = solver.reason(clLits.back());
		CPPUNIT_ASSERT(reason == &c);
		LitVec r;
		c.reason(clLits.back(), r);
		for (LitVec::size_type i = 0; i != clLits.size() - 1; ++i) {
			LitVec::iterator it = std::find(r.begin(), r.end(), ~clLits[i]);
			CPPUNIT_ASSERT_MESSAGE(s.c_str(), it != r.end());
			r.erase(it);
		}
		CPPUNIT_ASSERT(r.empty());
	}
	std::string toString(const Clause& c) {
		std::string res="[";
		for (int i = 0; i != c.size(); ++i) {
			if (c[i].sign()) {
				res += '~';
			}
			res += ('a' + i);
			res += ' ';
		}
		res+="]";
		return res;
	}
	Clause* createClause(int pos, int neg) {
		clLits.clear();
		int size = pos + neg;
		CPPUNIT_ASSERT(size < 10);
		LitVec lit(size);
		for (int i = 0; i < pos; ++i) {
			lit[i] = posLit(i+1);
			clLits.push_back(lit[i]);
		}
		for (int i = pos; i < pos + neg; ++i) {
			lit[i] = negLit(i+1);
			clLits.push_back(lit[i]);
		}
		return (Clause*)Clause::newClause(solver, lit);
	}

	Clause* createRandomClause(int size) {
		int pos = rand() % size + 1;
		return createClause( pos, size - pos );
	}
	
	LoopFormula* lfTestInit() {
		solver.endAddConstraints();
		solver.assume(~b1);
		solver.assume(~b2);
		solver.assume(~b3);
		solver.propagate();
		Literal bodies[3] = {b1, b2, b3 };
		LoopFormula* lf = LoopFormula::newLoopFormula(solver, &bodies[0], 3, 2, 3);
		solver.addLearnt(lf, lf->size());
		solver.force(~a1, lf);
		solver.force(~a2, lf);
		solver.force(~a3, lf);
		lf->addAtom(~a1, solver);
		lf->addAtom(~a2, solver);
		lf->addAtom(~a3, solver);
		solver.propagate();
		return lf;
	}
	Literal a1, a2, a3, b1, b2, b3;
	LitVec clLits;
};
CPPUNIT_TEST_SUITE_REGISTRATION(ClauseTest);
} } 
