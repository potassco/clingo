import os

from clingo import ast
from clingo.core import Library


def rewrite(lib, stms):
    stms_res = []
    stms_other = []
    params_const = []
    for stm in stms:
        if isinstance(stm, ast.StatementConst):
            params_const.append(stm.name)
            stms_res.append(stm)
        else:
            stms_other.append(stm)

    params = params_const
    for stm in stms_other:
        if isinstance(stm, ast.StatementProgram):
            params = params_const + stm.arguments
            stms_res.append(stm)
        else:
            stms_res.extend(ast.rewrite_statement(lib, stm, parameters=params))

    return stms_res


def dependency(stms):
    # TODO: add edges from body to head
    # {a} :- b.
    #   a :- b, not not a.
    #   for choice rules add edge (a, a, -)
    # {a: c} :- b.
    #   a :- b, c, not not a.
    #   for positive literal in condition add (a, c, +)
    # {a: c} > 1 :- b.
    #   {a: c} :- b.
    #   :- b, not {a: c} > 1.
    #   guards of head aggregates can be ignored
    pass


def run():
    with Library() as lib:
        files = [os.path.join(os.path.dirname(__file__), "example.lp")]
        with ast.Scanner(lib, files) as scanner:
            stms = list(scanner)

        stms = rewrite(lib, stms)

        for stm in stms:
            print(stm)


run()
