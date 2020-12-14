'''
Tests control.
'''
from unittest import TestCase
from clingo import Control, Function, Number

class TestError(Exception):
    '''
    Test exception.
    '''

class Context:
    '''
    Simple context with some test functions.
    '''
    def cb_num(self, c):
        '''
        Simple test callback.
        '''
        return [Number(c.number + 1), Number(c.number - 1)]

    def cb_error(self):
        '''
        Simple test raising an error.
        '''
        raise TestError('test')

class TestSymbol(TestCase):
    '''
    Tests basic solving and related functions.
    '''
    def test_ground(self):
        '''
        Test grounding with context and parameters.
        '''
        ctx = Context()
        ctl = Control()
        ctl.add('part', ['c'], 'p(@cb_num(c)).')
        ctl.ground([('part', [Number(1)])], ctx)
        symbols = [atom.symbol for atom in ctl.symbolic_atoms]
        self.assertEqual(sorted(symbols), [Function('p', [Number(0)]), Function('p', [Number(2)])])

    def test_ground_error(self):
        '''
        Test grounding with context and parameters.
        '''
        ctx = Context()
        ctl = Control()
        ctl.add('part', ['c'], 'p(@cb_error()).')
        self.assertRaisesRegex(TestError, 'test', ctl.ground, [('part', [Number(1)])], ctx)
