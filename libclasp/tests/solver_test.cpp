#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/clasp_facade.h>
#include "common.h"

namespace Clasp { namespace Test {
 
struct TestingConstraint : public LearntConstraint {
	TestingConstraint(bool* del = 0, ConstraintType t = Constraint_t::native_constraint) 
		: type_(t), propagates(0), undos(0), sat(false), keepWatch(true), setConflict(false), deleted(del) {}
	PropResult propagate(const Literal&, uint32&, Solver& s) {
		++propagates;
		return PropResult(!setConflict, keepWatch);
	}
	void undoLevel(Solver&) {
		++undos;
	}
	bool simplify(Solver& s, bool) { return sat; }
	void reason(const Literal&, LitVec&) {}
	void destroy() {
		if (deleted) *deleted = true;
		LearntConstraint::destroy();
	}
	bool locked(const Solver& s) const { return false; }
	void removeWatches(Solver&) {}
	bool isSatisfied(const Solver&, LitVec&) const { return true; }
	LitVec::size_type size() const { return LitVec::size_type(-1); }
	ConstraintType type() const { return type_; }
	ConstraintType type_;
	int propagates;
	int undos;
	bool sat;
	bool keepWatch;
	bool setConflict;
	bool* deleted;
};
struct TestingPostProp : public PostPropagator {
	explicit TestingPostProp(bool cfl) : props(0), resets(0), symMods(0), prio(PostPropagator::priority_single), conflict(cfl), symModel(false) {}
	bool propagate(Solver&) {
		++props;
		return !conflict;
	}
	void reset() {
		++resets;
	}
	uint32 priority() const { return prio; }
	bool nextSymModel(Solver& s, bool expand) {
		++symMods;
		return expand && symModel; 
	}
	int props;
	int resets;
	int symMods;
	uint32 prio;
	bool conflict;
	bool symModel; 
};

class SolverTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(SolverTest);
	CPPUNIT_TEST(testDefaults);
	CPPUNIT_TEST(testVarNullIsSentinel);
	CPPUNIT_TEST(testSolverAlwaysContainsSentinelVar);
	CPPUNIT_TEST(testSolverOwnsConstraints);
	CPPUNIT_TEST(testAddVar);
	CPPUNIT_TEST(testEliminateVar);
	CPPUNIT_TEST(testResurrectVar);

	CPPUNIT_TEST(testPreferredLitByType);
	CPPUNIT_TEST(testInitSavedValue);
	CPPUNIT_TEST(testReset);
	CPPUNIT_TEST(testForce);
	CPPUNIT_TEST(testNoUpdateOnConsistentAssign);
	CPPUNIT_TEST(testAssume);
	CPPUNIT_TEST(testGetDecision);
	CPPUNIT_TEST(testAddWatch);
	CPPUNIT_TEST(testRemoveWatch);
	CPPUNIT_TEST(testNotifyWatch);
	CPPUNIT_TEST(testKeepWatchOnPropagate);
	CPPUNIT_TEST(testRemoveWatchOnPropagate);
	CPPUNIT_TEST(testWatchOrder);
	CPPUNIT_TEST(testUndoUntil);
	CPPUNIT_TEST(testUndoWatches);
	CPPUNIT_TEST(testPropBinary);
	CPPUNIT_TEST(testPropTernary);
	CPPUNIT_TEST(testRestartAfterUnitLitResolvedBug);
	CPPUNIT_TEST(testEstimateBCP);
	CPPUNIT_TEST(testEstimateBCPLoop);
	CPPUNIT_TEST(testAssertImmediate);
	CPPUNIT_TEST(testPreferShortBfs);

	CPPUNIT_TEST(testPropagateCallsPostProp);
	CPPUNIT_TEST(testPropagateCallsResetOnConflict);
	CPPUNIT_TEST(testPostpropPriority);
	CPPUNIT_TEST(testPostpropSymModel);

	CPPUNIT_TEST(testSimplifyRemovesSatBinClauses);
	CPPUNIT_TEST(testSimplifyRemovesSatTernClauses);
	CPPUNIT_TEST(testSimplifyRemovesSatConstraints);


	CPPUNIT_TEST(testResolveUnary);
	CPPUNIT_TEST(testResolveConflict);
	CPPUNIT_TEST(testResolveConflictBounded);

	CPPUNIT_TEST(testClearAssumptions);

	CPPUNIT_TEST(testSearchKeepsAssumptions);
	CPPUNIT_TEST(testSearchAddsLearntFacts);
	CPPUNIT_TEST(testSearchMaxConflicts);

