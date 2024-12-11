# {{{ Val
class FunVal:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def match(self, node, other, subst):
        node.matchFun(self, other, subst)

    def sig(self):
        return (self.name, len(self.args))

    def __eq__(self, other):
        return (
            isinstance(other, FunVal)
            and self.name == other.name
            and self.args == other.args
        )

    def __repr__(self):
        return self.name + "(" + ",".join([str(arg) for arg in self.args]) + ")"


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
# {{{ Term


class FunTerm:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def sig(self):
        return (self.name, len(self.args))

    def add(self, node, other, leaf):
        node.addFun(self, other, leaf)

    def unify(self, node, other, subst):
        node.unifyFun(self, other, subst)

    def occurs(self, subst, var):
        for arg in self.args:
            if arg.occurs(subst, var):
                return True
        return False

    def __repr__(self):
        return self.name + "(" + ",".join([str(arg) for arg in self.args]) + ")"


class VarTerm:
    def __init__(self, name):
        self.name = name

    def add(self, node, other, leaf):
        node.addVar(self, other, leaf)

    def unify(self, node, other, subst):
        node.unifyVar(self, other, subst)

    def occurs(self, subst, var):
        if self.name == var:
            return True
        else:
            t = subst.get(self.name)
            return t != None and t.occurs(subst, var)

    def __repr__(self):
        return self.name


# class ValTerm:
#     ...

# }}}


class Node:
    def __init__(self):
        self.fun = {}
        self.var = {}
        self.leaf = None

    def addFun(self, fun, other, leaf):
        x = self.fun.setdefault(fun.sig(), Node())
        n = fun.args + other
        n[0].add(x, n[1:], leaf)

    def addVar(self, var, other, leaf):
        x = self.var.setdefault(var.name, Node())
        if len(other) > 0:
            other[0].add(x, other[1:], leaf)
        else:
            x.leaf = leaf

    def __matchVar(self, val, other, subst):
        for var, node in self.var.items():
            match = True
            if var in subst:
                match = subst[var] == val
            else:
                subst = dict(subst)
                subst[var] = val
            if match:
                if len(other) > 0:
                    other[0].match(node, other[1:], subst)
                else:
                    print("  matched: " + str(node.leaf) + " with: " + str(subst))

    def matchFun(self, fun, other, subst):
        node = self.fun.get(fun.sig())
        if node != None:
            n = fun.args + other
            n[0].match(node, n[1:], subst)
        self.__matchVar(fun, other, subst)

    def matchNum(self, num, other, subst):
        self.__matchVar(num, other, subst)

    def unifyFun(self, fun, other, subst):
        node = self.fun.get(fun.sig())
        if node != None:
            n = fun.args + other
            n[0].unify(node, n[1:], subst)
        for var, node in self.var.items():
            t = subst.get(var)
            match = True
            if t != None:
                print("  TODO: unify " + str(fun) + " with " + str(t))
                match = False
            else:
                if not fun.occurs(subst, var):
                    subst = dict(subst)
                    subst[var] = fun
                else:
                    match = False
            if match:
                if len(other) > 0:
                    other[0].unify(node, other[1:], subst)
                else:
                    print("  matched: " + str(node.leaf) + " with: " + str(subst))

    def getFun(self, name, n, args):
        # the data structures don't go together very well :(
        if n == 0:
            return [(self, FunTerm(name, args))]
        else:
            ret = []
            for (nameB, nB), nodeB in self.fun.items():
                funs = nodeB.getFun(nameB, nB, [])
                for node, fun in funs:
                    ret.extend(node.getFun(name, n - 1, args + [fun]))
            for varB, nodeB in self.var.items():
                ret.extend(nodeB.getFun(name, n - 1, args + [VarTerm(varB)]))
            return ret

    def unifyVar(self, var, other, subst):
        t = subst.get(var.name)
        if t != None:
            t.unify(self, other, subst)
        else:
            for (name, n), node in self.fun.items():
                for nodeB, fun in node.getFun(name, n, []):
                    if not fun.occurs(subst, var.name):
                        substB = dict(subst)
                        substB[var] = fun
                        if len(other) > 0:
                            other[0].unify(nodeB, other[1:], substB)
                        else:
                            print(
                                "  matched: "
                                + str(nodeB.leaf)
                                + " with: "
                                + str(substB)
                            )

            for varB, node in self.var.items():
                t = subst.get(varB)
                match = True
                if t != None:
                    print("  TODO: unify " + str(var) + " with " + str(t))
                    match = False
                else:
                    if var.name != varB:
                        subst = dict(subst)
                        subst[var] = VarTerm(varB)
                if match:
                    if len(other) > 0:
                        other[0].unify(node, other[1:], subst)
                    else:
                        print("  matched: " + str(node.leaf) + " with: " + str(subst))

    def toString(self, ident):
        s = ""
        for x, y in self.fun.items():
            s += ident + str(x) + "\n"
            s += y.toString(ident + "  ")
        for x, y in self.var.items():
            s += ident + str(x) + "\n"
            s += y.toString(ident + "  ")
        if self.leaf != None:
            s += ident + "*" + str(self.leaf) + "*\n"
        return s


