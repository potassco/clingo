from typing import Callable, Container, List, Optional, Sequence, cast
from pprint import pprint
from clingo.ast import AST, Transformer, parse_string

class Extractor(Transformer):
    '''
    Simple visitor returning the first theory term in a program.
    '''
    # pylint: disable=invalid-name
    atom: Optional[AST]

    def __init__(self):
        self.atom = None

    def visit_TheoryAtom(self, x: AST):
        '''
        Extract theory atom.
        '''
        self.atom = x
        return x

def last_stm(s: str) -> AST:
    """
    Convert string to rule.
    """
    v = Extractor()
    stm = None

    def set_stm(x):
        nonlocal stm
        stm = x
        v(stm)

    parse_string(s, set_stm)

    return cast(AST, stm)


pprint(repr(last_stm('a := &sum{ p(X) }.')), indent=4)
print()
pprint(repr(last_stm('''
#theory t {
    group { };
    &a/0 : group, directive
}.

&a{}.''')), indent=4)
