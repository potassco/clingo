'''
Tests for solving.
'''
from unittest import TestCase
from typing import cast
from clingo import Control, Function, Model, ModelType, SolveHandle, SolveResult, SymbolicAtom

from . util import _MCB, _check_sat, _p

class TestSolving(TestCase):
    '''
    Tests basic solving and related functions.
    '''

    def setUp(self):
        self.mcb = _MCB()
        self.mit = _MCB()
        self.ctl = Control(['0'])

    def tearDown(self):
        self.mcb = None
        self.mit = None
        self.ctl = None

    def test_solve_cb(self):
        '''
        Test solving using callback.
        '''
        self.ctl.add("base", [], "1 {a; b} 1. c.")
        self.ctl.ground([("base", [])])
        _check_sat(self, cast(SolveResult, self.ctl.solve(on_model=self.mcb.on_model, yield_=False, async_=False)))
        self.assertEqual(self.mcb.models, _p(['a', 'c'], ['b', 'c']))
        self.assertEqual(self.mcb.last[0], ModelType.StableModel)

    def test_solve_async(self):
        '''
        Test asynchonous solving.
        '''
        self.ctl.add("base", [], "1 {a; b} 1. c.")
        self.ctl.ground([("base", [])])
        with cast(SolveHandle, self.ctl.solve(on_model=self.mcb.on_model, yield_=False, async_=True)) as hnd:
            _check_sat(self, hnd.get())
            self.assertEqual(self.mcb.models, _p(['a', 'c'], ['b', 'c']))

    def test_solve_yield(self):
        '''
        Test solving yielding models.
        '''
        self.ctl.add("base", [], "1 {a; b} 1. c.")
        self.ctl.ground([("base", [])])
        with cast(SolveHandle, self.ctl.solve(on_model=self.mcb.on_model, yield_=True, async_=False)) as hnd:
            for m in hnd:
                self.mit.on_model(m)
            _check_sat(self, hnd.get())
            self.assertEqual(self.mcb.models, _p(['a', 'c'], ['b', 'c']))
            self.assertEqual(self.mit.models, _p(['a', 'c'], ['b', 'c']))

    def test_solve_async_yield(self):
        '''
        Test solving yielding models asynchronously.
        '''
        self.ctl.add("base", [], "1 {a; b} 1. c.")
        self.ctl.ground([("base", [])])
        with self.ctl.solve(on_model=self.mcb.on_model, yield_=True, async_=True) as hnd:
            while True:
                hnd.resume()
                _ = hnd.wait()
                m = hnd.model()
                if m is None:
                    break
                self.mit.on_model(m)
            _check_sat(self, hnd.get())
            self.assertEqual(self.mcb.models, _p(['a', 'c'], ['b', 'c']))
            self.assertEqual(self.mit.models, _p(['a', 'c'], ['b', 'c']))

    def test_solve_interrupt(self):
        '''
        Test interrupting solving.
        '''
        self.ctl.add("base", [], "1 { p(P,H): H=1..99 } 1 :- P=1..100.\n1 { p(P,H): P=1..100 } 1 :- H=1..99.")
        self.ctl.ground([("base", [])])
        with self.ctl.solve(async_=True) as hnd:
            hnd.resume()
            hnd.cancel()
            ret = hnd.get()
            self.assertTrue(ret.interrupted)

        with self.ctl.solve(async_=True) as hnd:
            hnd.resume()
            self.ctl.interrupt()
            ret = hnd.get()
            self.assertTrue(ret.interrupted)

    def test_solve_core(self):
        '''
        Test core retrieval.
        '''
        self.ctl.add("base", [], "3 { p(1..10) } 3.")
        self.ctl.ground([("base", [])])
        ass = []
        for atom in self.ctl.symbolic_atoms.by_signature("p", 1):
            ass.append(-atom.literal)
        ret = cast(SolveResult, self.ctl.solve(on_core=self.mcb.on_core, assumptions=ass))
        self.assertTrue(ret.unsatisfiable)
        self.assertTrue(len(self.mcb.core) > 7)

    def test_enum(self):
        '''
        Test core retrieval.
        '''
        self.ctl = Control(['0', '-e', 'cautious'])
        self.ctl.add("base", [], "1 {a; b} 1. c.")
        self.ctl.ground([("base", [])])
        self.ctl.solve(on_model=self.mcb.on_model)
        self.assertEqual(self.mcb.last[0], ModelType.CautiousConsequences)
        self.assertEqual([self.mcb.last[1]], _p(['c']))

        self.ctl = Control(['0', '-e', 'brave'])
        self.ctl.add("base", [], "1 {a; b} 1. c.")
        self.ctl.ground([("base", [])])
        self.ctl.solve(on_model=self.mcb.on_model)
        self.assertEqual(self.mcb.last[0], ModelType.BraveConsequences)
        self.assertEqual([self.mcb.last[1]], _p(['a', 'b', 'c']))

    def test_model(self):
        '''
        Test functions of model.
        '''
        def on_model(m: Model):
            self.assertTrue(m.contains(Function('a')))
            self.assertTrue(m.is_true(cast(SymbolicAtom, m.context.symbolic_atoms[Function('a')]).literal))
            self.assertFalse(m.is_true(1000))
            self.assertEqual(m.thread_id, 0)
            self.assertEqual(m.number, 1)
            self.assertFalse(m.optimality_proven)
            self.assertEqual(m.cost, [3])
            m.extend([Function('e')])
            self.assertEqual(m.symbols(theory=True), [Function('e')])
        self.ctl.add("base", [], "a. b. c. #minimize { 1,a:a; 1,b:b; 1,c:c }.")
        self.ctl.ground([("base", [])])
        self.ctl.solve(on_model=on_model)

    def test_control_clause(self):
        '''
        Test adding clauses while solving.
        '''
        self.ctl.add("base", [], "1 {a; b; c} 1.")
        self.ctl.ground([("base", [])])
        with cast(SolveHandle, self.ctl.solve(on_model=self.mcb.on_model, yield_=True, async_=False)) as hnd:
            for m in hnd:
                clause = []
                if m.contains(Function('a')):
                    clause.append((Function('b'), False))
                else:
                    clause.append((Function('a'), False))
                m.context.add_clause(clause)

            _check_sat(self, hnd.get())
            self.assertEqual(len(self.mcb.models), 2)

    def test_control_nogood(self):
        '''
        Test adding nogoods while solving.
        '''
        self.ctl.add("base", [], "1 {a; b; c} 1.")
        self.ctl.ground([("base", [])])
        with cast(SolveHandle, self.ctl.solve(on_model=self.mcb.on_model, yield_=True, async_=False)) as hnd:
            for m in hnd:
                clause = []
                if m.contains(Function('a')):
                    clause.append((Function('b'), True))
                else:
                    clause.append((Function('a'), True))
                m.context.add_nogood(clause)

            _check_sat(self, hnd.get())
            self.assertEqual(len(self.mcb.models), 2)
