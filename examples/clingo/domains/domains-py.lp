#script (python)

import gringo

def main(prg):
    prg.ground([("base", [])])
    print ("universe: " + str(len(prg.domains)))
    for x in prg.domains:
        print (x.atom, x.is_fact, x.is_external)
    print ("p(2) is in domain: " + str(prg.domains[gringo.Fun("p", {3})] is not None))
    print ("p(4) is in domain: " + str(prg.domains[gringo.Fun("p", {4})] is not None))
    print ("domain of p/1")
    for x in prg.domains.by_signature("p", 1):
        print (x.atom, x.is_fact, x.is_external)
    print ("signatures")
    for x in prg.domains.signatures():
        print (x[0] + "/" + str(x[1]))

#end. 
