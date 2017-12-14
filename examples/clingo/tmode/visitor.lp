#script (python)

import sys
import clingo
import clingo.ast

class Transformer:
    def visit_children(self, x, *args, **kwargs):
        for key in x.child_keys:
            setattr(x, key, self.visit(getattr(x, key), *args, **kwargs))
        return x

    def visit(self, x, *args, **kwargs):
        if isinstance(x, clingo.ast.AST):
            attr = "visit_" + str(x.type)
            if hasattr(self, attr):
                return getattr(self, attr)(x, *args, **kwargs)
            else:
                return self.visit_children(x, *args, **kwargs)
        elif isinstance(x, list):
            return [self.visit(y, *args, **kwargs) for y in x]
        elif x is None:
            return x
        else:
            raise TypeError("unexpected type")

class TermTransformer(Transformer):
    def __init__(self, parameter):
        self.parameter = parameter

    def __get_param(self, name, location):
        n = name.translate(None, '\'')
        primes = len(name) - len(n)
        param = clingo.ast.Symbol(location, self.parameter)
        if primes > 0:
            param = clingo.ast.BinaryOperation(location, clingo.ast.BinaryOperator.Minus, param, clingo.ast.Symbol(location, primes))
        return (n, param)

    def visit_Function(self, term):
        term.name, param = self.__get_param(term.name, term.location)
        term.arguments.append(param)
        return term

    def visit_Symbol(self, term):
        # this function is not necessary if gringo's parser is used
        # but this case could occur in a valid AST
        raise RuntimeError("not implemented")

class ProgramTransformer(Transformer):
    def __init__(self, parameter):
        self.final = False
        self.parameter = parameter
        self.term_transformer = TermTransformer(parameter)

    def visit(self, x, *args, **kwargs):
        ret = Transformer.visit(self, x, *args, **kwargs)
        if self.final and isinstance(x, clingo.ast.AST) and hasattr(x, "body"):
            loc = x.location
            x.body.append(clingo.ast.Literal(loc, clingo.ast.Sign.NoSign, clingo.ast.SymbolicAtom(clingo.ast.Function(loc, "finally", [clingo.ast.Symbol(loc, self.parameter)], False))));
        return ret

    def visit_SymbolicAtom(self, atom):
        atom.term = self.term_transformer.visit(atom.term)
        return atom

    def visit_Program(self, prg):
        self.final = prg.name == "final"
        if self.final:
            prg.name = "static"
        prg.parameters.append(clingo.ast.Id(prg.location, self.parameter.name))
        return prg

    def visit_ShowSignature(self, sig):
        sig.arity += 1
        return sig

    def visit_ProjectSignature(self, sig):
        sig.arity += 1
        return sig

def get(val, default):
    return val if val != None else default

def on_model(model):
    table = {}
    for sym in model.symbols(shown=True):
        if sym.type == clingo.SymbolType.Function and len(sym.arguments) > 0:
            table.setdefault(sym.arguments[-1], []).append(clingo.Function(sym.name, sym.arguments[:-1]))
    sys.stdout.write("Answer: {}\n".format(model.number))
    for step, symbols in sorted(table.items()):
        sys.stdout.write(" State {}:".format(step))
        sig = None
        for sym in sorted(symbols):
            if (sym.name, len(sym.arguments)) != sig:
                sys.stdout.write("\n ")
                sig = (sym.name, len(sym.arguments))
            sys.stdout.write(" {}".format(sym))
        sys.stdout.write("\n".format(step))

def imain(prg):
    imin   = get(prg.get_const("imin"), clingo.Number(0))
    imax   = prg.get_const("imax")
    istop  = get(prg.get_const("istop"), clingo.String("SAT"))

    step, ret = 0, None
    while ((imax is None or step < imax.number) and
           (step == 0 or step < imin.number or (
              (istop.string == "SAT"     and not ret.satisfiable) or
              (istop.string == "UNSAT"   and not ret.unsatisfiable) or
              (istop.string == "UNKNOWN" and not ret.unknown)))):
        parts = []
        parts.append(("base", [step]))
        parts.append(("static", [step]))
        if step > 0:
            prg.release_external(clingo.Function("finally", [step-1]))
            parts.append(("dynamic", [step]))
            prg.cleanup()
        else:
            parts.append(("initial", [0]))
        prg.ground(parts)
        prg.assign_external(clingo.Function("finally", [step]), True)
        ret, step = prg.solve(on_model=on_model), step+1

def main(prg):
    with prg.builder() as b:
        t = ProgramTransformer(clingo.Function("__t"))
        clingo.parse_program(
            open("example.lp").read(),
            lambda stm: b.add(t.visit(stm)))
    imain(prg)

#end.

#program initial(t).
initially(t).

#program static(t).
#external finally(t).
