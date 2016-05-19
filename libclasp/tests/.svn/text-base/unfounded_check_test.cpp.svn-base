#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <clasp/unfounded_check.h>
#include <clasp/program_builder.h>
#include <clasp/clause.h>
#include "common.h"
#include <memory>

namespace Clasp { namespace Test {
	class UnfoundedCheckTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(UnfoundedCheckTest);
	CPPUNIT_TEST(testTightProgram);
	CPPUNIT_TEST(testInitOrder);
	CPPUNIT_TEST(testProgramWithLoops);
	CPPUNIT_TEST(testCloneProgramWithLoops);
	
	CPPUNIT_TEST(testAllUncoloredNoUnfounded);
	
	CPPUNIT_TEST(testAlternativeSourceNotUnfounded);
	CPPUNIT_TEST(testOnlyOneSourceUnfoundedIfMinus);
	CPPUNIT_TEST(testWithSimpleCardinalityConstraint);
	CPPUNIT_TEST(testWithSimpleWeightConstraint);
	CPPUNIT_TEST(testNtExtendedBug);
	CPPUNIT_TEST(testNtExtendedFalse);

	CPPUNIT_TEST(testDependentExtReason);

	CPPUNIT_TEST(testEqBodyDiffType);

	CPPUNIT_TEST(testChoiceCardInterplay);
	CPPUNIT_TEST(testCardInterplayOnBT);

	CPPUNIT_TEST(testIncrementalUfs);
	CPPUNIT_TEST_SUITE_END(); 
public:
	UnfoundedCheckTest() : ufc(0) {
	}
	void setUp() { ufc = new DefaultUnfoundedCheck(); }
	void testTightProgram() { 
		builder.startProgram(index, ufc.release())
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule().addHead(1).addToBody(2, false).endRule()
		.endProgram(solver, true);
		ufc = 0;
		CPPUNIT_ASSERT_EQUAL(true, builder.stats.sccs == 0);
	}

	void testInitOrder() {
		builder.startProgram(index, ufc.release(), 0)
			.setAtomName(1,"a").setAtomName(2,"b").setAtomName(3,"x").setAtomName(4,"y")
			.startRule().addHead(4).addToBody(3, true).endRule()  // y :- x.
			.startRule().addHead(3).addToBody(4, true).endRule()  // x :- y.
			.startRule().addHead(2).addToBody(3, true).endRule()  // b :- x.
			.startRule().addHead(2).addToBody(1, true).endRule()  // b :- a.
			.startRule().addHead(1).addToBody(2, true).endRule()  // a :- b.
			.startRule().addHead(3).addToBody(1, false).endRule() // x :- not a.
		.endProgram(solver, true);
		
		CPPUNIT_ASSERT_EQUAL(true, builder.stats.sccs == 2);

		// Test Pred-Order
		DefaultUnfoundedCheck::UfsAtomNode* b = ufc->atomNode(index[2].lit);
		DefaultUnfoundedCheck::UfsAtomNode* x = ufc->atomNode(index[3].lit);

		CPPUNIT_ASSERT(b->preds[0]->scc != b->scc);
		CPPUNIT_ASSERT(b->preds[1]->scc == b->scc);
		CPPUNIT_ASSERT(b->preds[2]      == 0);

		CPPUNIT_ASSERT(x->preds[0]->scc != x->scc);
		CPPUNIT_ASSERT(x->preds[1]->scc == x->scc);
		CPPUNIT_ASSERT(x->preds[2]      == 0);

		DefaultUnfoundedCheck::UfsBodyNode* xBody = b->preds[0];
		CPPUNIT_ASSERT(xBody->succs[0]->scc == xBody->scc);
		CPPUNIT_ASSERT(xBody->succs[1]->scc != xBody->scc);
		CPPUNIT_ASSERT(xBody->succs[2]      == 0);
	}
	
