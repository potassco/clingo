#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <clasp/program_builder.h>
#include <clasp/solver.h>
#include <utility>
namespace Clasp { namespace Test {

class RuleTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(RuleTest);
	CPPUNIT_TEST(testHashIgnoresOrder);
	CPPUNIT_TEST(testRemoveDuplicateInNormal);
	CPPUNIT_TEST(testMergeDuplicateInExtended);
	CPPUNIT_TEST(testContraNormal);
	CPPUNIT_TEST(testNoContraExtended);
	CPPUNIT_TEST(testContraExtended);
	CPPUNIT_TEST(testMultiSimplify);
	CPPUNIT_TEST(testCardinalityIfAllWeightsEqual);
	CPPUNIT_TEST(testNormalIfMinWeightNeeded);
	CPPUNIT_TEST(testSelfblockNormal);
	CPPUNIT_TEST(testTautNormal);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
		rs.clear();
	}

	void testHashIgnoresOrder() {
		PrgRule r1, r2, r3;
		RuleState rs1, rs2, rs3;
		PrgRule::RData rd1, rd2, rd3;
		r1.setType(BASICRULE).addHead(1).addToBody(10, false).addToBody(20, true).addToBody(25, true);
		r2.setType(BASICRULE).addHead(1).addToBody(20, true).addToBody(25, true).addToBody(10, false);
		r3.setType(BASICRULE).addHead(1).addToBody(25, true).addToBody(10, false).addToBody(20, true);
		rd1 = r1.simplify(rs1);
		rd2 = r2.simplify(rs2);
		rd3 = r3.simplify(rs3);
		CPPUNIT_ASSERT(rd1.hash == rd2.hash && rd2.hash == rd3.hash);
	}

	void testRemoveDuplicateInNormal() {
		// a :- b, b, not c -> a :- b, not c.
		rule.setType(BASICRULE).addHead(1).addToBody(2, true).addToBody(2, true).addToBody(3, false);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(2 == rule.body.size());
		CPPUNIT_ASSERT(1 == res.posSize);
	}

	void testMergeDuplicateInExtended() {
		// a :- 2 {b, not c, b} -> a :- 2 [b=2, not c].
		rule.setType(CONSTRAINTRULE).setBound(2).addHead(1).addToBody(2, true).addToBody(3, false).addToBody(2, true);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(2 == rule.body.size());
		CPPUNIT_ASSERT(1 == res.posSize);
		CPPUNIT_ASSERT(rule.body[0].second == 2);
		CPPUNIT_ASSERT(rule.type() == WEIGHTRULE);
		CPPUNIT_ASSERT(res.sumWeight == 3);

		
		rule.clear(), rs.clear();
		// {b, not c, b} -> [b=2, not c].
		rule.setType(OPTIMIZERULE).addToBody(2, true).addToBody(3, false).addToBody(2, true);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(2 == rule.body.size());
		CPPUNIT_ASSERT(1 == res.posSize);
		CPPUNIT_ASSERT(rule.body[0].second == 2);
		CPPUNIT_ASSERT(rule.type() == OPTIMIZERULE);
		CPPUNIT_ASSERT(res.sumWeight == 3);
	}

