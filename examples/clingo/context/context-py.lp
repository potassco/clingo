#script (python) 

class State:
    def __init__(self, value):
        self.value = value

    def test(self, value):
        return self.value + value
    
    def __on_model(self, m):
        print (m)

    def run(self, prg):
        prg.ground([("base", [])], self)
        prg.solve(None, self.__on_model)
        
def main(prg):
    x = State(21)
    x.run(prg)

#end.