	void testProgramWithLoops() {
		builder.startProgram(index, ufc.release())
		.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
		.setAtomName(4, "d").setAtomName(5, "g").setAtomName(6, "x").setAtomName(7, "y")
		.startRule().addHead(1).addToBody(6, false).endRule() // a :- not x.
		.startRule().addHead(2).addToBody(1, true).endRule()  // b :- a.
		.startRule().addHead(1).addToBody(2, true).addToBody(4, true).endRule() // a :- b, d.
		.startRule().addHead(2).addToBody(5, false).endRule() // b :- not g.
		.startRule().addHead(3).addToBody(6, true).endRule()  // c :- x.
		.startRule().addHead(4).addToBody(3, true).endRule()  // d :- c.
		.startRule().addHead(3).addToBody(4, true).endRule()  // c :- d.
		.startRule().addHead(4).addToBody(5, false).endRule() // d :- not g.
		.startRule().addHead(7).addToBody(5, false).endRule() // y :- not g.
		.startRule().addHead(6).addToBody(7, true).endRule()  // x :- y.
		.startRule().addHead(5).addToBody(7, false).endRule() // g :- not y.
		.endProgram(solver, true);
		
		
		CPPUNIT_ASSERT_EQUAL(index[6].lit, index[7].lit);
		CPPUNIT_ASSERT_EQUAL(~index[6].lit, index[5].lit);
		
		CPPUNIT_ASSERT( ufc->atomNode(index[1].lit)->scc == 0 );
		CPPUNIT_ASSERT( ufc->atomNode(index[2].lit)->scc == 0 );
		CPPUNIT_ASSERT( ufc->atomNode(index[3].lit)->scc == 1 );
		CPPUNIT_ASSERT( ufc->atomNode(index[4].lit)->scc == 1 );
		CPPUNIT_ASSERT( ufc->atomNode(index[5].lit) == 0 );
		CPPUNIT_ASSERT( ufc->atomNode(index[6].lit) == 0 );
		CPPUNIT_ASSERT( ufc->atomNode(index[7].lit) == 0 );
		
		CPPUNIT_ASSERT(LitVec::size_type(10) == ufc->nodes());
		
		// check that lists are partitioned by component number
		DefaultUnfoundedCheck::UfsAtomNode* a = ufc->atomNode(index[1].lit);
		CPPUNIT_ASSERT(a->preds[0]->scc == PrgNode::noScc);
		CPPUNIT_ASSERT(a->preds[1]->scc == a->scc);
		CPPUNIT_ASSERT(a->preds[2] == 0);
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(a->lit.var()));

		DefaultUnfoundedCheck::UfsBodyNode* bd = a->preds[1];
		CPPUNIT_ASSERT_EQUAL(true, solver.frozen(bd->lit.var()));
		CPPUNIT_ASSERT(bd->preds[0]->lit == index[2].lit);
		CPPUNIT_ASSERT(bd->preds[1]==0);
		CPPUNIT_ASSERT(bd->succs[0] == a);
		CPPUNIT_ASSERT(bd->succs[1] == 0);
	}
	
	void testCloneProgramWithLoops() {
		builder.startProgram(index, ufc.release())
		.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
		.setAtomName(4, "d").setAtomName(5, "g").setAtomName(6, "x").setAtomName(7, "y")
		.startRule().addHead(1).addToBody(6, false).endRule() // a :- not x.
		.startRule().addHead(2).addToBody(1, true).endRule()  // b :- a.
		.startRule().addHead(1).addToBody(2, true).addToBody(4, true).endRule() // a :- b, d.
		.startRule().addHead(2).addToBody(5, false).endRule() // b :- not g.
		.startRule().addHead(3).addToBody(6, true).endRule()  // c :- x.
		.startRule().addHead(4).addToBody(3, true).endRule()  // d :- c.
		.startRule().addHead(3).addToBody(4, true).endRule()  // c :- d.
		.startRule().addHead(4).addToBody(5, false).endRule() // d :- not g.
		.startRule().addHead(7).addToBody(5, false).endRule() // y :- not g.
		.startRule().addHead(6).addToBody(7, true).endRule()  // x :- y.
		.startRule().addHead(5).addToBody(7, false).endRule() // g :- not y.
		.endProgram(solver, true);

		uint32 sccs = builder.stats.sccs;
		uint32 ufs  = builder.stats.ufsNodes;

		Solver clone;
		builder.endProgram(clone, true);

		CPPUNIT_ASSERT_EQUAL(sccs, builder.stats.sccs);
		CPPUNIT_ASSERT_EQUAL(ufs, builder.stats.ufsNodes);
	}

