#script (python)

def main(prg):
    prg.ground("base", [])
    prg.setConf("-n 0 -e record")
    print("enumerate 4 solutions")
    prg.solve()
    prg.enableEnumAssumption(False)
    print("enumerate 4 solutions")
    prg.solve()
    print("enumerate 0 solutions")
    prg.solve()

#end.
{ a; b }.
