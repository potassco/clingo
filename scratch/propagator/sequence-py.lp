#script (python)

def resize(l, n, d = None):
    while len(l) < n:
        l.append(d)

class Propagator:
    class State:
        def __init__(self, pat_len, seq_len):
            self.seq_active = [] # [bool|None]
            self.stack = []      # [+pattern index|-sequence index]
            self.trail = []      # [(decision level,stack index)]
            self.pat = []        # [(pattern literal, item index)|None]
            self.pat_assigned = 0
            resize(self.pat, pat_len)
            resize(self.seq_active, seq_len, True)

    def __init__(self, trace):
        self.__seq_atoms = []     # [(sequence literal, [item index])]
        self.__pat_atoms = dict() # {pattern literal: [(pattern index, item index)]}
        self.__occ_list = []      # [[sequence index]]
        self.__item_map = dict()  # {item: item index}
        self.__trace = trace      # enable this to see what happens
        self.__pat_len = 0        # the pattern length
        self.__states = []        # [State]

    # {{{1 Initialization Functions

    def __state(self, sid):
        while len(self.__states) <= sid:
            self.__states.append(Propagator.State(self.__pat_len, len(self.__seq_atoms)))
        return self.__states[sid]

    def __map_item(self, item):
        return self.__item_map.setdefault(item, len(self.__item_map))

    def __add_seq_atom(self, init, sid, atom):
        resize(self.__seq_atoms, sid + 1)
        assert(self.__seq_atoms[sid] is None)
        self.__seq_atoms[sid] = (init.map_literal(atom.literal()), [])
        for elem in atom.elements():
            index, item = elem.terms()
            index = index.number()
            assert(index >= 0)
            item = self.__map_item(str(item))
            resize(self.__seq_atoms[sid][1], index + 1);
            self.__seq_atoms[sid][1][index] = item

    def __add_pat_atoms(self, init, atom):
        for elem in atom.elements():
            lit = init.map_literal(elem.condition_literal())
            index, item = elem.terms()
            index = index.number()
            assert(index >= 0)
            item = self.__map_item(str(item))
            pat = (index, item)
            if lit not in self.__pat_atoms:
                init.add_watch(lit)
            self.__pat_atoms.setdefault(lit, []).append(pat)
            self.__pat_len = max(self.__pat_len, index + 1)

    def __init_occ_list(self):
        for item in self.__item_map:
            self.__occ_list.append([])
        sid = 0
        for seq in self.__seq_atoms:
            for item in set(seq[1]):
                assert(item is not None)
                self.__occ_list[item].append(sid)
            sid += 1

    def init(self, init):
        for atom in init.theory_iter():
            term = atom.term()
            args = term.args()
            if term.name() == "seq" and len(args) == 1:
                self.__add_seq_atom(init, args[0].number(), atom)
            if term.name() == "pat" and len(args) == 0:
                self.__add_pat_atoms(init, atom)
        self.__init_occ_list()

    # {{{1 Tracing Functions
    def __str_item(self, iid):
        for item, jid in self.__item_map.iteritems():
            if iid == jid:
                return item

    def __str_items(self, items):
        s = ""
        for item in items:
            s+= self.__str_item(item)
        return s

    def __str_pat(self, state):
        s = ""
        for pat in state.pat:
            s+= "." if pat is None else self.__str_item(pat[1])
        return s

    def __str_seq(self, sid):
        return self.__str_items(self.__seq_atoms[sid][1])

    # {{{1 Propagation

    def __propagate_sequence_lit(self, state, control, assign, sid, lit):
        if self.__trace:
            print "      implication:", self.__str_pat(state), "=>", ("" if lit == self.__seq_atoms[sid][0] else "-") + self.__str_seq(sid)
        state.seq_active[sid] = None
        state.stack.append(-sid-1)
        if not assign.is_true(lit):
            klaus = [lit]
            for pat in state.pat:
                if pat is not None:
                    klaus.append(-pat[0])
            return control.add_clause(klaus) and control.propagate()
        return True

    def __propagate_sequence(self, state, control, assign, sid, lit, items):
        if self.__trace:
            print "    propagate sequence:", self.__str_items(items)
        if state.pat_assigned < self.__pat_len and assign.is_false(lit):
            return True
        iid = 0
        for pat in state.pat:
            while True:
                if iid == len(items):
                    return self.__propagate_sequence_lit(state, control, assign, sid, -lit)
                iid+= 1
                if pat is None or items[iid-1] == pat[1]:
                    break
        return state.pat_assigned < self.__pat_len or self.__propagate_sequence_lit(state, control, assign, sid, lit)

    def propagate(self, control, assign, changes):
        state = self.__state(control.id())
        if len(state.trail) == 0 or state.trail[-1][0] < assign.decision_level():
            state.trail.append((assign.decision_level(), len(state.stack)))
        if self.__trace:
            print "propagate " + str(len(changes)) + " atoms"
        for lit in changes:
            for pat in self.__pat_atoms[lit]:
                if state.pat[pat[0]] is not None:
                    # this case should not occur if the pattern is generated properly
                    old = state.pat[pat[0]][0]
                    assert(assign.is_true(old))
                    return control.add_clause([-lit, -old]) and control.propagate()
                else:
                    state.stack.append(pat[0])
                    state.pat_assigned+= 1
                    state.pat[pat[0]] = (lit, pat[1])
                    if self.__trace:
                        print "  propagate index", str(pat[0]), "in", self.__str_pat(state)
                    for sid in self.__occ_list[pat[1]]:
                        if state.seq_active[sid] and not self.__propagate_sequence(state, control, assign, sid, *self.__seq_atoms[sid]):
                            return False

        return True

    def undo(self, solver_id, undo):
        state = self.__state(solver_id)
        sid = state.trail[-1][1]
        for psid in state.stack[sid:]:
            if psid >= 0:
                state.pat[psid] = None
                state.pat_assigned-= 1
            else:
                state.seq_active[-psid-1] = True
        del state.stack[sid:]
        state.trail.pop()
        if self.__trace:
            print "undoing", len(undo), "literals:", self.__str_pat(state)

    def model(self, control, assign):
        return True

    # }}}1

def main(prg):
    prg.register_propagator(Propagator(bool(prg.get_const("trace"))))
    prg.ground([("base", [])])
    prg.solve()

#end.

#show pat/2.

#theory seq {
    term { };
    &seq/1 : term, body;
    &pat/0 : term, directive
}.

#const n = 5.

1 { pat(P,I) : seq(_,_,I) } 1 :- P = 0..(n-1).

&pat { P,I : pat(P,I) }.
sup(U) :- &seq(U) { P,I : seq(U,P,I) }, seq(U,_,_).

% Note: these rules should be generated by the python script
% this needs an additional low level interface for the logic program
:- not sup(U), seq(U,_,_), n == 0.
:- sup(U), pat(_,I), not seq(U,_,I).

#maximize { 1,U : sup(U) }.

