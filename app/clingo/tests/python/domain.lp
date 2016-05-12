p(1).
{ p(3) }. 
#external p(1..3).

q(X) :- p(X).

#script (python)

import gringo

def main(prg):
    prg.ground([("base", [])])
    print "Solving..."
    print "Answer: 1"
    out = "universe(" + str(len(prg.domains)) + ")"
    for x in prg.domains:
        out = out + " domain(" + str(x.atom) + "," + str(x.is_fact).lower() + "," + str(x.is_external).lower() + ")"
    out = out + " in_domain(p(2)," + str(prg.domains[gringo.Fun("p", [3])] is not None).lower() + ")"
    out = out + " in_domain(p(4)," + str(prg.domains[gringo.Fun("p", [4])] is not None).lower() + ")"
    for x in prg.domains.by_signature("p", 1):
        out = out + " domain_of_p(" + str(x.atom) + "," + str(x.is_fact).lower() + "," + str(x.is_external).lower() + ")"
    for x in prg.domains.signatures():
        out = out + " sig(" + x[0] + "," + str(x[1]) + ")"
    print (out)

#end. 
