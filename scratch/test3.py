# {{{ class NumVal

class NumVal:
    def __init__(self, num):
        self.num = num

    def getSig(self):
        raise Exception("can only happen if used wrongly!!!")

    def match(self, x):
        return x.matchNum(self)

    def __hash__(self):
        return hash((NumVal, self.num))

    def __eq__(self, other):
        return isinstance(other, NumVal) and self.num == other.num

    def __repr__(self):
        return str(self.num)

# }}}
# {{{ class FunVal

class FunVal:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def getSig(self):
        return (self.name, len(self.args))

    def match(self, x):
        return x.matchFun(self)

    def __hash__(self):
        return hash((NumVal, self.name, tuple(self.args)))

    def __eq__(self, other):
        return isinstance(other, FunVal) and self.name == other.name and self.args == other.args

    def __repr__(self):
        return self.name + "(" + ",".join([str(x) for x in self.args]) + ")"

# }}}

# {{{ class ValTerm

class ValTerm:
    def __init__(self, val):
        self.val = val

    def getSig(self):
        return self.val.getSig()

    def eval(self):
        return self.val

    def init(self, varMap):
        pass

    def occurs(self, name):
        return False

    def matchNum(self, val):
        return self.val == val

    def matchFun(self, fun):
        return self.val == fun

    def unify(self, x):
        return self.val.match(x)

    def unifyVar(self, var):
        return self.val.match(var)

    def unifyFun(self, fun):
        return self.val.match(fun)

    def __repr__(self):
        return str(self.val)

# }}}
# {{{ class FunTerm

class FunTerm:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def getSig(self):
        return (self.name, len(self.args))

    def eval(self):
        args = []
        for x in self.args:
            args.append(x.eval())
            if not args[-1]:
                return False
        return FunVal(self.name, args)

    def init(self, varMap):
        for x in self.args:
            x.init(varMap)

    def occurs(self, name):
        for x in self.args:
            if x.occurs(name):
                return True
        return False

    def matchNum(self, val):
        return False

    def matchFun(self, fun):
        if self.getSig() != fun.getSig():
            return False
        for x, y in zip(self.args, fun.args):
            if not y.match(x):
                return False
        return True

    def unify(self, x):
        return x.unifyFun(self)

    def unifyVar(self, var):
        if var.ref:
            return var.ref.unifyFun(self)
        elif not self.occurs(var.name):
            var.ref.setTerm(self)
            return True
        else:
            return False

    def unifyFun(self, fun):
        if self.getSig() != fun.getSig():
            return False
        for x, y in zip(self.args, fun.args):
            if not y.unify(x):
                return False
        return True

    def __repr__(self):
        return self.name + "(" + ",".join([str(x) for x in self.args]) + ")"

# }}}
# {{{ class VarTerm

class ValRef:
    def __init__(self):
        self.__val  = None
        self.__term = None

    def __nonzero__(self):
        return 1 if self.__val or self.__term else 0

    def get(self):
        if self.__val:
            return self.__val
        else:
            return self.__term

    def setVal(self, x):
        self.__val = x
        self.__term = None

    def setTerm(self, x):
        self.__term = x
        self.__val  = None

    def clear(self):
        self.__term = None
        self.__val  = None

    def unifyVar(self, x):
        if self.__val:
            return self.__val.match(x)
        else:
            return self.__term.unifyVar(x)

    def unifyFun(self, x):
        if self.__val:
            return self.__val.match(x)
        else:
            return self.__term.unifyFun(x)

    def match(self, x):
        if self.__val:
            return self.__val == x
        else:
            return x.match(self.__term)

class VarTerm:
    def __init__(self, name):
        self.name = name
        self.ref  = None

    def getSig(self):
        raise Exception("can only happen if used wrongly!!!")

    def eval(self):
        return None

    def init(self, varMap):
        self.ref = varMap.setdefault(self.name, ValRef())

    def occurs(self, name):
        return self.name == name

    def matchNum(self, val):
        if self.ref:
            return self.ref.match(val)
        else:
            self.ref.setVal(val)
            return True

    def matchFun(self, fun):
        if self.ref:
            return self.ref.match(fun)
        else:
            self.ref.setVal(fun)
            return True

    def unify(self, x):
        return x.unifyVar(self)

    def unifyVar(self, var):
        if self.ref:
            return self.ref.unifyVar(var)
        elif var.ref:
            return var.ref.unifyVar(var)
        elif self.name == var.name:
            return True
        else:
            self.ref.setTerm(var)
            return True

    def unifyFun(self, fun):
        if self.ref:
            return self.ref.unifyFun(fun)
        elif not fun.occurs(self.name):
            self.ref.setVal(fun)
            return True
        else:
            return False

    def __repr__(self):
        return self.name

