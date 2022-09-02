'''
Tests for aspif parsing.
'''
from unittest import TestCase
from textwrap import dedent

from ..control import Control


class TestASPIF(TestCase):
    '''
    Test ASPIF parsing.
    '''

    def test_preamble(self):
        '''
        Test preamble parsing.
        '''
        ctl = Control()
        ctl.add(dedent("""\
            asp 1 0 0
            0
            """))
        ctl = Control([], lambda msg, code: None)
        self.assertRaises(RuntimeError, ctl.add, dedent("""\
            asp 1 0 0 unknown
            0
            """))
        ctl = Control([], lambda msg, code: None)
        ctl.add(dedent("""\
            asp 1 0 0 incremental
            0
            """))
        self.assertRaises(RuntimeError, ctl.add, dedent("""\
            asp 1 0 0 incremental
            0
            """))
        ctl = Control([], lambda msg, code: None)
        self.assertRaises(RuntimeError, ctl.add, dedent("""\
            asp 1 0 0 incremental
            0
            0
            """))
