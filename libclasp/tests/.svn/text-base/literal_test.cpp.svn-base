#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <clasp/solver_types.h>
#include "common.h"

namespace Clasp { namespace Test {

class LiteralTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LiteralTest);
	CPPUNIT_TEST(testCtor);
	CPPUNIT_TEST(testIndex);
	CPPUNIT_TEST(testIndexIgnoresWatchedFlag);
	CPPUNIT_TEST(testFromIndex);
	CPPUNIT_TEST(testWatchedFlag);
	CPPUNIT_TEST(testWatchedFlagCopy);
	CPPUNIT_TEST(testComplement);
	CPPUNIT_TEST(testComplementIsNotWatched);
	CPPUNIT_TEST(testEquality);
	CPPUNIT_TEST(testValue);
	CPPUNIT_TEST(testLess);

	CPPUNIT_TEST(testAntecedentAssumptions);
	CPPUNIT_TEST(testAntecedentNullPointer);
	CPPUNIT_TEST(testAntecedentPointer);
	CPPUNIT_TEST(testAntecedentBin);
	CPPUNIT_TEST(testAntecedentTern);
	CPPUNIT_TEST_SUITE_END();
public:
	LiteralTest() {
		min = posLit(0), mid = posLit(varMax / 2),  max = posLit(varMax - 1);
	}
	void testCtor() {
		Literal p, q(42, true);
		CPPUNIT_ASSERT_EQUAL(Var(0), p.var());
		CPPUNIT_ASSERT_EQUAL(false, p.sign());
		CPPUNIT_ASSERT_EQUAL(false, p.watched());

		CPPUNIT_ASSERT_EQUAL(Var(42), q.var());
		CPPUNIT_ASSERT_EQUAL(true, q.sign());
		CPPUNIT_ASSERT_EQUAL(false, q.watched());

		Literal x = posLit(7);
		Literal y = negLit(7);
		CPPUNIT_ASSERT(x.var() == y.var() && y.var() == Var(7));
		CPPUNIT_ASSERT_EQUAL(false, x.sign());
		CPPUNIT_ASSERT_EQUAL(true, y.sign());
	}
	void testIndex() {
		uint32 minIdx	= min.index();
		uint32 maxIdx	= max.index();
		uint32 midIdx	= mid.index();

		CPPUNIT_ASSERT_EQUAL(uint32(0), minIdx);
		CPPUNIT_ASSERT_EQUAL(uint32(1), (~min).index());
		
		CPPUNIT_ASSERT_EQUAL(uint32((max.var()*2)), maxIdx);
		CPPUNIT_ASSERT_EQUAL(uint32((max.var()*2)+1), (~max).index());
		
		CPPUNIT_ASSERT_EQUAL(uint32((mid.var()*2)), midIdx);
		CPPUNIT_ASSERT_EQUAL(uint32((mid.var()*2)+1), (~mid).index());

		CPPUNIT_ASSERT_EQUAL( minIdx, index(min.var()) );
		CPPUNIT_ASSERT_EQUAL( maxIdx, index(max.var()) );
		CPPUNIT_ASSERT_EQUAL( midIdx, index(mid.var()) );
	}
	void testIndexIgnoresWatchedFlag() {
		Literal maxW = max;
		maxW.watch();
		CPPUNIT_ASSERT_EQUAL(max.index(), maxW.index());
	}
	void testFromIndex() {
		CPPUNIT_ASSERT(min == Literal::fromIndex(min.index()));
		CPPUNIT_ASSERT(max == Literal::fromIndex(max.index()));
		CPPUNIT_ASSERT(mid == Literal::fromIndex(mid.index()));
	}
	void testWatchedFlag() {
		Literal p = posLit(42);
		p.watch();
		CPPUNIT_ASSERT_EQUAL(true, p.watched());
		p.clearWatch();
		CPPUNIT_ASSERT_EQUAL(false, p.watched());
	}
	void testWatchedFlagCopy() {
		Literal p = posLit(42);
		p.watch();
		Literal q = p;
		CPPUNIT_ASSERT_EQUAL(true, q.watched());
	}
	void testComplement() {
		Literal lit = posLit(7);
		Literal cLit = ~lit;
		CPPUNIT_ASSERT_EQUAL(lit.var(), cLit.var());
		CPPUNIT_ASSERT_EQUAL(false, lit.sign());
		CPPUNIT_ASSERT_EQUAL(true, cLit.sign());
		CPPUNIT_ASSERT_EQUAL(true, lit == ~~lit);
	}
	void testComplementIsNotWatched() {
		Literal lit = posLit(7);
		lit.watch();
		Literal cLit = ~lit;
		CPPUNIT_ASSERT_EQUAL(false, cLit.watched());
	}

	void testEquality() {
		Literal p = posLit(42);
		Literal q = negLit(42);
		CPPUNIT_ASSERT_EQUAL(p, p);
		CPPUNIT_ASSERT_EQUAL(p, ~q);
		CPPUNIT_ASSERT_EQUAL(false, p == q);
		CPPUNIT_ASSERT_EQUAL(Literal(), Literal());
	}

