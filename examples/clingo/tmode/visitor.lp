#script (python)

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
        self.parameter = parameter
        self.term_transformer = TermTransformer(parameter)

    def visit_SymbolicAtom(self, atom):
        atom.term = self.term_transformer.visit(atom.term)
        return atom

    def visit_Program(self, prg):
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
        ret, step = prg.solve(), step+1

def main(prg):
    with prg.builder() as b:
        t = ProgramTransformer(clingo.Function("__t"))
        clingo.parse_program(
            open("example.lp").read(),
            lambda stm: b.add(t.visit(stm)))
    imain(prg)

#end.

#program static(t).
#external finally(t).
