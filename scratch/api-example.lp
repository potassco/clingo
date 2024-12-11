#script (python)

from gringo import *

# (ground) terms usable as return values
# in functions called during grounding
terms = []
terms.append(42)              # a number
terms.append("hello")         # a string
terms.append(Fun("world"))     # a symbolic constant
terms.append(tuple(terms))    # a tuple
terms.append(Fun("f", terms)) # a function symbol

# functions on ground terms
print terms[2].name()
print terms[4].name()
print terms[4].args()
# compare functions according to gringo's inbuilt term order
print cmp(terms[1], terms[2]),cmp(terms[2], terms[1]), cmp(terms[1], terms[1])

def someTerm(i):
    return terms[i]

def someTerms():
    return terms

def onModel(m):
    # gringo uses ground terms to refer to atoms
    # because function symbols and symbolic constants
    # are structurally equivalent

    # prints a model in a similar fashion to clasp
    print "answer:  {0}".format(m)

    # print all shown atoms and terms and CSP assignments in the answer set
    print "default: {0}".format(m.atoms())
    print "shown:   {0}".format(m.atoms(Model.SHOWN)) # equivalent to the above

    # print all atoms in the answer set (including hidden atoms)
    print "atoms:   {0}".format(m.atoms(Model.ATOMS))

    # print all shown terms the answer set
    print "terms:   {0}".format(m.atoms(Model.TERMS))

    # print all csp assignments in the answer set (including hidden assignments)
    print "csp:     {0}".format(m.atoms(Model.CSP))

    # check if an atom appears in an answer set
    print "answer contains a: {0}".format(m.contains(Fun("a")))
    print "answer contains b: {0}".format(m.contains(Fun("b")))

    # optionally can return a boolean to indicate
    # whether solving should be continued
    return True

def onFinish(ret, interrupted):
    # ret holds the SolveResult (see below)
    # interrupted indicates whether the search was interrupted
    print "solving finished!"

# if there is a main clingo will execute this function
# instead of grounding and solving the base part
# the argument can be used to control grounding and solving
# (it must only be used in the context of the main function)
def main(prg):
    # (instructs to) ground the program with a given name
    # and corresponding parameters
    # note that the actual grounding happens during solve
    prg.ground("base", [])
    # fix the truth value of an external atom
    # (this can be changed as long as the external is not defined or forgotten)
    prg.assignExternal(Fun("b"), True)
    # solve what was grounded previously
    # accepts an optional callback to intercept models
    # returns either SolveResult.UNSAT, SolveResult.SAT, or SolveResult.UNKNOWN
    ret = prg.solve(onModel)
    print ret

    # get a constant defined with #const
    # (returns None if the constant is not defined)
    n = prg.getConst("n")
    # add an arbitrary program
    prg.add("add", ["t"], "someTerm(t).")
    # and ground it
    prg.ground("add", [n])
    # forget about an external
    prg.releaseExternal(Fun("b"))
    # solve asynchronously
    # takes an additional (optional) callback
    # that signals when solve finished
    # returns an object to interrupt and get the solving result
    f = prg.asolve(onModel, onFinish)
    f.wait(10.5)    # wait for 10.5 seconds for solve to finish
    f.wait()        # wait until solve finishes
    f.interrupt(  ) # interrupt the current solve call
    print f.get()   # get the result as for solve (possibly waiting till it is there)

#end.

#const n = 23.

#program base.

#external b.

a.
{c}.
someTerm(@someTerm(0)).
someTerms(@someTerms()).

$x$=1.
$y$=1.

#show a/0.
#show x.
#show $y.