# }}}
# {{{ class LinearTerm

class LinearTerm:
    def __init__(self, name, m, n):
        self.name = name
        self.m    = m
        self.n    = n
        self.ref  = None

    def getSig(self):
        raise Exception("can only happen if used wrongly!!!")

    def eval(self):
        return None

    def init(self, varMap):
        self.ref = varMap.setdefault(self.name, ValRef())

    def occurs(self, name):
        return self.name == name

    def matchNum(self, num):
        n  = num.num
        n -= self.n
        if n % self.m != 0:
            return False
        n /= self.m
        if self.ref:
            return self.ref.match(NumVal(n))
        else:
            self.ref.setVal(NumVal(n))
            return True

    def matchFun(self, fun):
        return False

    def unify(self, x):
        # serves as a crude approximation
        # that should work well in practice
        # Note: that more could be done
        return True

    def unifyVar(self, var):
        # see LinearTerm::unify
        return True

    def unifyFun(self, fun):
        return False

    def __repr__(self):
        return "(" + str(self.m) + "*" + self.name + "+" + str(self.n) + ")"

# }}}

# {{{ class Occurrence

class Occurrence:
    def __init__(self, term):
        self.term  = term
        self.subst = []
        varMap = {}
        self.term.init(varMap)
        self.subst = list(varMap.items())

    def getSig(self):
        return self.term.getSig()

    def eval(self):
        return self.term.eval()

    def match(self, x):
        if x.match(self.term):
            subst = []
            for name, ref in self.subst:
                if ref:
                    subst.append((name, ref.get()))
                    ref.clear()
            return subst
        else:
            for _, ref in self.subst: ref.clear()
            return None

    def unify(self, x):
        if x.term.unify(self.term):
            subst = []
            for name, ref in self.subst + x.subst:
                if ref:
                    subst.append((name, ref.get()))
                    ref.clear()
            return subst
        else:
            for _, ref in self.subst + x.subst: ref.clear()
            return None

    def __repr__(self):
        return str(self.term)

# }}}

# {{{ class Lookup

class Lookup:
    def __init__(self):
        self.funs = {}

    def add(self, x):
        print("adding: " + str(x))
        sig = x.getSig()
        y = self.funs.setdefault(sig, ({}, []))
        z = x.eval()
        if z: y[0].setdefault(z, []).append(x)
        else:
            # Note: one could make a difference between term to match and occurrence here too!!!
            y[1].append(x)

    def match(self, x):
        print("matching: " + str(x))
        y = self.funs.get(x.getSig())
        if y:
            for term in y[0].get(x, []):
                print("  matched with " + str(term) + " with empty substitution")
            for term in y[1]:
                subst = term.match(x)
                if subst != None:
                    print("  matched with " + str(term) + " with subst: " + str(subst))

    def unify(self, x):
        print("unifying: " + str(x))
        z = x.eval()
        if z: self.match(z)
        else:
            y = self.funs.get(x.getSig())
            if y:
                for val, terms in y[0].items():
                    if x.match(val):
                        for term in terms:
                            print("  matched with " + str(term))
                for term in y[1]:
                    subst = x.unify(term)
                    if subst != None:
                        print("  unified with " + str(term) + " with subst: " + str(subst))

    def __repr__(self):
        return str(self.funs)

# }}}
# {{{ tests

l = Lookup()
l.add(Occurrence(FunTerm("p", [FunTerm("f", [VarTerm("X"), VarTerm("Y")]), VarTerm("Z")])))
l.add(Occurrence(FunTerm("p", [FunTerm("g", [VarTerm("X"), VarTerm("Y")]), VarTerm("Z")])))
l.add(Occurrence(FunTerm("p", [VarTerm("X"), VarTerm("Y")])))
l.add(Occurrence(FunTerm("p", [VarTerm("X"), VarTerm("X")])))
l.add(Occurrence(FunTerm("p", [LinearTerm("X", 3, 7)])))
print(l)

l.match(FunVal("p", [FunVal("f", [NumVal(1), NumVal(2)]), NumVal(3)]))
l.match(FunVal("p", [FunVal("f", [NumVal(1), NumVal(2)]), FunVal("f", [NumVal(1), NumVal(2)])]))
l.match(FunVal("p", [NumVal(4)]))

l.unify(Occurrence(FunTerm("p", [FunTerm("g", [VarTerm("A"), VarTerm("B")]), VarTerm("C")])))
l.unify(Occurrence(FunTerm("p", [VarTerm("A"), VarTerm("B")])))
l.unify(Occurrence(FunTerm("p", [VarTerm("A")])))

# }}}

