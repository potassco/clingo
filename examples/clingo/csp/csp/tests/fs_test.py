"""
Basic tests checking the whole system.
"""

import unittest
from csp.tests import solve

FSI = """\
            machine(1).      machine(2).
task(a). duration(a,1,3). duration(a,2,4).
task(b). duration(b,1,1). duration(b,2,6).
task(c). duration(c,1,5). duration(c,2,5).
"""

FSE = """\
1 { cycle(T,U) : task(U), U != T } 1 :- task(T).
1 { cycle(T,U) : task(T), U != T } 1 :- task(U).

reach(M) :- M = #min { T : task(T) }.
reach(U) :- reach(T), cycle(T,U).
:- task(T), not reach(T).

1 { start(T) : task(T) } 1.

permutation(T,U) :- cycle(T,U), not start(U).

seq((T,M),(T,M+1),D) :- task(T), duration(T,M,D), machine(M+1).
seq((T1,M),(T2,M),D) :- permutation(T1,T2), duration(T1,M,D).

&sum {  1*T1 + -1*T2 } <= -D :- seq(T1,T2,D).
&sum { -1*(T,M) } <= 0       :- duration(T,M,D).
&sum {  1*(T,M) } <= B       :- duration(T,M,D), B=bound-D.

#show permutation/2.
"""

FSD = """\
1 { cycle(T,U) : task(U), U != T } 1 :- task(T).
1 { cycle(T,U) : task(T), U != T } 1 :- task(U).

reach(M) :- M = #min { T : task(T) }.
reach(U) :- reach(T), cycle(T,U).
:- task(T), not reach(T).

1 { start(T) : task(T) } 1.

permutation(T,U) :- cycle(T,U), not start(U).

seq((T,M),(T,M+1),D) :- task(T), duration(T,M,D), machine(M+1).
seq((T1,M),(T2,M),D) :- permutation(T1,T2), duration(T1,M,D).

&diff { T1-T2 } <= -D :- seq(T1,T2,D).
&diff { 0-(T,M) } <= 0 :- duration(T,M,D).
&sum { (T,M)-0 } <= B :- duration(T,M,D), B=bound-D.

#show permutation/2.
"""

SOL11 = [['permutation(a,c)', 'permutation(b,a)', ('(a,1)', 1), ('(a,2)', 7), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 4), ('(c,2)', 11)],
         ['permutation(a,c)', 'permutation(b,a)', ('(a,1)', 1), ('(a,2)', 7), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 5), ('(c,2)', 11)],
         ['permutation(a,c)', 'permutation(b,a)', ('(a,1)', 1), ('(a,2)', 7), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 6), ('(c,2)', 11)],
         ['permutation(a,c)', 'permutation(b,a)', ('(a,1)', 2), ('(a,2)', 7), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 5), ('(c,2)', 11)],
         ['permutation(a,c)', 'permutation(b,a)', ('(a,1)', 2), ('(a,2)', 7), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 6), ('(c,2)', 11)],
         ['permutation(a,c)', 'permutation(b,a)', ('(a,1)', 3), ('(a,2)', 7), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 6), ('(c,2)', 11)]]

SOL16 = SOL11 + [
        ['permutation(b,c)', 'permutation(c,a)', ('(a,1)', 6), ('(a,2)', 12), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 1), ('(c,2)', 7)],
        ['permutation(b,c)', 'permutation(c,a)', ('(a,1)', 7), ('(a,2)', 12), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 1), ('(c,2)', 7)],
        ['permutation(b,c)', 'permutation(c,a)', ('(a,1)', 7), ('(a,2)', 12), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 2), ('(c,2)', 7)],
        ['permutation(b,c)', 'permutation(c,a)', ('(a,1)', 8), ('(a,2)', 12), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 1), ('(c,2)', 7)],
        ['permutation(b,c)', 'permutation(c,a)', ('(a,1)', 8), ('(a,2)', 12), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 2), ('(c,2)', 7)],
        ['permutation(b,c)', 'permutation(c,a)', ('(a,1)', 9), ('(a,2)', 12), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 1), ('(c,2)', 7)],
        ['permutation(b,c)', 'permutation(c,a)', ('(a,1)', 9), ('(a,2)', 12), ('(b,1)', 0), ('(b,2)', 1), ('(c,1)', 2), ('(c,2)', 7)]]


# pylint: disable=missing-docstring

class TestMain(unittest.TestCase):
    def test_fse(self):
        self.assertEqual(solve(FSE + FSI, maxint=10, options=["-c", "bound=16"]), [])
        self.assertEqual(solve(FSE + FSI, maxint=11, options=["-c", "bound=16"]), SOL11)
        self.assertEqual(solve(FSE + FSI, options=["-c", "bound=16"]), SOL16)

    def test_fsd(self):
        self.assertEqual(solve(FSD + FSI, maxint=10, options=["-c", "bound=16"]), [])
        self.assertEqual(solve(FSD + FSI, maxint=11, options=["-c", "bound=16"]), SOL11)
        self.assertEqual(solve(FSD + FSI, options=["-c", "bound=16"]), SOL16)
