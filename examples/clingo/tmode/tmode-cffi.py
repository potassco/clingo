from sys import stdout, exit
from copy import copy
from collections.abc import Sequence

from clingo.application import Application
from clingo import SymbolType, Number, String, Function, ast, clingo_main

class Transformer:
    def visit_children(self, x, *args, **kwargs):
        update = []
        for key in x.child_keys:
            old = getattr(x, key)
            new = self.visit(old, *args, **kwargs)
            if new is not old:
                update.append((key, new))
        if update:
            ret = copy(x)
            for key, new in update:
                setattr(ret, key, new)
        else:
            ret = x
        return x

    def visit(self, x, *args, **kwargs):
        if isinstance(x, ast.AST):
            attr = "visit_" + str(x.ast_type).replace('ASTType.', '')
            if hasattr(self, attr):
                return getattr(self, attr)(x, *args, **kwargs)
            else:
                return self.visit_children(x, *args, **kwargs)
        elif isinstance(x, Sequence):
            ret, lst = x, []
            for old in x:
                lst.append(self.visit(old, *args, **kwargs))
                if lst[-1] is not old:
                    ret = lst
            return ret
        elif x is None:
            return x
        else:
            raise TypeError("unexpected type")

class TermTransformer(Transformer):
    def __init__(self, parameter):
        self.parameter = parameter

    def __get_param(self, name, location):
        n = name.replace('\'', '')
        primes = len(name) - len(n)
        param = ast.SymbolicTerm(location, self.parameter)
        if primes > 0:
            param = ast.BinaryOperation(location, ast.BinaryOperator.Minus, param, ast.SymbolicTerm(location, Number(primes)))
        return (n, param)

    def visit_Function(self, term):
        term.name, param = self.__get_param(term.name, term.location)
        term.arguments.append(param)
        return term

    def visit_SymbolicTerm(self, term):
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
        if self.final and isinstance(ret, ast.AST) and hasattr(ret, "body"):
            loc = ret.location
            ret.body.append(ast.Literal(loc, ast.Sign.NoSign, ast.SymbolicAtom(ast.Function(loc, "finally", [ast.SymbolicTerm(loc, self.parameter)], False))));
        return ret

    def visit_SymbolicAtom(self, atom):
        atom.symbol = self.term_transformer.visit(atom.symbol)
        return atom

    def visit_Program(self, prg):
        self.final = prg.name == "final"
        if self.final:
            prg.name = "static"
        prg.parameters.append(ast.Id(prg.location, self.parameter.name))
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
    imin   = get(prg.get_const("imin"), Number(0))
    imax   = prg.get_const("imax")
    istop  = get(prg.get_const("istop"), String("SAT"))

    step, ret = 0, None
    while ((imax is None or step < imax.number) and
           (step == 0 or step < imin.number or (
              (istop.string == "SAT"     and not ret.satisfiable) or
              (istop.string == "UNSAT"   and not ret.unsatisfiable) or
              (istop.string == "UNKNOWN" and not ret.unknown)))):
        parts = []
        parts.append(("base", [Number(step)]))
        parts.append(("static", [Number(step)]))
        if step > 0:
            prg.release_external(Function("finally", [Number(step-1)]))
            parts.append(("dynamic", [Number(step)]))
        else:
            parts.append(("initial", [Number(0)]))
        prg.ground(parts)
        prg.assign_external(Function("finally", [Number(step)]), True)
        ret, step = prg.solve(), step+1

class TModeApp(Application):
    def print_model(self, model, printer):
        table = {}
        for sym in model.symbols(shown=True):
            if sym.type == SymbolType.Function and len(sym.arguments) > 0:
                table.setdefault(sym.arguments[-1], []).append(Function(sym.name, sym.arguments[:-1]))
        for step, symbols in sorted(table.items()):
            stdout.write(" State {}:".format(step))
            sig = None
            for sym in sorted(symbols):
                if (sym.name, len(sym.arguments)) != sig:
                    stdout.write("\n ")
                    sig = (sym.name, len(sym.arguments))
                stdout.write(" {}".format(sym))
            stdout.write("\n")

    def main(self, ctl, files):
        with ast.ProgramBuilder(ctl) as bld:
            ptf = ProgramTransformer(Function("__t"))
            ast.parse_files(files, lambda stm: bld.add(ptf.visit(stm)))
        ctl.add("initial", ["t"], "initially(t).")
        ctl.add("static", ["t"], "#external finally(t).")
        imain(ctl)

exit(clingo_main(TModeApp()))
