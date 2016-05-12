#script (python) 

import socket
import os
import errno
from gringo import Fun, SolveResult

class Receiver:
    def __init__(self, conn):
        self.conn = conn
        self.data = bytearray()
    def readline(self):
        pos = self.data.find("\n")
        while pos < 0:
            while True:
                try: self.data.extend(self.conn.recv(4096))
                except socket.error as (code, msg):
                    if code != errno.EINTR: raise
                else: break
            pos = self. data.find("\n")
        msg = self.data[:pos]
        self.data = self.data[pos+1:]
        return msg

class States:
    SOLVE = 1
    IDLE  = 2

def main(prg):
    with open(".controller.PORT", "r") as f:
        p = int(f.read())
    os.remove(".controller.PORT")
    conn = socket.create_connection(("127.0.0.1", p))
    try:
        recv  = Receiver(conn)
        state = States.IDLE
        k     = 0
        prg.ground([("pigeon", []), ("sleep",  [k])])
        prg.assign_external(Fun("sleep", [k]), True)
        while True:
            if state == States.SOLVE:
                f = prg.solve_async(
                    on_model  = lambda model:            conn.sendall("Answer: " + str(model) + "\n"),
                    on_finish = lambda ret, interrupted: conn.sendall("finish:" + str(ret) + (":INTERRUPTED" if interrupted else "") + "\n"))
            msg = recv.readline()
            if state == States.SOLVE:
                f.cancel()
                ret = f.get()
            else:
                ret = SolveResult.UNKNOWN
            if msg == "interrupt":
                state = States.IDLE
            elif msg == "exit":
                return
            elif msg == "less_pigeon_please":
                prg.assign_external(Fun("p"), False)
                state = States.IDLE
            elif msg == "more_pigeon_please":
                prg.assign_external(Fun("p"), True)
                state = States.IDLE
            elif msg == "solve":
                state = States.SOLVE
            else: raise(RuntimeError("unexpected message: " + msg))
            if ret != SolveResult.UNKNOWN:
                k = k + 1
                prg.ground([("sleep", [k])])
                prg.release_external(Fun("sleep", [k-1]))
                prg.assign_external(Fun("sleep", [k]), True)
    finally:
        conn.close()
#end.
