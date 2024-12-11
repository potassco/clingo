#script (python)

def tostring(self, prefix):
    ret = prefix + str(self["term"])
    if len(self["children"]) > 0:
        ret += ":\n"
        for child in self["children"]:
            ret += tostring(child, prefix + "  ");
    else:
        ret += "\n"
    return ret

def main(prg):
    x = prg.parse( """
p(1).
p(1,2).
q(1,3).
a :- p(X), 1 #sum { Z : p(X,Z); X,Y : q(X,Y) }.
""");

    for ast in x:
        print tostring(ast, "")
#end.