	void testAllUncoloredNoUnfounded() {
		setupSimpleProgram();
		uint32 x = solver.numAssignedVars();
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(x, solver.numAssignedVars());
	}
	void testAlternativeSourceNotUnfounded() {
		setupSimpleProgram();
		solver.assume( index[6].lit );
		solver.propagateUntil(ufc.get());
		uint32 old = solver.numAssignedVars();
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(old, solver.numAssignedVars());
	}
	void testOnlyOneSourceUnfoundedIfMinus() {
		setupSimpleProgram();
		solver.assume( index[6].lit );
		solver.assume( index[5].lit );
		solver.propagateUntil(ufc.get());
		uint32 old = solver.numAssignedVars();
		uint32 oldC = solver.numConstraints();
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT(old < solver.numAssignedVars());
		CPPUNIT_ASSERT(solver.isFalse(index[4].lit));
		CPPUNIT_ASSERT(solver.isFalse(index[1].lit));
		CPPUNIT_ASSERT(!solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT(oldC+1 == solver.numConstraints());
	}

	void testWithSimpleCardinalityConstraint() {
		builder.startProgram(index, ufc.release())
			.setAtomName(1, "a").setAtomName(2, "b")
			.startRule(CHOICERULE).addHead(2).endRule()
			.startRule(CONSTRAINTRULE, 1).addHead(1).addToBody(1, true).addToBody(2,true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		
		
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(2u, solver.numVars());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver) );
		CPPUNIT_ASSERT_EQUAL(0u, solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~index[2].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver) );
		CPPUNIT_ASSERT_EQUAL(2u, solver.numAssignedVars());
		LitVec r;
		solver.reason(~index[1].lit).reason(~index[1].lit, r);
		CPPUNIT_ASSERT(1 == r.size());
		CPPUNIT_ASSERT(r[0] == ~index[2].lit);
	}
	
	void testWithSimpleWeightConstraint() {
		builder.startProgram(index, ufc.release())
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c")
			.startRule(CHOICERULE).addHead(2).addHead(3).endRule()
			.startRule(WEIGHTRULE, 2).addHead(1).addToBody(1, true, 2).addToBody(2,true, 2).addToBody(3, true, 1).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		

		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(3u, solver.numVars());
		CPPUNIT_ASSERT_EQUAL(0u, solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver) );
		CPPUNIT_ASSERT_EQUAL(0u, solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~index[3].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver) );
		CPPUNIT_ASSERT_EQUAL(1u, solver.numAssignedVars());

		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~index[2].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(2u, solver.numAssignedVars());

		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver) );
		CPPUNIT_ASSERT_EQUAL(3u, solver.numAssignedVars());
		
		LitVec r;
		solver.reason(~index[1].lit).reason(~index[1].lit, r);
		CPPUNIT_ASSERT(2 == r.size());
		CPPUNIT_ASSERT(r[0] == ~index[2].lit);
		CPPUNIT_ASSERT(r[1] == ~index[3].lit);

		solver.undoUntil(0);
		CPPUNIT_ASSERT_EQUAL(true, solver.assume(~index[2].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(1u, solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver) );
		CPPUNIT_ASSERT_EQUAL(2u, solver.numAssignedVars());
		r.clear();
		solver.reason(~index[1].lit).reason(~index[1].lit, r);
		CPPUNIT_ASSERT(1 == r.size());
		CPPUNIT_ASSERT(r[0] == ~index[2].lit);
	}

	void testNtExtendedBug() {
		builder.startProgram(index, ufc.release())
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "t").setAtomName(5, "x")
			.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).endRule() // {a,b,c}.
			.startRule(CONSTRAINTRULE, 2).addHead(4).addToBody(2, true).addToBody(4, true).addToBody(5,true).endRule()
			.startRule().addHead(5).addToBody(4,true).addToBody(3,true).endRule()
			.startRule().addHead(5).addToBody(1,true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		
		// T: {t,c}
		solver.assume(Literal(6, false));
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		solver.assume(~index[1].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(false, ufc->propagate(solver));  // {x, t} are unfounded!
		
		solver.undoUntil(0);
		ufc->reset();

		// F: {t,c}
		solver.assume(Literal(6, true));
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		// F: a
		solver.assume(~index[1].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		// x is false because both of its bodies are false
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[5].lit));
	
		// t is now unfounded but its defining body is not
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[4].lit));
		LitVec r;
		solver.reason(~index[4].lit).reason(~index[4].lit, r);
		CPPUNIT_ASSERT(r.size() == 1 && r[0] == ~index[5].lit);
	}

	void testNtExtendedFalse() {
		// {z}.
		// r :- 2 {x, y, s}
		// s :- r, z.
		// r :- s, z.
		// x :- not z.
		// y :- not z.
		builder.startProgram(index, ufc.release(), 0)
			.setAtomName(1, "x").setAtomName(2, "y").setAtomName(3, "z").setAtomName(4, "r").setAtomName(5, "s")
			.startRule(CHOICERULE).addHead(3).endRule() // {z}.
			.startRule().addHead(1).addToBody(3,false).endRule()
			.startRule().addHead(2).addToBody(3,false).endRule()
			.startRule(CONSTRAINTRULE, 2).addHead(4).addToBody(1, true).addToBody(2, true).addToBody(5,true).endRule()
			.startRule().addHead(5).addToBody(4,true).addToBody(3,true).endRule()
			.startRule().addHead(4).addToBody(5,true).addToBody(3,true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		
		
		
		solver.assume(index[3].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[4].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[5].lit));

		solver.backtrack();
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT(solver.numVars() == solver.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, solver.isTrue(index[4].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[5].lit));
	}

	void testDependentExtReason() {
		// {z, q}.
		// x :- not q.
		// x :- 2 {x, y, z}.
		// x :- y, z.
		// y :- x.
		builder.startProgram(index, ufc.release(), 0)
			.setAtomName(1, "x").setAtomName(2, "y").setAtomName(3, "z").setAtomName(4, "q")
			.startRule(CHOICERULE).addHead(3).addHead(4).endRule()
			.startRule().addHead(1).addToBody(4,false).endRule()
			.startRule(CONSTRAINTRULE, 2).addHead(1).addToBody(1, true).addToBody(2, true).addToBody(3, true).endRule()
			.startRule().addHead(1).addToBody(2,true).addToBody(3, true).endRule()
			.startRule().addHead(2).addToBody(1,true).endRule()
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		
		
		// assume ~B1, where B1 = 2 {x, y, z}
		DefaultUnfoundedCheck::UfsAtomNode* a = ufc->atomNode(index[1].lit);
		Literal x = a->preds[1]->lit;

		solver.assume(~x);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()) && ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(value_free, solver.value(index[1].lit.var()));
		CPPUNIT_ASSERT_EQUAL(value_free, solver.value(index[2].lit.var()));
		// empty body + B1
		CPPUNIT_ASSERT_EQUAL(2u, solver.numAssignedVars());
		
		// assume q
		solver.assume(index[4].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		// empty body + B1 + q + {not q}
		CPPUNIT_ASSERT_EQUAL(4u, solver.numAssignedVars());

		// U = {x, y}.
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[1].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[2].lit));
		Literal extBody = a->preds[0]->lit;
		LitVec r;
		solver.reason(~index[1].lit).reason(~index[1].lit, r);
		CPPUNIT_ASSERT(r.size() == 1u && r[0] == ~extBody);
	}


	void testEqBodyDiffType() { 
		builder.startProgram(index, ufc.release())
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x").setAtomName(5,"y")
			.startRule(CHOICERULE).addHead(1).addHead(4).addHead(5).endRule()
			.startRule(CHOICERULE).addHead(2).addToBody(1,true).endRule()
			.startRule().addHead(3).addToBody(1,true).endRule()
			.startRule().addHead(2).addToBody(3,true).addToBody(4, true).endRule()
			.startRule().addHead(3).addToBody(2,true).addToBody(5,true).endRule()
		.endProgram(solver, true);
		CPPUNIT_ASSERT(builder.stats.sccs == 1);
		
		solver.assume(~index[1].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[3].lit));
	}

	void testChoiceCardInterplay() {  
		builder.startProgram(index, ufc.release(), 0)  // No eq-prepro
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "x")
			.startRule(CHOICERULE).addHead(4).endRule() // {x}.
			.startRule(CHOICERULE).addHead(1).addToBody(4, true).endRule()  // {a} :- x.
			.startRule(CONSTRAINTRULE,1).addHead(2).addToBody(1, true).addToBody(3, true).endRule() // b :- 1{a,c}
			.startRule().addHead(3).addToBody(2,true).endRule() // c :- b.
			.startRule(CHOICERULE).addHead(1).addToBody(3,true).endRule() // {a} :- c.
		.endProgram(solver, true);
		CPPUNIT_ASSERT(builder.stats.sccs == 1);
		
		solver.assume(~index[1].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
		CPPUNIT_ASSERT_EQUAL(true, ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT_EQUAL(true, solver.isFalse(index[3].lit));
	}

	void testCardInterplayOnBT() {  
		builder.startProgram(index, ufc.release(), 0)  // No eq-prepro
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d").setAtomName(5,"z")
			.startRule(CHOICERULE).addHead(1).addHead(5).endRule()                                  // {a,z}.
			.startRule(CONSTRAINTRULE,1).addHead(2).addToBody(1, true).addToBody(3, true).endRule() // b :- 1{a,c}
			.startRule(BASICRULE).addHead(2).addToBody(4, true).endRule()                           // b :- d.
			.startRule(BASICRULE).addHead(4).addToBody(2, true).endRule()                           // d :- b.
			.startRule(BASICRULE).addHead(4).addToBody(5, true).endRule()                           // d :- z.
			.startRule(BASICRULE).addHead(3).addToBody(2,true).addToBody(4,true).endRule()          // c :- b,d.      
		.endProgram(solver, true);
		CPPUNIT_ASSERT(builder.stats.sccs == 1);
		
		
		solver.assume(~index[1].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()) && ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(false, solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT_EQUAL(false, solver.isFalse(index[3].lit));
		solver.undoUntil(0);
		
		solver.assume(~index[5].lit);
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()) && ufc->propagate(solver));
		CPPUNIT_ASSERT_EQUAL(false, solver.isFalse(index[2].lit));
		CPPUNIT_ASSERT_EQUAL(false, solver.isFalse(index[3].lit));
	}

	void testIncrementalUfs() {
		builder.startProgram(index, ufc.release(), 0);
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
		CPPUNIT_ASSERT(0u == ufc->nodes());
		
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
		CPPUNIT_ASSERT(5u == ufc->nodes());
		CPPUNIT_ASSERT(1 == builder.stats.sccs);

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
		CPPUNIT_ASSERT(10u == ufc->nodes());
		CPPUNIT_ASSERT(1 == builder.stats.sccs);
	}

