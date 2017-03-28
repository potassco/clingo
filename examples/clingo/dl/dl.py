import clingo, heapq

class Graph:
    def __init__(self):
        self.__potential = {}          # {node: potential}
        self.__graph = {}              # {node: {node : weight}}
        self.__gamma = {}              # {node: gamma}
        self.__last_edges = {}         # {node: edge}
        self.__previous_edge = {}      # {level: {(node, node): weight}}
        self.__previous_potential = {} # {level: {node: potential}}

    def __set(self, level, key, val, previous, get_current):
        p = previous.setdefault(level, {})
        c, k = get_current(key)
        if not key in p:
            if k in c: p[key] = c[k]
            else:      p[key] = None
        c[k] = val

    def __reset(self, level, previous, get_current):
        if level in previous:
            for key, val in previous[level].items():
                c, k = get_current(key)
                if val is None: del c[k]
                else:           c[k] = val
            del previous[level]

    def __set_potential(self, level, key, val):
        self.__set(level, key, val, self.__previous_potential, lambda key: (self.__potential, key))

    def add_edge(self, level, edge):
        u, v, d = edge
        # If edge already exists from u to v with lower weight, new edge is redundant
        if u in self.__graph and v in self.__graph[u] and self.__graph[u][v] <= d:
            return None

        # Initialize potential and graph
        if u not in self.__potential:
            self.__set_potential(level, u, 0)
        if v not in self.__potential:
            self.__set_potential(level, v, 0)
        self.__graph.setdefault(u, {})
        self.__graph.setdefault(v, {})

        changed = set() # Set of nodes for which potential has been changed
        min_gamma = []

        # Update potential change induced by new edge, 0 for other nodes
        self.__gamma[u] = 0
        self.__gamma[v] = self.__potential[u] + d - self.__potential[v]

        if self.__gamma[v] < 0:
            heapq.heappush(min_gamma, (self.__gamma[v], v))
            self.__last_edges[v] = (u, v, d)

        # Propagate negative potential change
        while len(min_gamma) > 0 and self.__gamma[u] == 0:
            _, s = heapq.heappop(min_gamma)
            if s not in changed:
                self.__set_potential(level, s, self.__potential[s] + self.__gamma[s])
                self.__gamma[s] = 0
                changed.add(s)
                for t in self.__graph[s]:
                    if t not in changed:
                        gamma_t = self.__potential[s] + self.__graph[s][t] - self.__potential[t]
                        if gamma_t < self.__gamma[t]:
                            self.__gamma[t] = gamma_t
                            heapq.heappush(min_gamma, (gamma_t, t))
                            self.__last_edges[t] = (s, t, self.__graph[s][t])

        cycle = None
        # Check if there is a negative cycle
        if self.__gamma[u] < 0:
            cycle = []
            x, y, c = self.__last_edges[v]
            cycle.append((x, y, c))
            while v != x:
                x, y, c = self.__last_edges[x]
                cycle.append((x, y, c))
        else:
            self.__set(level, (u, v), d, self.__previous_edge, lambda key: (self.__graph[key[0]], key[1]))

        # Ensure that all gamma values are zero
        self.__gamma[v] = 0;
        while len(min_gamma) > 0:
            _, s = heapq.heappop(min_gamma)
            self.__gamma[s] = 0

        return cycle

    def get_assignment(self):
        adjust = self.__potential['0'] if '0' in self.__potential else 0
        return [(node, adjust-self.__potential[node]) for node in self.__potential if node != '0']

    def backtrack(self, level):
        self.__reset(level, self.__previous_edge, lambda key: (self.__graph[key[0]], key[1]))
        self.__reset(level, self.__previous_potential, lambda key: (self.__potential, key))

class Propagator:
    def __init__(self):
        self.__l2e = {}    # {literal: [(node, node, weight)]}
        self.__e2l = {}    # {(node, node, weight): [literal]}
        self.__states = [] # [Graph]

    def init(self, init):
        for atom in init.theory_atoms:
            term = atom.term
            if term.name == "diff" and len(term.arguments) == 0:
                if len(atom.guard[1].arguments) > 0:
                    weight = -atom.guard[1].arguments[0].number
                else:
                    weight = atom.guard[1].number
                u = str(atom.elements[0].terms[0].arguments[0])
                v = str(atom.elements[0].terms[0].arguments[1])
                edge = (u, v, weight)
                lit = init.solver_literal(atom.literal)
                self.__l2e.setdefault(lit, []).append(edge)
                self.__e2l.setdefault(edge, []).append(lit)
                init.add_watch(lit)

    def propagate(self, control, changes):
        state = self.__state(control.thread_id)
        level = control.assignment.decision_level
        for lit in changes:
            for edge in self.__l2e[lit]:
                cycle = state.add_edge(level, edge)
                if cycle is not None:
                    c = [self.__literal(control, e) for e in cycle]
                    control.add_nogood(c) and control.propagate()
                    return

    def undo(self, thread_id, assign, changes):
        self.__state(thread_id).backtrack(assign.decision_level)

    def get_assignment(self, thread_id):
        return self.__state(thread_id).get_assignment()

    def __state(self, thread_id):
        while len(self.__states) <= thread_id:
            self.__states.append(Graph())
        return self.__states[thread_id]

    def __literal(self, control, edge):
        for lit in self.__e2l[edge]:
            if control.assignment.is_true(lit):
                return lit

