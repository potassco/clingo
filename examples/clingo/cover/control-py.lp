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
        self.last = m.symbols(atoms=True)
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
        pre.load("instance.lp")
        pre.load("preprocess.lp")
        pre.ground([("base", [])])
        ret = pre.solve(on_model=self.on_model)
        k = 0
        while ret.satisfiable:
            k = k + 1
            pre.cleanup()
            pre.ground([("step", [k])], self)
            ret = pre.solve(on_model=self.on_model)
        self.prepare_instance(k)
        prg.load("cover.lp")
        prg.ground([("base", [])], self)
        prg.solve()

def main(prg):
    s = State()
    s.run(prg)

#end.