	void testValue() {
		CPPUNIT_ASSERT_EQUAL(value_true, trueValue(posLit(0)));
		CPPUNIT_ASSERT_EQUAL(value_false, trueValue(negLit(0)));
		CPPUNIT_ASSERT_EQUAL(value_false, falseValue(posLit(0)));
		CPPUNIT_ASSERT_EQUAL(value_true, falseValue(negLit(0)));
	}

	void testLess() {
		Literal p = posLit(42), q = negLit(42);
		CPPUNIT_ASSERT_EQUAL(false, p < p);
		CPPUNIT_ASSERT_EQUAL(false, q < q);
		CPPUNIT_ASSERT_EQUAL(true, p < q);
		CPPUNIT_ASSERT_EQUAL(false, q < p);

		Literal one(1, false);
		Literal two(2, true);
		Literal negOne = ~one;
		CPPUNIT_ASSERT_EQUAL(true, one < two);
		CPPUNIT_ASSERT_EQUAL(true, negOne < two);
		CPPUNIT_ASSERT_EQUAL(false, two < negOne);
	}
	void testAntecedentAssumptions() {
		Antecedent::checkPlatformAssumptions();
	}
	
	void testAntecedentNullPointer() {
		Antecedent a;
		Antecedent b( (Constraint*) 0 );
		CPPUNIT_ASSERT_EQUAL(true, a.isNull());
		CPPUNIT_ASSERT_EQUAL(true, b.isNull());
	}

	void testAntecedentPointer() {
		struct Con : Constraint {
			PropResult propagate(const Literal&, uint32&, Solver&) { return PropResult(true, true); }
			void reason(const Literal&, LitVec&) {}
			ConstraintType type() const { return Constraint_t::native_constraint; }
		};
		
		Constraint* c = new Con;
		Antecedent a(c);
		CPPUNIT_ASSERT_EQUAL(false, a.isNull());
		CPPUNIT_ASSERT_EQUAL(Antecedent::generic_constraint, a.type());
		CPPUNIT_ASSERT_EQUAL(c, a.constraint());
		c->destroy();
	}

	void testAntecedentBin() {
		CPPUNIT_ASSERT_EQUAL(true, testBin(max));
		CPPUNIT_ASSERT_EQUAL(true, testBin(min));
		CPPUNIT_ASSERT_EQUAL(true, testBin(mid));
	}

	void testAntecedentTern() {
		CPPUNIT_ASSERT_EQUAL(true, testTern(max, max));
		CPPUNIT_ASSERT_EQUAL(true, testTern(max, mid));
		CPPUNIT_ASSERT_EQUAL(true, testTern(max, min));
		CPPUNIT_ASSERT_EQUAL(true, testTern(mid, max));
		CPPUNIT_ASSERT_EQUAL(true, testTern(mid, mid));
		CPPUNIT_ASSERT_EQUAL(true, testTern(mid, min));
		CPPUNIT_ASSERT_EQUAL(true, testTern(min, max));
		CPPUNIT_ASSERT_EQUAL(true, testTern(min, mid));
		CPPUNIT_ASSERT_EQUAL(true, testTern(min, min));
	}
private:
	Literal min, mid, max;
	bool testBin(const Literal& p) {
		Antecedent ap(p);
		Antecedent aNotp(~p);
		CPPUNIT_ASSERT_EQUAL(false, ap.isNull());
		CPPUNIT_ASSERT_EQUAL(Antecedent::binary_constraint, ap.type());
		CPPUNIT_ASSERT(p == ap.firstLiteral());

		CPPUNIT_ASSERT_EQUAL(false, aNotp.isNull());
		CPPUNIT_ASSERT_EQUAL(Antecedent::binary_constraint, aNotp.type());
		CPPUNIT_ASSERT(~p == aNotp.firstLiteral());
		return true;
	}
	bool testTern(const Literal& p, const Literal& q) {
		Antecedent app(p, q);
		Antecedent apn(p, ~q);
		Antecedent anp(~p, q);
		Antecedent ann(~p, ~q);

		CPPUNIT_ASSERT_EQUAL(false, app.isNull());
		CPPUNIT_ASSERT_EQUAL(Antecedent::ternary_constraint, app.type());
		CPPUNIT_ASSERT_EQUAL(false, apn.isNull());
		CPPUNIT_ASSERT_EQUAL(Antecedent::ternary_constraint, apn.type());
		CPPUNIT_ASSERT_EQUAL(false, anp.isNull());
		CPPUNIT_ASSERT_EQUAL(Antecedent::ternary_constraint, anp.type());
		CPPUNIT_ASSERT_EQUAL(false, ann.isNull());
		CPPUNIT_ASSERT_EQUAL(Antecedent::ternary_constraint, ann.type());
		
		CPPUNIT_ASSERT(p == app.firstLiteral() && q == app.secondLiteral());
		CPPUNIT_ASSERT(p == apn.firstLiteral() && ~q == apn.secondLiteral());
		CPPUNIT_ASSERT(~p == anp.firstLiteral() && q == anp.secondLiteral());
		CPPUNIT_ASSERT(~p == ann.firstLiteral() && ~q == ann.secondLiteral());
		return true;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(LiteralTest);
} } 
