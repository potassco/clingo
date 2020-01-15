"""
Basic functions to run tests.
"""

import clingo
import csp


def parse_model(model, prp):
    """
    Combine model and assignment in one list.
    """
    m = []
    for sym in model.symbols(shown=True):
        m.append(str(sym))
    a = []
    for var, val in prp.get_assignment(model.thread_id):
        if not var.startswith("_"):
            a.append((var, val))

    return list(sorted(m)), list(sorted(a))


def solve(s, minint=-20, maxint=20):
    """
    Return the models/assignments of the program in the given string.
    """

    csp.MIN_INT = minint
    csp.MAX_INT = maxint

    prp = csp.Propagator()
    prg = clingo.Control(['0'], message_limit=0)
    prg.register_propagator(prp)

    prg.add("base", [], csp.THEORY)
    with prg.builder() as b:
        csp.transform(b, s)

    prg.ground([("base", [])])

    ret = []
    prg.solve(on_model=lambda m: ret.append(parse_model(m, prp)))

    return [a + b for a, b in list(sorted(ret))]
