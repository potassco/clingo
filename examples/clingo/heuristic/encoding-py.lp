#script (python)

import clingo

class ColoringHeuristic:
    def __init__(self):
        self.graph = {}
        self.degree = {}
        self.assign = {}

    def init(self, init):
        for a in init.symbolic_atoms:
            if a.match("edge", 2):
                u = a.symbol.arguments[0]
                v = a.symbol.arguments[1]
                self.graph.setdefault(u, []).append(v)
                self.graph.setdefault(v, []).append(u)
                self.degree.setdefault(u, 0)
                self.degree.setdefault(v, 0)
                self.degree[u] += 1
                self.degree[v] += 1
            elif a.match("assign", 2):
                u = a.symbol.arguments[0]
                l = init.solver_literal(a.literal)
                self.assign.setdefault(l, []).append(u)
                init.add_watch(l)

    def propagate(self, ctl, changes):
        for l in changes:
            for u in self.assign[l]:
                for v in self.graph[u]:
                    self.degree[v] -= 1

    def undo(self, thread_id, assign, changes):
        for l in changes:
            for u in self.assign[l]:
                for v in self.graph[u]:
                    self.degree[v] += 1

    def decide(self, fallback):
        # in practice this implementation is unusable because it is too inefficient
        # it is out of the scope of this example to implement this efficiently here
        # hint: heap
        free, degree = fallback, 0
        for l, u in self.assign.items():
            # if assign.value(l) is None and degree < self.degree[u]:
            #     free = l
            pass
        return free

def main(prg):
    prg.register_propagator(ColoringHeuristic())
    prg.ground([("base", [])])
    ret = prg.solve()

#end.

1 { assign(U,C) : color(C) } 1 :- vertex(U).
:- edge(U,V), assign(U,C), assign(V,C).

