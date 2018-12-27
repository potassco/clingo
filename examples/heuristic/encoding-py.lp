#script (python)

import clingo
import copy
import networkx as nx

class ColouringHeuristic(object):
    """A largest degree based graph colouring heuristic for clingo"""
    def __init__(self):
        super(ColouringHeuristic, self).__init__()
        self.graph = nx.Graph()
        self.free_colours = dict()
        self.free = dict()
        self.chosen_colour_lit = dict()
        self.lit_chosen_colour = dict()
        self.deg_order = []

    def init(self, init):
        """Called at initialization time of the heuristic object.

        :type init: clingo.PropagateInit
        """
        colours = set()

        # create graph and colours, and chosenColour/2
        for a in init.symbolic_atoms:
            sym = str(a.symbol)
            if sym.startswith("node("):
                node = int(str(a.symbol.arguments[0]))
                self.graph.add_node(node)
            elif sym.startswith("link("):
                u = int(str(a.symbol.arguments[0]))
                v = int(str(a.symbol.arguments[1]))
                self.graph.add_edge(u,v)
            elif sym.startswith("colour("):
                colours.add(str(a.symbol.arguments[0]))
            elif sym.startswith("chosenColour("):
                lit = init.solver_literal(a.literal)
                if lit < 0: raise Exception("Negative literal found for chosenColour/2")
                node = int(str(a.symbol.arguments[0]))
                col = str(a.symbol.arguments[1])
                print("init {} = {}".format(sym,lit))
                # watch chosenColour/2
                init.add_watch(lit)
                init.add_watch(-lit)
                # set up literal mappings
                try:
                    self.chosen_colour_lit[node][col] = lit
                except KeyError:
                    self.chosen_colour_lit[node] = dict()
                    self.chosen_colour_lit[node][col] = lit
                try:
                    self.lit_chosen_colour[lit].add((node, col))
                except KeyError:
                    self.lit_chosen_colour[lit] = set()
                    self.lit_chosen_colour[lit].add((node, col))

        # create available colour dictionary per node
        for n in self.graph.nodes_iter():
            self.free_colours[n] = copy.copy(colours)
            self.free[n] = True

        # order vertices by degree
        self.deg_order = sorted(self.graph.nodes(), key=self.graph.degree,
                reverse=True)

    def __assign_col(self, node, col):
        self.free[node] = False
        for neighbor in self.graph.neighbors(node):
            try:
                self.free_colours[neighbor].remove(col)
            except KeyError:
                pass

    def __unassign_col(self, node, col):
        self.free[node] = True
        for neighbor in self.graph.neighbors(node):
            self.free_colours[neighbor].add(col)

    def propagate(self, ctl, changes):
        """Listen to changes in the assignment and constrain neighbours of
        choice nodes.

        :type ctl: clingo.PropagateControl
        :type changes: list
        :returns: TODO
        """
        print("propagate {}".format(changes))
        for lit in changes:
            if ctl.assignment.is_false(abs(lit)):
                for (node, col) in self.lit_chosen_colour[abs(lit)]:
                    print("prop {} != {}".format(node, col))
                    self.__assign_col(node, col)
            else:
                for (node, col) in self.lit_chosen_colour[abs(lit)]:
                    print("prop {} = {}".format(node, col))
                    self.__assign_col(node, col)

    def decide(self, vsids):
        """Decision function. Decides on the uncoloured vertex of largest
        degree that still has some colour available. On failure, returns VSIDS
        decision.

        :type vsids: int
        :returns: int
        """
        for n in self.deg_order:
            if self.free[n]:
                col = next(iter(self.free_colours[n]))
                lit = self.chosen_colour_lit[n][col]
                self.free[n] = False
                print("set {} = {} ({})".format(n,col,lit))
                return lit

        return vsids

    def undo(self, thread_id, assign, changes):
        """Listen to changes in the assignment

        :type thread_id: int
        :assign: TODO
        :type changes: list
        """
        print("undo {}".format(changes))
        for lit in changes:
            for (node, col) in self.lit_chosen_colour[abs(lit)]:
                self.__unassign_col(node, col)

def main(prg):
    prg.set_heuristic(ColouringHeuristic())
    prg.ground([("base", [])])
    ret = prg.solve()
    print(ret)

#end.