	void testContraNormal() {
		// a :- b, c, not b.
		rule.setType(BASICRULE).addHead(1).addToBody(2, true).addToBody(3, true).addToBody(2, false);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == ENDRULE);
		CPPUNIT_ASSERT(res.value == value_false);
	}

	void testNoContraExtended() {
		// a :- 2 {b, c, not b}.
		rule.setType(CONSTRAINTRULE).setBound(2).addHead(1).addToBody(2, true).addToBody(3, true).addToBody(2, false);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == CONSTRAINTRULE);
		CPPUNIT_ASSERT(res.value == value_free);

		// a :- 4 {not b, b, b, c, d}.
		rule.clear(), rs.clear();
		rule.setType(CONSTRAINTRULE).setBound(4).addHead(1).addToBody(2, false).addToBody(2, true).addToBody(2, true).addToBody(3, true).addToBody(4, true);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == WEIGHTRULE);
		CPPUNIT_ASSERT(res.value == value_free);

		rule.clear(), rs.clear();
		// a :- 4 [b=2, c=1, not b=1, not c=2].
		rule.setType(WEIGHTRULE).setBound(4).addHead(1).addToBody(2, true, 2).addToBody(3, true).addToBody(2, false).addToBody(3, false, 2);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == WEIGHTRULE);
		CPPUNIT_ASSERT(res.value == value_free);

		rule.clear(), rs.clear();
		rule.setType(OPTIMIZERULE).addToBody(1, true).addToBody(1, false);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == OPTIMIZERULE);
		CPPUNIT_ASSERT(res.value == value_free);
	}

	void testContraExtended() {
		// a :- 3 {b, c, not b, not c}.
		rule.setType(CONSTRAINTRULE).setBound(3).addHead(1).addToBody(2, true).addToBody(3, true).addToBody(2, false).addToBody(3, false);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == ENDRULE);
		CPPUNIT_ASSERT(res.value == value_false);

		rule.clear(), rs.clear();
		// a :- 4 [b=2, c=1, not b=1, not c=1].
		rule.setType(WEIGHTRULE).setBound(4).addHead(1).addToBody(2, true, 2).addToBody(3, true).addToBody(2, false).addToBody(3, false);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == ENDRULE);
		CPPUNIT_ASSERT(res.value == value_false);
	}

	void testMultiSimplify() {
		// a :- 1 [b=0,c=0,d=2,e=0] -> a :- d.
		//  - remove 0 weights: 1 [d=2]
		//  - bound weights   : 1 [d=1]
		//  - flatten         : d.
		rule.setType(WEIGHTRULE).addHead(1).setBound(1).addToBody(2, true).addToBody(3, true).addToBody(4, true, 2).addToBody(5, true);
		rule.body[0].second = 0;
		rule.body[1].second = 0;
		rule.body[3].second = 0;
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == BASICRULE);
		CPPUNIT_ASSERT(res.posSize == 1);
	}

	void testCardinalityIfAllWeightsEqual() {
		// a :- 3 [b=2,c=2, d=2,e=0] -> 
		rule.setType(WEIGHTRULE).addHead(1).setBound(3).addToBody(2, true, 2).addToBody(3, true, 2).addToBody(4, true, 2).addToBody(5, true);
		rule.body[3].second = 0;
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type()  == CONSTRAINTRULE);
		CPPUNIT_ASSERT(rule.bound() == 2);

		rule.clear(), rs.clear();
		// a :- 2 [b=1,c=2 b=1] -> 1 {b,c}
		rule.setType(WEIGHTRULE).addHead(1).setBound(2).addToBody(2, true, 1).addToBody(3, true, 2).addToBody(2, true, 1);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type()  == CONSTRAINTRULE);
		CPPUNIT_ASSERT(rule.bound() == 1);

	}
	
	void testNormalIfMinWeightNeeded() {
		// a :- 8 [b=4,c=3, d=2,e=0] -> 
		rule.setType(WEIGHTRULE).addHead(1).setBound(8).addToBody(2, true, 4).addToBody(3, true, 3).addToBody(4, true, 2).addToBody(5, true);
		rule.body[3].second = 0;
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type()  == BASICRULE);
	}

	void testSelfblockNormal() {
		// a :- not a.
		rule.setType(BASICRULE);
		rule.addHead(1).addToBody(1, false);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(value_false == res.value);
	}	

	void testTautNormal() {
		// a :- a, b.
		rule.setType(BASICRULE).addHead(1).addToBody(1, true).addToBody(2, true);
		res = rule.simplify(rs);
		CPPUNIT_ASSERT(rule.type() == ENDRULE);
	}	

private:
	Solver					solver;
	RuleState       rs;
	PrgRule					rule;
	PrgRule::RData  res;
};


class RuleTransformTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(RuleTransformTest);
	CPPUNIT_TEST(testChoiceRuleEmpty);
	CPPUNIT_TEST(testChoiceRuleOneHead);
	CPPUNIT_TEST(testChoiceRuleUseExtraHead);
	
	CPPUNIT_TEST(testTrivialConstraintRule);
	CPPUNIT_TEST(testUnsatConstraintRule);
	CPPUNIT_TEST(testDegeneratedConstraintRule);
	CPPUNIT_TEST(testBoundEqOneExp);
	CPPUNIT_TEST(testBoundEqOneQuad);

	CPPUNIT_TEST(testSixThreeExp);
	CPPUNIT_TEST(testSixThreeQuad);
	CPPUNIT_TEST(testWeightSixFourExp);
	CPPUNIT_TEST(testWeightSixFourQuad);
	CPPUNIT_TEST(testWeightBug);

	CPPUNIT_TEST(testStupidWeightBug);
	CPPUNIT_TEST(testWeightBogusNormal);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
		prg.startProgram(index, 0, false);
		prg.setAtomName(1, "a").setAtomName(2, "b").setAtomName(3, "c").setAtomName(4, "d")
			 .setAtomName(5, "e").setAtomName(6, "f").setAtomName(7, "g");
	}
	void testChoiceRuleEmpty() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_choice);
		rule.setType(CHOICERULE);
		prg.addRule(rule);
		CPPUNIT_ASSERT_EQUAL(0u, prg.stats.rules[CHOICERULE]);
	}
	void testChoiceRuleOneHead() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_choice);
		
		rule.setType(CHOICERULE);
		rule.addHead(1);
		prg.addRule(rule);
		
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp << "1 1 1 1 8 \n"
			  << "1 8 1 1 1 \n";
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}

	void testChoiceRuleUseExtraHead() {
		prg.startRule(CHOICERULE).addHead(4).addHead(5).addHead(6).addHead(7).endRule();
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_choice);
		rule.setType(CHOICERULE);
		// {a, b, c} :- d, e, not f, not g.
		rule.addHead(1).addHead(2).addHead(3)
			.addToBody(4, true).addToBody(5, true).addToBody(6, false).addToBody(7, false);

		prg.addRule(rule);
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp << "1 1 2 1 9 8 \n"		// a			:- auxBody, not auxA
			  << "1 9 1 1 1 \n"			// auxA		:- not a.
				<< "1 2 2 1 10 8 \n"	// b			:- auxBody, not auxB
				<< "1 10 1 1 2 \n"		// auxB		:- not b.
				<< "1 3 2 1 11 8 \n"	// c			:- auxBody, not auxC
				<< "1 11 1 1 3 \n"		// auxC		:- not c.
				<< "1 8 4 2 6 7 4 5 \n"; // auxB :- d, e, not f, not g
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}
	
	void testTrivialConstraintRule() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform);
		rule.setType(CONSTRAINTRULE);
		rule.addHead(1);
		rule.setBound(0);
		prg.addRule(rule);
		CPPUNIT_ASSERT_EQUAL(1u, prg.stats.rules[BASICRULE]);
	}
	
	void testUnsatConstraintRule() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform);
		rule.setType(CONSTRAINTRULE);
		rule.addHead(1);
		rule.addToBody(2, true);
		rule.setBound(2);
		prg.addRule(rule);
		CPPUNIT_ASSERT_EQUAL(0u, prg.stats.rules[0]);

		rule.clear();
		rule.setType(WEIGHTRULE);
		rule.addHead(1);
		rule.addToBody(2, true, 2);
		rule.setBound(3);
		prg.addRule(rule);
		CPPUNIT_ASSERT_EQUAL(0u, prg.stats.rules[0]);
	}
	
	void testDegeneratedConstraintRule() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform);
		
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).endRule();
		rule.setType(CONSTRAINTRULE);
		// a :- 3 { b, c, d }.
		rule.addHead(1).addToBody(2, true).addToBody(3, true).addToBody(4, true).setBound(3);
		prg.addRule(rule);
		CPPUNIT_ASSERT_EQUAL(1u, prg.stats.rules[BASICRULE]);
	}
	void testBoundEqOneExp() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).endRule();
		rule.setType(CONSTRAINTRULE);
		// a :- 1 { b, c, d }.
		rule.addHead(1).addToBody(2, true).addToBody(3, true).addToBody(4, true).setBound(1);
		prg.addRule(rule);
		CPPUNIT_ASSERT_EQUAL(3u, prg.stats.rules[BASICRULE]);
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp << "1 1 1 0 2 \n"
				<< "1 1 1 0 3 \n"
				<< "1 1 1 0 4 \n";
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}
	void testBoundEqOneQuad() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).endRule();
		rule.setType(CONSTRAINTRULE);
		// a :- 1 { b, c, d }.
		rule.addHead(1).addToBody(2, true).addToBody(3, true).addToBody(4, true).setBound(1);
		PrgRuleTransform tm;
		CPPUNIT_ASSERT_EQUAL(4u, tm.transform(prg, rule));
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp << "1 1 1 0 2 \n"
			  << "1 1 1 0 8 \n"
				<< "1 8 1 0 3 \n"
				<< "1 8 1 0 4 \n";
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}
	void testSixThreeExp() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).addHead(5).addHead(6).addHead(7).endRule();
		rule.setType(CONSTRAINTRULE);
		// a :- 3 {b, c, d, e, f, g}
		rule.addHead(1).addToBody(2, true).addToBody(3, true).addToBody(4, true).addToBody(5, true).addToBody(6, true).addToBody(7, true);
		rule.setBound(3);
		PrgRuleTransform tm;
		CPPUNIT_ASSERT_EQUAL(20u, tm.transformNoAux(prg, rule));
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp // starting with b
				<< "1 1 3 0 2 3 4 \n"	
				<< "1 1 3 0 2 3 5 \n"	
				<< "1 1 3 0 2 3 6 \n"	
				<< "1 1 3 0 2 3 7 \n"	
				<< "1 1 3 0 2 4 5 \n"	
				<< "1 1 3 0 2 4 6 \n"	
				<< "1 1 3 0 2 4 7 \n"
				<< "1 1 3 0 2 5 6 \n"	
				<< "1 1 3 0 2 5 7 \n"
				<< "1 1 3 0 2 6 7 \n"
				// starting with c
				<< "1 1 3 0 3 4 5 \n"
				<< "1 1 3 0 3 4 6 \n"
				<< "1 1 3 0 3 4 7 \n"
				<< "1 1 3 0 3 5 6 \n"
				<< "1 1 3 0 3 5 7 \n"
				<< "1 1 3 0 3 6 7 \n"
				// starting with d
				<< "1 1 3 0 4 5 6 \n"
				<< "1 1 3 0 4 5 7 \n"
				<< "1 1 3 0 4 6 7 \n"
				// starting with e
				<< "1 1 3 0 5 6 7 \n";
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}
 
	void testSixThreeQuad() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).addHead(5).addHead(6).addHead(7).endRule();
		rule.setType(CONSTRAINTRULE);
		// a :- 3 {b, c, d, e, f, g}
		rule.addHead(1).addToBody(2, true).addToBody(3, true).addToBody(4, true).addToBody(5, true).addToBody(6, true).addToBody(7, true);
		rule.setBound(3);
		PrgRuleTransform tm;
		CPPUNIT_ASSERT_EQUAL(18u, tm.transform(prg, rule));
		CPPUNIT_ASSERT_EQUAL(15u, prg.numAtoms());
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::string s = out.str();
		std::stringstream exp;
		exp << "1 1 2 0 2 8 \n"		// a			:- b, (c,2)
				<< "1 1 1 0 9 \n"			// a			:- (c,3)
				
				<< "1 8 2 0 3 10 \n"	// (c,2)	:- c, (d,1)
				<< "1 8 1 0 11 \n"		// (c,2)	:- (d,2)
				<< "1 9 2 0 3 11 \n"	// (c,3)	:- c, (d,2)
				<< "1 9 1 0 12 \n"		// (c,3)	:- (d,3)
				
				<< "1 10 1 0 4 \n"		// (d,1)	:- d.
				<< "1 10 1 0 13 \n"	  // (d,1)	:- (e,1)
				<< "1 11 2 0 4 13 \n" // (d,2)	:- d, (e,1).
				<< "1 11 1 0 14 \n"	  // (d,2)	:- (e,2).
				<< "1 12 2 0 4 14 \n" // (d,3)	:- d, (e,2).
				<< "1 12 3 0 5 6 7 \n"// (d,3)	:- (e,2).
				
				<< "1 13 1 0 5 \n"		// (e,1)	:- e.
				<< "1 13 1 0 15 \n"		// (e,1)	:- (f,1).
				<< "1 14 2 0 5 15 \n"	// (e,2)	:- e, (f,1).
				<< "1 14 2 0 6 7 \n"	// (e,2)	:- f,g.

				<< "1 15 1 0 6 \n"	  // (f,1)	:- f
				<< "1 15 1 0 7 \n";	  // (f,1)	:- g
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}

	void testWeightSixFourExp() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).addHead(5).addHead(6).addHead(7).endRule();
		rule.setType(WEIGHTRULE);
		// a :- 4 {b=4, c=3, d=2, e=2, f=1, g=1}
		rule.addHead(1).addToBody(2, true,4).addToBody(3, true,3).addToBody(4, true,2).addToBody(5, true,2).addToBody(6, true,1).addToBody(7, true,1);
		rule.setBound(4);
		PrgRuleTransform tm;
		CPPUNIT_ASSERT_EQUAL(8u, tm.transformNoAux(prg, rule));	
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp // starting with b
				<< "1 1 1 0 2 \n"
				<< "1 1 2 0 3 4 \n"
				<< "1 1 2 0 3 5 \n"
				<< "1 1 2 0 3 6 \n"
				<< "1 1 2 0 3 7 \n"
				<< "1 1 2 0 4 5 \n"
				<< "1 1 3 0 4 6 7 \n"
				<< "1 1 3 0 5 6 7 \n";
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}
	
	void testWeightSixFourQuad() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).addHead(5).addHead(6).addHead(7).endRule();
		rule.setType(WEIGHTRULE);
		// a :- 4 {b=4, c=3, d=2, e=2, f=1, g=1}
		rule.addHead(1).addToBody(2, true,4).addToBody(3, true,3).addToBody(4, true,2).addToBody(5, true,2).addToBody(6, true,1).addToBody(7, true,1);
		rule.setBound(4);
		PrgRuleTransform tm;
		CPPUNIT_ASSERT_EQUAL(14u, tm.transform(prg, rule));
		CPPUNIT_ASSERT(13u == prg.numAtoms());
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp 
			<< "1 1 1 0 2 \n"			// a			:- b.
			<< "1 1 1 0 8 \n"			// a			:- (c, 4)

			<< "1 8 2 0 3 9 \n"		// (c,4)	:- c, (d,1).
			<< "1 8 1 0 10 \n"		// (c,4)	:- (d, 4)
			
			<< "1 9 1 0 4 \n"		  // (d,1)	:- d.
			<< "1 9 1 0 11 \n"		// (d,1)	:- (e, 1)
			<< "1 10 2 0 4 12 \n" // (d,4)	:- d, (e,2)
			<< "1 10 3 0 5 6 7 \n"// (d,4)	:- e,f,g.
			
			<< "1 11 1 0 5 \n"		// (e,1)	:- e.
			<< "1 12 1 0 5 \n"		// (e,2)	:- e.
			<< "1 11 1 0 13 \n"		// (e,1)	:- (f,1).
			<< "1 12 2 0 6 7 \n"	// (e,2)	:- f,g.

			<< "1 13 1 0 6 \n"		// (f,1)	:- f.
			<< "1 13 1 0 7 \n";	  // (f,1)	:- g.
			
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
		
	}

	void testWeightBug() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(2).addHead(3).addHead(4).endRule();
		rule.setType(WEIGHTRULE);
		// a :- 5 {b=3, c=3, d=1}
		rule.addHead(1).addToBody(2, true,3).addToBody(3, true,3).addToBody(4, true,1);
		rule.setBound(5);
		PrgRuleTransform tm;
		CPPUNIT_ASSERT_EQUAL(2u, tm.transform(prg, rule));
		CPPUNIT_ASSERT(8u == prg.numAtoms());
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp 
			<< "1 1 2 0 2 8 \n"			// a			:- b, (c, 2)
			<< "1 8 1 0 3 \n";			// (c,2)	:- c.
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}

	void testStupidWeightBug() {
		prg.setAtomName(8, "h").setAtomName(9, "x");
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).addHead(4).addHead(5).addHead(6).addHead(7).addHead(8).endRule();
		rule.setType(WEIGHTRULE);
		// x :- 24 {a=31, b=29, c=29, d=28, e=21, f=15, g=8, h=5}
		rule.addHead(9).addToBody(1, true,31).addToBody(2, true,29).addToBody(3, true,29).addToBody(4, true,28).addToBody(5, true,21).addToBody(6, true,15)
		               .addToBody(7,true,8).addToBody(8,true,5);

		rule.setBound(24);
		PrgRuleTransform tm;
		uint32 prev = prg.numAtoms();
		CPPUNIT_ASSERT_EQUAL(14u, tm.transform(prg, rule));
		CPPUNIT_ASSERT(prg.numAtoms() == prev+6);
		prg.endProgram(solver, false);
		std::ostringstream out;
		prg.writeProgram(out);
		std::stringstream exp;
		exp  << "1 13 3 0 6 7 8 \n";
		CPPUNIT_ASSERT(out.str().find(exp.str()) != std::string::npos);
	}

	void testWeightBogusNormal() {
		prg.setExtendedRuleMode(ProgramBuilder::mode_transform_weight);
		prg.startRule(CHOICERULE).addHead(1).addHead(2).addHead(3).endRule();
		rule.setType(WEIGHTRULE);
		// a :- 24 {b=12,c=12}.
		rule.addHead(1).addToBody(2, true,12).addToBody(3, true,12);

		rule.setBound(24);
		PrgRuleTransform tm;
		uint32 prev = prg.numAtoms();
		CPPUNIT_ASSERT_EQUAL(1u, tm.transform(prg, rule));
		CPPUNIT_ASSERT(prg.numAtoms() == prev);
	}
private:
	Solver					solver;
	ProgramBuilder	prg;
	AtomIndex       index;
	PrgRule					rule;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RuleTest);
CPPUNIT_TEST_SUITE_REGISTRATION(RuleTransformTest);

} } 
