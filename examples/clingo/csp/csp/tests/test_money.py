"""
Basic tests checking the whole system.

Note: this would also a good example to check show, domain and distinct
constraints should we ever implement them.
"""

import unittest
from csp.tests import solve

DOM = """\
%&dom {0..9} = X :- _letter(X).
&sum { X } <= 9 :- _letter(X).
&sum { X } >= 0 :- _letter(X).
"""

SMM = """\
_letter(s;e;n;d;m;o;r;y).

&sum {             1000*s + 100*e + 10*n + 1*d
     ;             1000*m + 100*o + 10*r + 1*e
     } = 10000*m + 1000*o + 100*n + 10*e + 1*y.
&sum { m } != 0.

%&distinct {X : _letter(X)}.
&sum { X } != Y :- _letter(X), _letter(Y), X < Y.

%&show {X : _letter(X)}.
"""

SMMD = """\
_letter(s;e;n;d;m;o;r;y).

&sum {             1000*s + 100*e + 10*n + 1*d
     ;             1000*m + 100*o + 10*r + 1*e
     } = 10000*m + 1000*o + 100*n + 10*e + 1*y.
&sum { m } != 0.

&distinct {X : _letter(X)}.

%&show {X : _letter(X)}.
"""

SOL = [[('d', 7), ('e', 5), ('m', 1), ('n', 6), ('o', 0), ('r', 8), ('s', 9), ('y', 2)]]
SOL10 = [
    [('d', 5), ('e', 8), ('m', 1), ('n', 0), ('o', 2), ('r', 7), ('s', 10), ('y', 3)],
    [('d', 6), ('e', 8), ('m', 1), ('n', 0), ('o', 2), ('r', 7), ('s', 10), ('y', 4)]] + SOL


# pylint: disable=missing-docstring

class TestMain(unittest.TestCase):
    def test_smm(self):
        self.assertEqual(solve(DOM + SMM), SOL)
        self.assertEqual(solve(SMM, minint=0, maxint=9), SOL)
        self.assertEqual(solve(SMM, minint=0, maxint=10), SOL10)

    def test_smmd(self):
        self.assertEqual(solve(DOM + SMMD), SOL)
        self.assertEqual(solve(SMMD, minint=0, maxint=9), SOL)
        self.assertEqual(solve(SMMD, minint=0, maxint=10), SOL10)