class Lookup:
    def __init__(self):
        self.root = Node()

    def add(self, x):
        print("adding: " + str(x))
        x.add(self.root, [], x)

    def match(self, x):
        print("matching: " + str(x))
        x.match(self.root, [], {})

    def unify(self, x):
        print("unifying: " + str(x))
        x.unify(self.root, [], {})

    def __repr__(self):
        return "root:\n" + self.root.toString("  ")


l = Lookup()
l.add(FunTerm("p", [FunTerm("f", [VarTerm("X"), VarTerm("Y")]), VarTerm("Z")]))
l.add(FunTerm("p", [FunTerm("g", [VarTerm("X"), VarTerm("Y")]), VarTerm("Z")]))
l.add(FunTerm("p", [VarTerm("X"), VarTerm("Y")]))
l.add(FunTerm("p", [VarTerm("X"), VarTerm("X")]))
print(l)

# next match tuples
l.match(FunVal("p", [FunVal("f", [NumVal(1), NumVal(2)]), NumVal(3)]))
l.match(
    FunVal(
        "p", [FunVal("f", [NumVal(1), NumVal(2)]), FunVal("f", [NumVal(1), NumVal(2)])]
    )
)

# next unify terms
l.unify(FunTerm("p", [FunTerm("g", [VarTerm("A"), VarTerm("B")]), VarTerm("C")]))
l.unify(FunTerm("p", [VarTerm("A"), VarTerm("B")]))
"""
subst:
    String -> Term
cases to consider:
    a:VarTerm - b:FunTerm
        if a in subst:
            # moves completely to terms ...
            return unify(subst[a], b, subst)
        else:
            # easily implemented ...
            if b.occur(subst, a.name): # applies the substitution on the fly
                return False
            subst[a.name] = b
            return True
    a:FunTerm - b:FunTerm
        # easiliy implemented ...
        if a.sig() != b.sig():
            return False
        for x, y in zip(a.args(), b.args()):
            if not unify(x, y, subst):
                return False
        return True
    a:FunTerm - b:VarTerm
        # needs extraction of FunTerm
        return unify(b, a, subst)
    a:VarTerm - b:VarTerm
        if a in subst:
            a = subst[a.name]
            return unify(a, b, subst)
        elif b in subst:
            b = subst[b.name]
            return unify(a, b, subst)
        elif a.name != b.name:
            # occurs check???
            subst[a.name] = b
            return True
        else:
            return True

the trick is to always let unify apply a substitution until a fixpoint
implement this similar to match in Lookup
afterwards adding these should be straightforward:
    next add ValTerms
    next add LinearTerm
"""
