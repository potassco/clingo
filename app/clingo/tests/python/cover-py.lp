#script (python)

import clingo
import sys

class State:
    def __init__(self):
        self.leaves = {}
        self.last = None

    def fix(self, k):
        return self.leaves.get(k.number-1, [])

    def on_model(self, m):
        self.last = m.atoms(atoms=True)
        for atom in self.last:
            if atom.name == "leaf":
                args = atom.arguments
                self.leaves[args[0]] = args[1]

    def prepare_instance(self, k):
        edges = []
        cover = []
        vertices = set()
        for atom in self.last:
            if atom.name == "edge":
                args = atom.arguments
                if args[0].number == k-1:
                    edges.append((args[1], args[2]))
                    vertices.add(args[1])
                    vertices.add(args[2])
            if atom.name == "cover":
                cover.append(atom.arguments[1])
        vertices = list(vertices)
        setattr(self, "edge", lambda: edges)
        setattr(self, "cover", lambda: cover)
        setattr(self, "vertex", lambda: vertices)

    def run(self, prg):
        pre = clingo.Control()
        pre.add("base", [], """
edge(a,(b;c)).
edge(b,(c;d)).
edge(d,e).
edge(e,f).

edge(x,(a;c)). % edge to keep something to solve

edge(X,Y) :- edge(Y,X).
vertex(X) :- edge(X,Y;Y,X).

edge(0,X,Y) :- edge(X,Y).
        """)
        pre.add("step", ["k"], """
:- not leaf(k-1,@fix(k)).

1 { leaf(k,X) : vertex(X) } 1.
cover(k,X) :- edge(k-1,X,Y), leaf(k,Y).
edge(k,X,Y) :- edge(k-1,X,Y), not cover(k,X), not cover(k,Y).

:- edge(k-1,X,Y), leaf(k,Y), edge(k-1,Y,Z), X < Z.
:- leaf(k,Y), not edge(k-1,_,Y).
        """)
        pre.ground([("base", [])])
        ret = pre.solve(on_model = self.on_model)
        k = 0
        while ret.satisfiable is True:
            k = k + 1
            pre.cleanup()
            pre.ground([("step", [k])], self)
            ret = pre.solve(on_model = self.on_model)
        self.prepare_instance(k)
        prg.add("base", [], """
vertex(X) :- X = @vertex().
edge(X,Y) :- (X,Y) = @edge().
cover(X)  :- X = @cover().

{ cover(X) : vertex(X) }.
:- edge(X,Y), not cover(X), not cover(Y).
%#minimize { 1,X : cover(X) }.
#show cover/1.
        """)
        prg.ground([("base", [])], self)
        prg.solve()

def main(prg):
    s = State()
    s.run(prg)

#end.
