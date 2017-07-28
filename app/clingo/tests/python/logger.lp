#script (python)

import clingo

messages = []

def log(code, message):
    messages.append((str(code), message.replace(" ", "_")))

def main(prg):
    ctl = clingo.Control(logger=log, message_limit=2)
    ctl.add("base", [], ":- a. :- b. :- c.")
    ctl.ground([("base", [])])

    prg.ground([("base", [])])
    prg.solve()

def msg():
    return messages

#end.

msg(X,Y) :- (X, Y) = @msg().