private:
	Solver solver;
	SingleOwnerPtr<DefaultUnfoundedCheck> ufc;
	ProgramBuilder builder;
	AtomIndex index;
	void setupSimpleProgram() {
		builder.startProgram(index, ufc.release());
		builder
			.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "f")
			.setAtomName(5, "x").setAtomName(6, "y").setAtomName(7, "z")
			.startRule(CHOICERULE).addHead(5).addHead(6).addHead(7).addHead(3).endRule()
			.startRule().addHead(2).addToBody(1, true).endRule()                    // b :- a.
			.startRule().addHead(1).addToBody(2, true).addToBody(4, true).endRule() // a :- b,f.
			.startRule().addHead(4).addToBody(1, true).addToBody(3, true).endRule() // f :- a,c.
			.startRule().addHead(1).addToBody(5, false).endRule()                   // a :- not x.
			.startRule().addHead(2).addToBody(7, false).endRule()                   // b :- not z.
			.startRule().addHead(4).addToBody(6, false).endRule()                   // f :- not y.
		;
		CPPUNIT_ASSERT_EQUAL(true, builder.endProgram(solver, true));
		
		CPPUNIT_ASSERT_EQUAL(true, solver.endAddConstraints());
		CPPUNIT_ASSERT_EQUAL(true, solver.propagateUntil(ufc.get()));
	}
};
CPPUNIT_TEST_SUITE_REGISTRATION(UnfoundedCheckTest);
} } 