	CPPUNIT_TEST(testStats);
	CPPUNIT_TEST(testIncrementalSolve);
	CPPUNIT_TEST_SUITE_END(); 
public:
	void testDefaults() {
		const SolverStrategies& x = s.strategies();
		CPPUNIT_ASSERT(x.heuristic.get() != 0);
		CPPUNIT_ASSERT(x.strengthenRecursive == false);
		CPPUNIT_ASSERT(x.cflMinAntes == SolverStrategies::all_antes);
		CPPUNIT_ASSERT(x.search == SolverStrategies::use_learning);
		CPPUNIT_ASSERT(x.compress() == uint32(250));
		CPPUNIT_ASSERT(x.randomWatches == false);

		CPPUNIT_ASSERT_EQUAL(0u, s.numVars());
		CPPUNIT_ASSERT_EQUAL(0u, s.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(0u, s.numConstraints());
		CPPUNIT_ASSERT_EQUAL(0u, s.numLearntConstraints());
		CPPUNIT_ASSERT_EQUAL(0u, s.decisionLevel());
		CPPUNIT_ASSERT_EQUAL(0u, s.queueSize());
	}
	void testVarNullIsSentinel() {
		Literal p = posLit(0);
		CPPUNIT_ASSERT_EQUAL(true, isSentinel(p));
		CPPUNIT_ASSERT_EQUAL(true, isSentinel(~p));
	}
	void testSolverAlwaysContainsSentinelVar() {
		CPPUNIT_ASSERT_EQUAL(value_true, s.value(sentVar));
		CPPUNIT_ASSERT(s.isTrue(posLit(sentVar)));
		CPPUNIT_ASSERT(s.isFalse(negLit(sentVar)));
		CPPUNIT_ASSERT(s.seen(sentVar) == true);
	}
	void testSolverOwnsConstraints() {
		bool conDel = false;
		bool lconDel = false;
		{
			Solver s;
			s.startAddConstraints();
			s.add( new TestingConstraint(&conDel) );
			s.endAddConstraints();
			s.addLearnt( new TestingConstraint(&lconDel, Constraint_t::learnt_conflict), 10 );
			CPPUNIT_ASSERT_EQUAL(1u, s.numConstraints());
			CPPUNIT_ASSERT_EQUAL(1u, s.numLearntConstraints());
		}
		CPPUNIT_ASSERT_EQUAL(true, conDel);
		CPPUNIT_ASSERT_EQUAL(true, lconDel);
	}

	void testAddVar() {
		Var v1 = s.addVar(Var_t::atom_var);
		Var v2 = s.addVar(Var_t::body_var);
		CPPUNIT_ASSERT_EQUAL(2u, s.numVars());
		CPPUNIT_ASSERT_EQUAL(0u, s.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(2u, s.numFreeVars());
		CPPUNIT_ASSERT_EQUAL(Var_t::atom_var, s.type(v1));
		CPPUNIT_ASSERT_EQUAL(Var_t::body_var, s.type(v2));

		CPPUNIT_ASSERT_EQUAL( negLit(v1), s.preferredLiteralByType(v1) );   
		CPPUNIT_ASSERT_EQUAL( posLit(v2), s.preferredLiteralByType(v2) );
	}

	void testEliminateVar() {
		Var v1 = s.addVar(Var_t::atom_var);
		Var v2 = s.addVar(Var_t::body_var);
		s.eliminate(v1, true);
		CPPUNIT_ASSERT_EQUAL(2u, s.numVars());
		CPPUNIT_ASSERT_EQUAL(1u, s.numEliminatedVars());
		CPPUNIT_ASSERT_EQUAL(1u, s.numFreeVars());
		CPPUNIT_ASSERT_EQUAL(0u, s.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(true, s.eliminated(v1));
		// so v1 is ignored by heuristics!
		CPPUNIT_ASSERT(s.value(v1) != value_free);

		// ignore subsequent calls 
		s.eliminate(v1, true);
		CPPUNIT_ASSERT_EQUAL(1u, s.numEliminatedVars());
	}
	void testResurrectVar() {
		Var v1 = s.addVar(Var_t::atom_var);
		Var v2 = s.addVar(Var_t::body_var);
		struct Dummy : public SelectFirst {
			Dummy() : res(0) {}
			void resurrect(const Solver&, Var v) { res = v; }

			Var res;
		}*h = new Dummy();
		s.strategies().heuristic.reset(h);
		// noop if v2 is not eliminated
		s.eliminate(v2, false);
		CPPUNIT_ASSERT_EQUAL(Var(0), h->res);
		
		s.eliminate(v2, true);
		CPPUNIT_ASSERT_EQUAL(1u, s.numEliminatedVars());
		CPPUNIT_ASSERT_EQUAL(1u, s.numFreeVars());
		
		s.eliminate(v2, false);
		CPPUNIT_ASSERT_EQUAL(v2, h->res);
		CPPUNIT_ASSERT_EQUAL(0u, s.numEliminatedVars());
		CPPUNIT_ASSERT_EQUAL(2u, s.numFreeVars());
	}

	void testPreferredLitByType() {
		Var v1 = s.addVar(Var_t::atom_var);
		Var v2 = s.addVar(Var_t::body_var);
		Var v3 = s.addVar(Var_t::atom_var, true);
		Var v4 = s.addVar(Var_t::body_var, true);
		CPPUNIT_ASSERT_EQUAL( negLit(v1), s.preferredLiteralByType(v1) );   
		CPPUNIT_ASSERT_EQUAL( posLit(v2), s.preferredLiteralByType(v2) );
		CPPUNIT_ASSERT_EQUAL( negLit(v3), s.preferredLiteralByType(v3) );   
		CPPUNIT_ASSERT_EQUAL( posLit(v4), s.preferredLiteralByType(v4) );
	}

	void testInitSavedValue() {
		Var v1 = s.addVar(Var_t::atom_var);
		Var v2 = s.addVar(Var_t::body_var);
		CPPUNIT_ASSERT_EQUAL( value_free, s.savedValue(v1) );   
		CPPUNIT_ASSERT_EQUAL( value_free, s.savedValue(v2) );

		s.initSavedValue(v1, value_true);
		s.initSavedValue(v2, value_false);

		CPPUNIT_ASSERT_EQUAL( value_true, s.savedValue(v1) );   
		CPPUNIT_ASSERT_EQUAL( value_false, s.savedValue(v2) );
	}

	void testReset() {
		s.strategies().randomWatches = true;
		s.addVar(Var_t::atom_var); s.addVar(Var_t::body_var);
		s.startAddConstraints();
		s.add( new TestingConstraint(0) );
		s.endAddConstraints();
		s.addLearnt( new TestingConstraint(0, Constraint_t::learnt_conflict),10 );
		s.assume( posLit(1) );
		s.reset();
		testDefaults();
		Var n = s.addVar(Var_t::body_var);
		CPPUNIT_ASSERT_EQUAL(Var_t::body_var, s.type(n));
	}

	void testForce() {
		Var v1 = s.addVar(Var_t::atom_var);
		Var v2 = s.addVar(Var_t::atom_var);
		CPPUNIT_ASSERT_EQUAL(true, s.force(posLit(v1), 0));
		CPPUNIT_ASSERT_EQUAL(true, s.force(negLit(v2), posLit(v1)));
		CPPUNIT_ASSERT_EQUAL(true, s.isTrue(posLit(v1)));
		CPPUNIT_ASSERT_EQUAL(true, s.isTrue(negLit(v2)));
		CPPUNIT_ASSERT(s.reason(negLit(v2)).type() == Antecedent::binary_constraint);

		CPPUNIT_ASSERT_EQUAL(2u, s.queueSize());
	}

	void testNoUpdateOnConsistentAssign() {
		Var v1 = s.addVar(Var_t::atom_var);
		Var v2 = s.addVar(Var_t::atom_var);
		s.force( posLit(v2), 0 );
		s.force( posLit(v1), 0 );
		uint32 oldA = s.numAssignedVars();
		CPPUNIT_ASSERT_EQUAL(true, s.force( posLit(v1), posLit(v2) ));
		CPPUNIT_ASSERT_EQUAL(oldA, s.numAssignedVars());
		CPPUNIT_ASSERT_EQUAL(2u, s.queueSize());
	}

	void testAssume() {
		Literal p = posLit(s.addVar(Var_t::atom_var));
		CPPUNIT_ASSERT_EQUAL(true, s.assume(p));
		CPPUNIT_ASSERT_EQUAL(value_true, s.value(p.var()));
		CPPUNIT_ASSERT_EQUAL(1u, s.decisionLevel());
		CPPUNIT_ASSERT_EQUAL(1u, s.queueSize());
	}

	void testGetDecision() {
		Literal p = posLit(s.addVar(Var_t::atom_var));
		Literal q = posLit(s.addVar(Var_t::atom_var));
		Literal r = posLit(s.addVar(Var_t::atom_var));
		s.assume(p);
		s.assume(q);
		s.assume(~r);
		CPPUNIT_ASSERT_EQUAL(p, s.decision(1));
		CPPUNIT_ASSERT_EQUAL(q, s.decision(2));
		CPPUNIT_ASSERT_EQUAL(~r, s.decision(3));
		CPPUNIT_ASSERT_EQUAL(~r, s.decision(s.decisionLevel()));
	}
	void testAddWatch() {
		Literal p = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		TestingConstraint c;
		CPPUNIT_ASSERT_EQUAL(false, s.hasWatch(p, &c));
		s.addWatch(p, &c);
		CPPUNIT_ASSERT_EQUAL(true, s.hasWatch(p, &c));
		CPPUNIT_ASSERT_EQUAL(1u, s.numWatches(p));
	}

	void testRemoveWatch() {
		Literal p = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		TestingConstraint c;
		s.addWatch(p, &c);
		s.removeWatch(p, &c);
		CPPUNIT_ASSERT_EQUAL(false, s.hasWatch(p, &c));
	}

	void testNotifyWatch() {
		Literal p = posLit(s.addVar(Var_t::atom_var)), q = posLit(s.addVar(Var_t::atom_var));
		TestingConstraint c;
		s.startAddConstraints();
		s.endAddConstraints();
		s.addWatch(p, &c);
		s.addWatch(q, &c);
		s.assume(p);
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(1, c.propagates);
		s.assume(~q);
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(1, c.propagates);
	}

	void testKeepWatchOnPropagate() {
		Literal p = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		s.endAddConstraints();
		TestingConstraint c;
		s.addWatch(p, &c);
		s.assume(p);
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(true, s.hasWatch(p, &c));
	}

	void testRemoveWatchOnPropagate() {
		Literal p = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		s.endAddConstraints();
		TestingConstraint c;
		c.keepWatch = false;
		s.addWatch(p, &c);
		s.assume(p);
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(false, s.hasWatch(p, &c));
	}

	void testWatchOrder() {
		Literal p = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		s.endAddConstraints();
		TestingConstraint c1, c2, c3;
		c1.keepWatch = false;
		c2.setConflict = true;
		s.addWatch(p, &c1);
		s.addWatch(p, &c2);
		s.addWatch(p, &c3);
		s.assume(p);
		CPPUNIT_ASSERT_EQUAL(false, s.propagate());
		CPPUNIT_ASSERT_EQUAL(false, s.hasWatch(p, &c1));
		CPPUNIT_ASSERT_EQUAL(true, s.hasWatch(p, &c2));
		CPPUNIT_ASSERT_EQUAL(true, s.hasWatch(p, &c3));
		CPPUNIT_ASSERT_EQUAL(1, c1.propagates);
		CPPUNIT_ASSERT_EQUAL(1, c2.propagates);
		CPPUNIT_ASSERT_EQUAL(0, c3.propagates);
	}

	void testUndoUntil() {
		Literal a = posLit(s.addVar(Var_t::atom_var)), b = posLit(s.addVar(Var_t::atom_var))
			, c = posLit(s.addVar(Var_t::atom_var)), d = posLit(s.addVar(Var_t::atom_var));
		
		s.assume(a);
		s.force(~b, a);
		s.force(~c, a);
		s.force(d, a);
		CPPUNIT_ASSERT_EQUAL(4u, s.queueSize());
		CPPUNIT_ASSERT_EQUAL(4u, s.numAssignedVars());
		s.undoUntil(0u);
		CPPUNIT_ASSERT_EQUAL(0u, s.numAssignedVars());
		for (Var i = a.var(); i != d.var()+1; ++i) {
			CPPUNIT_ASSERT_EQUAL(value_free, s.value(i));
		}
	}

	void testUndoWatches() {
		Literal a = posLit(s.addVar(Var_t::atom_var)), b = posLit(s.addVar(Var_t::atom_var));
		TestingConstraint c;
		s.startAddConstraints();
		s.endAddConstraints();
		s.assume(a);
		s.addUndoWatch(1, &c);
		s.assume(b);
		s.undoUntil(1);
		CPPUNIT_ASSERT_EQUAL(0, c.undos);
		s.undoUntil(0);
		CPPUNIT_ASSERT_EQUAL(1, c.undos);
	}

	void testPropBinary() {
		LitVec bin = addBinary();
		for (int i = 0; i < 2; ++i) {
			s.assume(~bin[i]);
			CPPUNIT_ASSERT(s.propagate());
			int o = (i+1)%2;
			CPPUNIT_ASSERT_EQUAL(true, s.isTrue(bin[o]));
			Antecedent reason = s.reason(bin[o]);
			CPPUNIT_ASSERT_EQUAL(Antecedent::binary_constraint, reason.type());
			LitVec r;
			reason.reason(bin[o], r);
			CPPUNIT_ASSERT_EQUAL(1u, (uint32)r.size());
			CPPUNIT_ASSERT(~bin[i] == r[0]);
			s.undoUntil(0);
		}
		s.assume(~bin[0]);
		s.force(~bin[1], 0);
		CPPUNIT_ASSERT_EQUAL(false, s.propagate());
		const LitVec& r = s.conflict();
		CPPUNIT_ASSERT_EQUAL(2u, (uint32)r.size());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~bin[0]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~bin[1]) != r.end());
	}

	void testPropTernary() {
		LitVec tern = addTernary();
		for (int i = 0; i < 3; ++i) {
			s.assume(~tern[i]);
			s.assume(~tern[(i+1)%3]);
			CPPUNIT_ASSERT(s.propagate());
			int o = (i+2)%3;
			CPPUNIT_ASSERT_EQUAL(true, s.isTrue(tern[o]));
			Antecedent reason = s.reason(tern[o]);
			CPPUNIT_ASSERT_EQUAL(Antecedent::ternary_constraint, reason.type());
			LitVec r;
			reason.reason(tern[o], r);
			CPPUNIT_ASSERT_EQUAL(2u, (uint32)r.size());
			CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~tern[i]) != r.end());
			CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~tern[(i+1)%3]) != r.end());
			s.undoUntil(0);
		}
		s.assume(~tern[0]);
		s.force(~tern[1], 0);
		s.force(~tern[2], 0);
		CPPUNIT_ASSERT_EQUAL(false, s.propagate());
		const LitVec& r = s.conflict();
		CPPUNIT_ASSERT_EQUAL(3u, (uint32)r.size());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~tern[0]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~tern[1]) != r.end());
		CPPUNIT_ASSERT(std::find(r.begin(), r.end(), ~tern[2]) != r.end());
	}

	void testRestartAfterUnitLitResolvedBug() {
		LitVec bin = addBinary();
		s.force(~bin[0], 0);
		s.undoUntil(0);
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(true, s.isTrue(~bin[0]));
		CPPUNIT_ASSERT_EQUAL(true, s.isTrue(bin[1]));
	}

	void testEstimateBCP() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		Literal b = posLit(s.addVar(Var_t::atom_var));
		Literal c = posLit(s.addVar(Var_t::atom_var));
		Literal d = posLit(s.addVar(Var_t::atom_var));
		Literal e = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		s.addBinary(a, b, false);
		s.addBinary(~b, c, false);
		s.addBinary(~c, d, false);
		s.addBinary(~d, e, false);
		s.endAddConstraints();
		for (int i = 0; i < 4; ++i) {
			uint32 est = s.estimateBCP(~a, i);
			CPPUNIT_ASSERT_EQUAL(uint32(i + 2), est);
		}
	}

	void testEstimateBCPLoop() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		Literal b = posLit(s.addVar(Var_t::atom_var));
		Literal c = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		s.addBinary(a, b, false);
		s.addBinary(~b, c, false);
		s.addBinary(~c, ~a, false);
		s.endAddConstraints();
		CPPUNIT_ASSERT_EQUAL(uint32(3), s.estimateBCP(~a, -1));
	}

	void testAssertImmediate() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		Literal b = posLit(s.addVar(Var_t::atom_var));
		Literal d = posLit(s.addVar(Var_t::atom_var));
		Literal q = posLit(s.addVar(Var_t::atom_var));
		Literal f = posLit(s.addVar(Var_t::atom_var));
		Literal x = posLit(s.addVar(Var_t::atom_var));
		Literal z = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		ClauseCreator cl(&s);
		cl.start().add(~z).add(d).end();
		cl.start().add(a).add(b).end();
		cl.start().add(a).add(~b).add(z).end();
		cl.start().add(a).add(~b).add(~z).add(d).end();
		cl.start().add(~b).add(~z).add(~d).add(q).end();
		cl.start().add(~q).add(f).end();
		cl.start().add(~f).add(~z).add(x).end();
		s.assume( ~a );
		CPPUNIT_ASSERT_EQUAL( true, s.propagate() );

		CPPUNIT_ASSERT_EQUAL( 7u, s.numAssignedVars());

		Antecedent whyB = s.reason(b);
		Antecedent whyZ = s.reason(z);
		Antecedent whyD = s.reason(d);
		Antecedent whyQ = s.reason(q);
		Antecedent whyF = s.reason(f);
		Antecedent whyX = s.reason(x);

		CPPUNIT_ASSERT(whyB.type() == Antecedent::binary_constraint && whyB.firstLiteral() == ~a);
		CPPUNIT_ASSERT(whyZ.type() == Antecedent::ternary_constraint && whyZ.firstLiteral() == ~a && whyZ.secondLiteral() == b);
		CPPUNIT_ASSERT(whyD.type() == Antecedent::generic_constraint);
		CPPUNIT_ASSERT(whyQ.type() == Antecedent::generic_constraint);
		
		CPPUNIT_ASSERT(whyF.type() == Antecedent::binary_constraint && whyF.firstLiteral() == q);
		CPPUNIT_ASSERT(whyX.type() == Antecedent::ternary_constraint && whyX.firstLiteral() == f && whyX.secondLiteral() == z);
	}

	void testPreferShortBfs() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		Literal b = posLit(s.addVar(Var_t::atom_var));
		Literal p = posLit(s.addVar(Var_t::atom_var));
		Literal q = posLit(s.addVar(Var_t::atom_var));
		Literal x = posLit(s.addVar(Var_t::atom_var));
		Literal y = posLit(s.addVar(Var_t::atom_var));
		Literal z = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		ClauseCreator cl(&s);
		cl.start().add(a).add(x).add(y).add(p).end();   // c1
		cl.start().add(a).add(x).add(y).add(z).end();   // c2
		cl.start().add(a).add(p).end();                 // c3
		cl.start().add(a).add(~p).add(z).end();         // c4
		cl.start().add(~z).add(b).end();                // c5
		cl.start().add(a).add(x).add(q).add(~b).end();  // c6
		cl.start().add(a).add(~b).add(~p).add(~q).end();// c7
		
		CPPUNIT_ASSERT_EQUAL(7u, s.numConstraints());
		CPPUNIT_ASSERT_EQUAL(2u, s.numBinaryConstraints());
		CPPUNIT_ASSERT_EQUAL(1u, s.numTernaryConstraints());
		
		s.assume( ~x );
		s.propagate();
		s.assume( ~y );
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(2u, s.numAssignedVars());
		s.assume( ~a );
		
		CPPUNIT_ASSERT_EQUAL(false, s.propagate());

		CPPUNIT_ASSERT_EQUAL( 7u, s.numAssignedVars());

		CPPUNIT_ASSERT( s.reason(b).type() == Antecedent::binary_constraint );
		CPPUNIT_ASSERT( s.reason(p).type() == Antecedent::binary_constraint );
		CPPUNIT_ASSERT( s.reason(z).type() == Antecedent::ternary_constraint );
		CPPUNIT_ASSERT( s.reason(q).type() == Antecedent::generic_constraint );
	}

	void testPropagateCallsPostProp() {
		TestingPostProp* p = new TestingPostProp(false);
		s.addPost(p);
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(1, p->props);
		CPPUNIT_ASSERT_EQUAL(0, p->resets);
	}
	void testPropagateCallsResetOnConflict() {
		TestingPostProp* p = new TestingPostProp(true);
		s.addPost(p);
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(1, p->props);
		CPPUNIT_ASSERT_EQUAL(1, p->resets);
	}
	
	void testPostpropPriority() {
		TestingPostProp* p1 = new TestingPostProp(false);
		p1->prio = PostPropagator::priority_single_high;
		TestingPostProp* p2 = new TestingPostProp(false);
		p2->prio = PostPropagator::priority_single_low;
		TestingPostProp* p3 = new TestingPostProp(false);
		s.addPost(p2);
		s.addPost(p1);
		s.addPost(p3);
		CPPUNIT_ASSERT(p1->next == p3 && p3->next == p2);
	}

	void testPostpropSymModel() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		Literal b = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		TestingPostProp* p1 = new TestingPostProp(false);
		TestingPostProp* p2 = new TestingPostProp(false);
		s.addPost(p1);
		s.addPost(p2);
		s.endAddConstraints();
		p1->symModel = false;
		p2->symModel = true;
		ValueRep v = s.search(1, 1);
		CPPUNIT_ASSERT_EQUAL(true, s.nextSymModel(true));
		CPPUNIT_ASSERT_EQUAL(1, p1->symMods);
		CPPUNIT_ASSERT_EQUAL(1, p2->symMods);
		p2->symModel = false;
		CPPUNIT_ASSERT_EQUAL(false, s.nextSymModel(true));
		CPPUNIT_ASSERT_EQUAL(1, p1->symMods);
		CPPUNIT_ASSERT_EQUAL(2, p2->symMods);
		s.undoUntil(0);
		v = s.search(1, 1);
		p1->symModel = true;
		CPPUNIT_ASSERT_EQUAL(true, s.nextSymModel(true));
		CPPUNIT_ASSERT_EQUAL(2, p1->symMods);
		CPPUNIT_ASSERT_EQUAL(2, p2->symMods);
		CPPUNIT_ASSERT_EQUAL(true, s.nextSymModel(true));
		CPPUNIT_ASSERT_EQUAL(3, p1->symMods);
		CPPUNIT_ASSERT_EQUAL(2, p2->symMods);
		p1->symModel = false;
		CPPUNIT_ASSERT_EQUAL(false, s.nextSymModel(true));
		CPPUNIT_ASSERT_EQUAL(4, p1->symMods);
		CPPUNIT_ASSERT_EQUAL(3, p2->symMods);
	}

	void testSimplifyRemovesSatBinClauses() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		Literal b = posLit(s.addVar(Var_t::atom_var));
		Literal c = posLit(s.addVar(Var_t::atom_var));
		Literal d = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		s.addBinary(a, b, false);
		s.addBinary(a, c, false);
		s.addBinary(~a, d, false);
		s.force(a, 0);
		s.simplify();
		CPPUNIT_ASSERT_EQUAL(0u, s.numBinaryConstraints());
	}

	void testSimplifyRemovesSatTernClauses() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		Literal b = posLit(s.addVar(Var_t::atom_var));
		Literal c = posLit(s.addVar(Var_t::atom_var));
		Literal d = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		s.addTernary(a, b, d, false);
		s.addTernary(~a, b, c, false);
		s.force(a, 0);
		s.simplify(); 
		s.assume(~b);
		CPPUNIT_ASSERT_EQUAL(0u, s.numTernaryConstraints());
		
		// simplify transformed the tern-clause ~a b c to the bin clause b c
		// because ~a is false on level 0
		CPPUNIT_ASSERT_EQUAL(1u, s.numBinaryConstraints());
		s.propagate();
		CPPUNIT_ASSERT_EQUAL(true, s.isTrue(c));
	}
	
	void testSimplifyRemovesSatConstraints() {
		Literal a = posLit(s.addVar(Var_t::atom_var));
		s.startAddConstraints();
		TestingConstraint* t1;
		TestingConstraint* t2;
		TestingConstraint* t3;
		TestingConstraint* t4;
		bool t2Del = false, t3Del = false;
		s.add( t1 = new TestingConstraint );
		s.add( t2 = new TestingConstraint(&t2Del) );
		s.endAddConstraints();
		s.addLearnt( t3 = new TestingConstraint(&t3Del, Constraint_t::learnt_conflict),10 );
		s.addLearnt( t4 = new TestingConstraint(0, Constraint_t::learnt_conflict),10 );
		t1->sat = false;
		t2->sat = true;
		t3->sat = true;
		t4->sat = false;
		CPPUNIT_ASSERT_EQUAL(2u, s.numLearntConstraints());
		CPPUNIT_ASSERT_EQUAL(2u, s.numLearntConstraints());
		s.force( a, 0 );
		s.simplify();
		CPPUNIT_ASSERT_EQUAL(1u, s.numLearntConstraints());
		CPPUNIT_ASSERT_EQUAL(1u, s.numLearntConstraints());
		CPPUNIT_ASSERT_EQUAL(true, t2Del);
		CPPUNIT_ASSERT_EQUAL(true, t3Del);
	}

	void testResolveUnary() {
		ClauseCreator cl(&s);
		Var a = s.addVar( Var_t::atom_var );
		Var b = s.addVar( Var_t::atom_var );
		Var c = s.addVar( Var_t::atom_var );
		s.startAddConstraints();
		s.addBinary(posLit(a), posLit(b), false);
		s.addBinary(negLit(b), posLit(c), false);
		s.addBinary(negLit(a), posLit(c), false);
		s.assume( negLit(c) );
		CPPUNIT_ASSERT_EQUAL(false, s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.resolveConflict());
		CPPUNIT_ASSERT_EQUAL(false, s.hasConflict());
		CPPUNIT_ASSERT_EQUAL(true, s.isTrue(posLit(c)));
		CPPUNIT_ASSERT_EQUAL(0u, s.decisionLevel());
	}

	void testResolveConflict() {
		ClauseCreator cl(&s);
		Literal x1 = posLit(s.addVar( Var_t::atom_var )); Literal x2 = posLit(s.addVar( Var_t::atom_var ));
		Literal x3 = posLit(s.addVar( Var_t::atom_var )); Literal x4 = posLit(s.addVar( Var_t::atom_var ));
		Literal x5 = posLit(s.addVar( Var_t::atom_var )); Literal x6 = posLit(s.addVar( Var_t::atom_var ));
		Literal x7 = posLit(s.addVar( Var_t::atom_var )); Literal x8 = posLit(s.addVar( Var_t::atom_var ));
		Literal x9 = posLit(s.addVar( Var_t::atom_var )); Literal x10 = posLit(s.addVar( Var_t::atom_var ));
		Literal x11 = posLit(s.addVar( Var_t::atom_var )); Literal x12 = posLit(s.addVar( Var_t::atom_var ));
		Literal x13 = posLit(s.addVar( Var_t::atom_var )); Literal x14 = posLit(s.addVar( Var_t::atom_var ));
		Literal x15 = posLit(s.addVar( Var_t::atom_var )); Literal x16 = posLit(s.addVar( Var_t::atom_var ));
		Literal x17 = posLit(s.addVar( Var_t::atom_var ));
		s.startAddConstraints();
		cl.start().add(~x11).add(x12).end();
		cl.start().add(x1).add(~x12).add(~x13).end();
		cl.start().add(~x4).add(~x12).add(x14).end();
		cl.start().add(x13).add(~x14).add(~x15).end();
		cl.start().add(~x2).add(x15).add(x16).end();
		cl.start().add(x3).add(x15).add(~x17).end();
		cl.start().add(~x6).add(~x16).add(x17).end();
		cl.start().add(~x2).add(x9).add(x10).end();
		cl.start().add(~x4).add(~x7).add(~x8).end();
		cl.start().add(x5).add(x6).end();
		CPPUNIT_ASSERT_EQUAL(true, s.endAddConstraints());
		CPPUNIT_ASSERT_EQUAL(0u, s.queueSize());
		
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x1) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(x2) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x3) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(x4) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x5) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(x7) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x9) && s.propagate());

		CPPUNIT_ASSERT_EQUAL(false, s.assume(x11) && s.propagate());
		
		CPPUNIT_ASSERT_EQUAL(true, s.resolveConflict());
		CPPUNIT_ASSERT_EQUAL(s.assignment().back(), x15); // UIP
		CPPUNIT_ASSERT_EQUAL(5u, s.decisionLevel());
		Antecedent ante = s.reason(s.assignment().back());
		CPPUNIT_ASSERT_EQUAL(Antecedent::generic_constraint, ante.type());
		Clause* cflClause = (Clause*)ante.constraint();
		CPPUNIT_ASSERT(LitVec::size_type(4) == cflClause->size());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), ~x2) != cflClause->end());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), x3) != cflClause->end());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), ~x6) != cflClause->end());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), x15) != cflClause->end());
		CPPUNIT_ASSERT_EQUAL(true, s.hasWatch(x6, cflClause));
		
	}

	void testResolveConflictBounded() {
		ClauseCreator cl(&s);
		Literal x1 = posLit(s.addVar( Var_t::atom_var )); Literal x2 = posLit(s.addVar( Var_t::atom_var ));
		Literal x3 = posLit(s.addVar( Var_t::atom_var )); Literal x4 = posLit(s.addVar( Var_t::atom_var ));
		Literal x5 = posLit(s.addVar( Var_t::atom_var )); Literal x6 = posLit(s.addVar( Var_t::atom_var ));
		Literal x7 = posLit(s.addVar( Var_t::atom_var )); Literal x8 = posLit(s.addVar( Var_t::atom_var ));
		Literal x9 = posLit(s.addVar( Var_t::atom_var )); Literal x10 = posLit(s.addVar( Var_t::atom_var ));
		Literal x11 = posLit(s.addVar( Var_t::atom_var )); Literal x12 = posLit(s.addVar( Var_t::atom_var ));
		Literal x13 = posLit(s.addVar( Var_t::atom_var )); Literal x14 = posLit(s.addVar( Var_t::atom_var ));
		Literal x15 = posLit(s.addVar( Var_t::atom_var )); Literal x16 = posLit(s.addVar( Var_t::atom_var ));
		Literal x17 = posLit(s.addVar( Var_t::atom_var )); Literal x18 = posLit(s.addVar( Var_t::atom_var ));
		s.startAddConstraints();
		cl.start().add(~x11).add(x12).end();
		cl.start().add(x1).add(~x12).add(~x13).end();
		cl.start().add(~x4).add(~x12).add(x14).end();
		cl.start().add(x13).add(~x14).add(~x15).end();
		cl.start().add(~x2).add(x15).add(x16).end();
		cl.start().add(x3).add(x15).add(~x17).end();
		cl.start().add(~x6).add(~x16).add(x17).end();
		cl.start().add(~x2).add(x9).add(x10).end();
		cl.start().add(~x4).add(~x7).add(~x8).end();
		cl.start().add(x5).add(x6).end();
		CPPUNIT_ASSERT_EQUAL(true, s.endAddConstraints());
		CPPUNIT_ASSERT_EQUAL(0u, s.queueSize());
		
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x1) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(x2) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x3) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(x4) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x5) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.assume(x7) && s.propagate());

		// force backtrack-level to 6
		CPPUNIT_ASSERT_EQUAL(true, s.assume(x18) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.backtrack());
		
		CPPUNIT_ASSERT_EQUAL(true, s.assume(~x9) && s.propagate());
		CPPUNIT_ASSERT_EQUAL(false, s.assume(x11) && s.propagate());
		
		CPPUNIT_ASSERT_EQUAL(true, s.resolveConflict());
		CPPUNIT_ASSERT_EQUAL(s.assignment().back(), x15); // UIP
		CPPUNIT_ASSERT_EQUAL(6u, s.decisionLevel());  // Jump was bounded!
		Antecedent ante = s.reason(s.assignment().back());
		CPPUNIT_ASSERT_EQUAL(Antecedent::generic_constraint, ante.type());
		Clause* cflClause = (Clause*)ante.constraint();
		CPPUNIT_ASSERT(LitVec::size_type(4) == cflClause->size());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), ~x2) != cflClause->end());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), x3) != cflClause->end());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), ~x6) != cflClause->end());
		CPPUNIT_ASSERT(std::find(cflClause->begin(), cflClause->end(), x15) != cflClause->end());
		CPPUNIT_ASSERT_EQUAL(true, s.hasWatch(x6, cflClause));

		CPPUNIT_ASSERT_EQUAL(true, s.backtrack());
		CPPUNIT_ASSERT_EQUAL(true, s.isTrue(x15));  // still true, because logically implied on level 5
		CPPUNIT_ASSERT_EQUAL(true, s.backtrack());
		CPPUNIT_ASSERT_EQUAL(value_free, s.value(x15.var()));
	}
	
	void testSearchKeepsAssumptions() {
		ClauseCreator cl(&s);
		Var a = s.addVar( Var_t::atom_var );
		Var b = s.addVar( Var_t::atom_var );
		Var c = s.addVar( Var_t::atom_var );
		Var d = s.addVar( Var_t::atom_var );
		s.startAddConstraints();
		s.addBinary(posLit(a), posLit(b), false);
		s.addBinary(negLit(b), posLit(c), false);
		s.addBinary(negLit(a), posLit(c), false);
		s.addBinary(negLit(c), negLit(d), false);
		s.endAddConstraints();
		s.simplify();
		s.assume( posLit(d) );
		s.setRootLevel(1);
		CPPUNIT_ASSERT_EQUAL(value_false, s.search(-1, -1, 0));
		CPPUNIT_ASSERT_EQUAL(1u, s.decisionLevel());
	}
	void testSearchAddsLearntFacts() {
		ClauseCreator cl(&s);
		Var a = s.addVar( Var_t::atom_var );
		Var b = s.addVar( Var_t::atom_var );
		Var c = s.addVar( Var_t::atom_var );
		Var d = s.addVar( Var_t::atom_var );
		struct Dummy : public DecisionHeuristic {
			Dummy(Literal first, Literal second) {lit_[0] = first; lit_[1] = second;}
			Literal doSelect(Solver& s) {
				for (uint32 i = 0; i < 2; ++i) {
					if (s.value(lit_[i].var()) == value_free) {
						return lit_[i];
					}
				}
				return Literal();
			}
			Literal lit_[2];
		}*h = new Dummy(negLit(c),negLit(a));
		s.strategies().heuristic.reset(h);
		s.startAddConstraints();
		s.addBinary(posLit(a), posLit(b), false);
		s.addBinary(negLit(b), posLit(c), false);
		s.addBinary(negLit(a), posLit(c), false);
		s.endAddConstraints();
		s.assume( posLit(d) );
		s.setRootLevel(1);
		CPPUNIT_ASSERT_EQUAL(value_true, s.search(-1, -1, 0));
		s.clearAssumptions();
		CPPUNIT_ASSERT_EQUAL(0u, s.decisionLevel());
		CPPUNIT_ASSERT(s.isTrue(posLit(c)));
	}

	void testSearchMaxConflicts() {
		ClauseCreator cl(&s);
		Var a = s.addVar( Var_t::atom_var );
		Var b = s.addVar( Var_t::atom_var );
		Var c = s.addVar( Var_t::atom_var );
		s.startAddConstraints();
		s.addBinary(posLit(a), negLit(b), false);
		s.addBinary(negLit(a), posLit(b), false);
		s.addBinary(negLit(a), negLit(b), false);
		s.addBinary(posLit(a), posLit(b), false);
		s.endAddConstraints();
		s.simplify();
		s.assume(posLit(c));
		s.setRootLevel(1);
		CPPUNIT_ASSERT_EQUAL(value_free, s.search(1, -1, 0));
		CPPUNIT_ASSERT_EQUAL(1u, s.decisionLevel());
	} 

	void testClearAssumptions() {
		Var a = s.addVar( Var_t::atom_var );
		Var b = s.addVar( Var_t::atom_var );
		Var c = s.addVar( Var_t::atom_var );
		s.startAddConstraints();
		s.addBinary(negLit(a), negLit(b), false);
		s.addBinary(negLit(a), posLit(b), false);
		s.endAddConstraints();
		s.assume(posLit(a));
		s.setRootLevel(1);
		CPPUNIT_ASSERT_EQUAL(false, s.propagate());
		CPPUNIT_ASSERT_EQUAL(true, s.clearAssumptions());		
		CPPUNIT_ASSERT_EQUAL(0u, s.decisionLevel());		
		
		s.force(posLit(a), 0);
		CPPUNIT_ASSERT_EQUAL(false, s.propagate());
		CPPUNIT_ASSERT_EQUAL(false, s.clearAssumptions());		
	}

	void testStats() {
		ProblemStats p1, p2;
		CPPUNIT_ASSERT_EQUAL(uint32(0), p1.vars);
		CPPUNIT_ASSERT_EQUAL(uint32(0), p2.eliminated);
		CPPUNIT_ASSERT_EQUAL(uint32(0), p1.constraints[0]);
		CPPUNIT_ASSERT_EQUAL(uint32(0), p2.constraints[1]);
		CPPUNIT_ASSERT_EQUAL(uint32(0), p2.constraints[2]);

		p1.vars          = 100; p2.vars          = 150;
		p1.eliminated    = 20;  p2.eliminated    = 30;
		p1.constraints[0]= 150; p2.constraints[0]= 150;
		p1.constraints[1]=   0; p2.constraints[1]= 100;
		p1.constraints[2]= 100; p2.constraints[2]=   0;
		p1.diff(p2);

		CPPUNIT_ASSERT_EQUAL(uint32(50), p1.vars);
		CPPUNIT_ASSERT_EQUAL(uint32(10), p1.eliminated);
		CPPUNIT_ASSERT_EQUAL(uint32(0),  p1.constraints[0]);
		CPPUNIT_ASSERT_EQUAL(uint32(100),p1.constraints[1]);
		CPPUNIT_ASSERT_EQUAL(uint32(100),p1.constraints[2]);

		SolveStats st, st2;
		st.models     = 10; st2.models    = 2;
		st.conflicts  = 12; st2.conflicts = 3;
		st.choices    = 100;st2.choices   = 99;	
		st.restarts   = 7;  st2.restarts  = 8;
		
		st.learnts[0] = 6;  st2.learnts[0] = 4;
		st.learnts[1] = 5;  st2.learnts[1] = 4;
		st.lits[0]    = 15; st2.lits[0]    = 14;
		st.lits[1]    = 5;  st2.lits[1]    = 4;
		st.binary     = 6;  st2.ternary    = 5;
		st.deleted    = 10;
		
		st.accu(st2);

		CPPUNIT_ASSERT_EQUAL(uint64(12), st.models);
		CPPUNIT_ASSERT_EQUAL(uint64(15), st.conflicts);
		CPPUNIT_ASSERT_EQUAL(uint64(199),st.choices);
		CPPUNIT_ASSERT_EQUAL(uint64(15),st.restarts);
		CPPUNIT_ASSERT_EQUAL(uint64(29),st.lits[0]);
		CPPUNIT_ASSERT_EQUAL(uint64(9),st.lits[1]);
		CPPUNIT_ASSERT_EQUAL(uint64(10),st.learnts[0]);
		CPPUNIT_ASSERT_EQUAL(uint64(9),st.learnts[1]);
		CPPUNIT_ASSERT_EQUAL(uint64(6),st.binary);
		CPPUNIT_ASSERT_EQUAL(uint64(5),st.ternary);
		CPPUNIT_ASSERT_EQUAL(uint64(10),st.deleted);
	}

	void testIncrementalSolve() {
		ClaspConfig config;
		config.solver = &s;
		struct IncrementalConfig : public IncrementalControl {
			IncrementalConfig() : maxSteps(static_cast<uint32>(-1)), minSteps(1), stopUnsat(false)  {}
			uint32  maxSteps;
			uint32  minSteps; 
			bool    stopUnsat; 
			void initStep(ClaspFacade&) {}
			bool nextStep(ClaspFacade& f) {
				ClaspFacade::Result stopRes = stopUnsat ? ClaspFacade::result_unsat : ClaspFacade::result_sat;
				return --maxSteps && ((minSteps > 0 && --minSteps) || f.result() != stopRes);
			}
		} inc;
		struct IncInput : Input {
			IncInput() : step(0) {}
			Format format()      const { return Input::SMODELS; }
			MinimizeConstraint* getMinimize(Solver&, ProgramBuilder*, bool) { return 0; }
			void   getAssumptions(LitVec& a) {
				if (assume[step-1] != 0) {
					a.push_back( ~index->find(assume[step-1])->lit );
				}
			}
			bool   read(Solver& s, ProgramBuilder* api, int) {
				if (step == 0) {
					api->setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "x")
					    .startRule().addHead(1).addToBody(2, true).endRule()
						  .startRule().addHead(2).addToBody(1, true).endRule()
							.startRule().addHead(1).addToBody(3, true).endRule()
							.freeze(3)
							.setCompute(1, true);
					assume[step] = 3;
				}
				else if (step == 1) {
					api->setAtomName(4, "y").setAtomName(5, "z").endRule()
					    .startRule().addHead(3).addToBody(4, true).endRule()
						  .startRule().addHead(4).addToBody(3, true).endRule()
							.startRule().addHead(4).addToBody(5, true).endRule()
							.freeze(5)
							.unfreeze(assume[step-1]);
					assume[step] = 5;
				}
				else if (step == 2) {
					api->setAtomName(6, "q").setAtomName(7, "r")
					    .startRule().addHead(5).addToBody(6, true).addToBody(7, true).endRule()
						  .startRule().addHead(6).addToBody(3, false).endRule()
							.startRule().addHead(7).addToBody(1, false).addToBody(2, false).endRule()
							.startRule(CHOICERULE).addHead(5).endRule()
							.unfreeze(assume[step-1]);
					assume[step] = 0;
				}
				else if (step == 3){
					api->setAtomName(8,"f").startRule(CHOICERULE).addHead(8).endRule();
					assume[step] = 0;
				}
				else { return false; }
				++step;
				index = s.strategies().symTab.get();
				return true;
			}
			AtomIndex*      index;
			uint32          step;
			Var             assume[4];
		} in;
		
		ClaspFacade f;
		config.api.eq	= 0;
		f.solveIncremental(in, config, inc, 0);
		CPPUNIT_ASSERT_EQUAL(ClaspFacade::result_sat, f.result());
		CPPUNIT_ASSERT_EQUAL(2, f.step());

		inc.stopUnsat = true;
		in.step       = 0;
		s.reset();
		f.solveIncremental(in, config, inc, 0);
		CPPUNIT_ASSERT_EQUAL(ClaspFacade::result_unsat, f.result());
		CPPUNIT_ASSERT_EQUAL(0, f.step());

		inc.stopUnsat = false;
		inc.maxSteps  = 2;
		s.reset();
		in.step       = 0;
		f.solveIncremental(in, config, inc, 0);
		CPPUNIT_ASSERT_EQUAL(ClaspFacade::result_unsat, f.result());
		CPPUNIT_ASSERT_EQUAL(1, f.step());

		inc.maxSteps  = uint32(-1);
		inc.minSteps  = 4;
		in.step       = 0;
		s.reset();
		f.solveIncremental(in, config, inc, 0);
		CPPUNIT_ASSERT_EQUAL(ClaspFacade::result_sat, f.result());
		CPPUNIT_ASSERT_EQUAL(3, f.step());
	}
private:
	Solver s;
	LitVec addBinary() {
		LitVec r;
		r.push_back( posLit(s.addVar(Var_t::atom_var)) );
		r.push_back( posLit(s.addVar(Var_t::atom_var)) );
		s.startAddConstraints();
		s.addBinary(r[0], r[1], false);
		s.endAddConstraints();
		return r;
	}
	LitVec addTernary() {
		LitVec r;
		r.push_back( posLit(s.addVar(Var_t::atom_var)) );
		r.push_back( posLit(s.addVar(Var_t::atom_var)) );
		r.push_back( posLit(s.addVar(Var_t::atom_var)) );
		s.startAddConstraints();
		s.addTernary(r[0], r[1],r[2], false);
		s.endAddConstraints();
		return r;
	}
};
CPPUNIT_TEST_SUITE_REGISTRATION(SolverTest);
} } 
