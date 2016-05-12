#script (python)

import json

def main(prg):
    prg.ground([("base", [])])
    prg.solve()
    print json.dumps(prg.stats, sort_keys=True, indent=4, separators=(',', ': '))
    print "lp.bodies", prg.stats["lp"]["bodies"]
    print "optimization", prg.stats["costs"]

#end.


