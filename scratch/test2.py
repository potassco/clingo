# {{{ class FunVal

class FunVal:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def match(self, node, other, subst):
        node.matchFun(self, other, subst)

    def sig(self):
        return (self.name, len(self.args))

    def __eq__(self, other):
        return isinstance(other, FunVal) and self.name == other.name and self.args == other.args

    def __repr__(self):
        return self.name + "("+ ",".join([str(arg) for arg in self.args]) + ")"

# }}}
# {{{ class NumVal

class NumVal:
    def __init__(self, num):
        self.num = num

    def match(self, node, other, subst):
        node.matchNum(self, other, subst)

    def __eq__(self, other):
        return isinstance(other, NumVal) and self.num == other.num

    def __repr__(self):
        return str(self.num)

# }}}

# {{{ class SigTerm

class SigTerm:
    def __init__(self, name, n):
        self.name = name
        self.n = n

    def skip(self):
        return self.n

    def __repr__(self):
        return self.name + "/" + str(self.n)

    def add(self, node):
        return node.addSig(self)

    def unify(self, node, x, subst):
        return node.unifySig(self, x, subst)

    def occur(self, subst, name):
        return False

    def apply(self, subst):
        return [ self ]

    def __eq__(self, other):
        return isinstance(other, SigTerm) and self.name == other.name and self.n == other.n

    def __hash__(self):
        return hash((SigTerm, self.name, self.n))

# }}}
# {{{ class VarTerm

class VarTerm:
    def __init__(self, name):
        self.name = name

    def skip(self):
        return 0

    def __repr__(self):
        return self.name

    def add(self, node):
        return node.addVar(self)

    def unify(self, node, x, subst):
        return node.unifyVar(self, x, subst)

    def occur(self, subst, var):
        if self == var:
            return False
        else:
            return occur(subst.get(self, []), subst, var)

    def apply(self, subst):
        t = subst.get(self)
        if t:
            ret = []
            for x in t: ret += x.apply(subst)
            return ret
        else: return [ self ]

    def __hash__(self):
        return hash((VarTerm, self.name))

    def __eq__(self, other):
        return isinstance(other, VarTerm) and self.name == other.name

# }}}
# {{{ class VarTerm

class ValTerm:
    def __init__(self, val):
        self.val = val

    def skip(self):
        return 0

    def __repr__(self):
        return str(self.val)

# }}}
# {{{ functions on terms

def split(args):
    n = 1
    m = 0
    for x in args:
        m += 1
        n += x.skip() - 1
        if n == 0:
            return (args[:m], args[m:])
    return None

def occur(x, subst, var):
    for t in x:
        if t.occur(subst, var):
            return True
    return False

def unifyTerm(x, y, subst):
    if x[0] == y[0]:
        return unify(x[1:], y[1:])
    elif isinstance(x[0], VarTerm):
        if not occur(y, subst, x[0]):
            subst[x[0]] = y
            return True
        else: return False
    elif isinstance(y[0], VarTerm):
        if not occur(x, subst, y[0]):
            subst[y[0]] = x
            return True
        else: return False
    else:
        return False

def unify(x, y, subst):
    if x:
        xx, xxx = split(x)
        yy, yyy = split(y)
        xx = [ u for z in xx for u in z.apply(subst) ]
        yy = [ u for z in yy for u in z.apply(subst) ]
        return unifyTerm(xx, yy, subst) and unify(xxx, yyy, subst)
    else: return True

def funTerm(name, args):
    return [SigTerm(name, len(args))] + [x for y in args for x in y]

def varTerm(name):
    return [VarTerm(name)]

def valTerm(name):
    return [ValTerm(name)]

# }}}

# {{{ class Node

class Node:
    def __init__(self):
        self.fun = {}
        self.var = {}
        self.val = {}

    def addSig(self, sig):
        return self.fun.setdefault(sig, Node())

    def addVar(self, var):
        return self.var.setdefault(var, Node())

    def unify(self, x, subst):
        if not x:
            print("unified with: " + str(subst))
        else:
            x[0].unify(self, x, subst)

    def unifySig(self, sig, x, subst):
        node = self.fun.get(sig)
        if node:
            node.unify(x[1:], subst)
        for var, node in self.var.items():
            tt, xx = split(x)
            substB = dict(subst)
            if unify(tt, [var], substB):
                node.unify(xx, substB)

    def unifySkip(self, var, x, t, n, subst):
        if n == 0:
            substB = dict(subst)
            if unify([var], t, substB):
                self.unify(x[1:], substB)
        else:
            for funB, nodeB in self.fun.items():
                nodeB.unifySkip(var, x, t+[funB], n+funB.skip()-1, subst)
            for varB, nodeB in self.var.items():
                nodeB.unifySkip(var, x, t+[varB], n+varB.skip()-1, subst)

    def unifyVar(self, var, x, subst):
        t = subst.get(var)
        if t:
            self.unify(t+x[1:], subst)
        else:
            for fun, nodeB in self.fun.items():
                nodeB.unifySkip(var, x, [fun], fun.skip(), subst)
            for varB, nodeB in self.var.items():
                substB = dict(subst)
                if unify([var], [varB], substB):
                    nodeB.unify(x[1:], substB)

    def __repr__(self):
        return str(self.fun.items() + self.var.items())

# }}}
# {{{ class Lookup

class Lookup:
    def __init__(self):
        self.root = Node()

    def add(self, x):
        print("adding: " + str(x))
        node = self.root
        for t in x:
            node = t.add(node)

    def unify(self, x):
        print("unifying: " + str(x))
        self.root.unify(x, {})

    def __repr__(self):
        return str(self.root)

# }}}

# {{{ Tests

l = Lookup()
l.add(funTerm("p", [funTerm("f", [varTerm("X"), varTerm("Y")]), varTerm("Z")]))
l.add(funTerm("p", [funTerm("g", [varTerm("X"), varTerm("Y")]), varTerm("Z")]))
l.add(funTerm("p", [varTerm("X"), varTerm("Y")]))
l.add(funTerm("p", [varTerm("X"), varTerm("X")]))

l.unify(funTerm("p", [funTerm("g", [varTerm("A"), varTerm("B")]), varTerm("C")]))
l.unify(funTerm("p", [varTerm("A"), varTerm("B")]))

# }}}

"""
assigning variable names clerverly allows for using one substitution only
  (or better make all variables share the same value!!!
  this requires to change the variable names in the original term - this should be no problem!!!
implement match here too and then add VarTerm ...
"""
