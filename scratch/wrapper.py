import clingo
import yaml
import threading
import os

class WrappedPropagateControl(object):
    def __init__(self, control, calls):
        self.control = control
        self.calls = calls

    def add_literal(self):
        call = {}
        call["name"] = "add_literal"
        call["args"] = []
        self.calls.append(call)
        return self.control.add_literal()

    def add_clause(self, lits, tag=False, lock=False):
        lits = list(lits)
        call = {}
        call["name"] = "add_clause"
        call["args"] = [lits[:], tag, lock]
        self.calls.append(call)
        if isinstance(self.control, clingo.PropagateInit):
            ret = self.control.add_clause(lits)
        else:
            ret = self.control.add_clause(lits, tag, lock)
        call["ret"] = ret
        return ret

    def add_watch(self, lit):
        call = {}
        call["name"] = "add_watch"
        call["args"] = [lit]
        self.calls.append(call)
        return self.control.add_watch(lit)

    def propagate(self):
        call = {}
        call["name"] = "propagate"
        call["args"] = []
        self.calls.append(call)
        ret = self.control.propagate()
        call["ret"] = ret
        return ret

    def __getattribute__(self, name):
        dc = object.__getattribute__(WrappedPropagateControl, "__dict__")
        do = object.__getattribute__(self, "__dict__")
        if name in dc or name in do:
            return object.__getattribute__(self, name)
        return getattr(self.control, name)


class WrappedBackend:
    def __init__(self, ctl):
        self.script = ctl.script
        self.backend = ctl.ctl.backend()

    def __enter__(self, *args, **kwargs):
        self.script.write("with ctl.backend() as b:\n")
        self.backend.__enter__(*args, **kwargs)
        return self

    def add_atom(self, symbol=None):
        self.script.write("    b.add_atom(clingo.parse_term({}))".format(repr(str(symbol))))
        self.script.flush()
        ret = self.backend.add_atom(symbol)
        self.script.write(" # {}\n".format(ret))
        self.script.flush()
        return ret

    def add_rule(self, head, body=[], choice=False):
        self.script.write("    b.add_rule({}, {}, {})\n".format(head, body, choice))
        self.script.flush()
        return self.backend.add_rule(head, body, choice)

    def __exit__(self, *args, **kwargs):
        return self.backend.__exit__(*args, **kwargs)


class WrappedControl:
    def __init__(self, args=[]):
        self.script = open("replay.py", "w")
        self.prefix = "file_"
        self.files = 0
        self.ctl = clingo.Control(args)
        self.script.write("import clingo\n")
        self.script.write("ctl = clingo.Control({})\n".format(repr(args)))
        self.script.flush()
        self.g_trace = []

    def backend(self):
        return WrappedBackend(self)

    def load(self, path):
        self.files += 1
        name = "{}{}.lp".format(self.prefix, self.files)
        open(name, "w").write(open(path).read())
        self.script.write("ctl.load({})\n".format(repr(name)))
        self.script.flush()
        self.ctl.load(name)

    def ground(self, parts, context=None):
        self.script.write("ctl.ground({})\n".format(repr(parts)))
        self.script.flush()
        self.ctl.ground(parts, context)

    def solve(self, assumptions=[], on_model=None, on_finish=None, yield_=False, async_=False):
        self.script.write("ctl.solve({}).get()\n".format(repr(assumptions)))
        self.script.flush()
        return self.ctl.solve(assumptions, on_model, on_finish, yield_, async_)

    def wrap_init(self, init):
        calls = []
        trace = {}
        trace["state"] = "init"
        trace["thread_id"] = 0
        trace["calls"] = calls
        self.g_trace.append(trace)
        return WrappedPropagateControl(init, calls)

    def wrap_control(self, control, where):
        calls = []
        trace = {}
        trace["state"] = where
        trace["thread_id"] = control.thread_id
        trace["calls"] = calls
        self.g_trace.append(trace)
        return WrappedPropagateControl(control, calls)

    def write_trace(self):
        txt = yaml.dump(self.g_trace, indent=2)
        with open("new-trace.yml", "w") as f:
            f.write(txt)
        try:
            a = os.path.getsize("trace.yml")
            b = os.path.getsize("new-trace.yml")
            if b < a:
                with open("trace.yml", "w") as f:
                    f.write(txt)
        except:
            with open("trace.yml", "w") as f:
                f.write(txt)

    @property
    def statistics(self):
        return self.ctl.statistics

    @property
    def _to_c(self):
        return self.ctl._to_c


class Retracer:
    def __init__(self):
        self.trace = yaml.load(open("trace.yml").read(), Loader=yaml.FullLoader)
        self.cv = threading.Condition()

    def init(self, init):
        init.check_mode = clingo.PropagatorCheckMode.Fixpoint
        self.run_trace(init, 0, "init")
        '''
        for trace in self.trace:
            print('trace.emplace_back("{}", {}, CallVec{{}});'.format(trace["state"], trace["thread_id"]))
            for call in trace["calls"]:
                if call["args"] and isinstance(call["args"][0], list):
                    args = map(str, call["args"][0])
                else:
                    args = map(str, call["args"])
                print('std::get<2>(trace.back()).emplace_back("{}",LitVec{{{}}});'.format(call["name"], ",".join(args)))
        '''

    def match(self, thread_id, where):
        if not self.trace:
            return True
        top = self.trace[0]
        return top["state"] == where and top["thread_id"] == thread_id

    def run_trace(self, control, thread_id, where):
        self.cv.acquire()
        print("WAIT: ", thread_id, where)
        while not self.match(thread_id, where):
            self.cv.wait()

        print("  START: ", thread_id, where)

        if self.trace:
            top = self.trace[0]

            for call in top["calls"]:
                if call["name"] == "propagate" or call["name"] == "add_clause":
                    print("    CALL", "{}({})".format(call["name"], ",".join(map(str, call["args"]))), "EXPECTING", call["ret"])
                    ret = getattr(control, call["name"])(*call["args"])
                    print("      RESULT", ret)
                else:
                    print("    CALL", call["name"])
                    getattr(control, call["name"])(*call["args"])

            self.trace.pop(0)

        if self.trace:
            print("  NEXT: ", self.trace[0]["state"], self.trace[0]["thread_id"])

        self.cv.notify_all()
        self.cv.release()

    def propagate(self, control, changes):
        self.run_trace(control, control.thread_id, "propagate")

    def check(self, control):
        self.run_trace(control, control.thread_id, "check")


if __name__ == "__main__":
    ctl = clingo.Control(["-t3", "0"])
    ctl.register_propagator(Retracer())

    print("============ STEP 1 ============")
    ctl.add("step1", [], """\
#theory cp {
    sum_term { };
    &minimize/0 : sum_term, directive
}.

&minimize { x }.
""")
    ctl.ground([("step1", [])])
    n = 0
    for m in ctl.solve(yield_=True):
        n += 1
        print("--- Found model[{}] ---".format(m.thread_id))
    print("MODELS", n)
    print()

    print("============ STEP 2 ============")
    ctl.add("step2", [], "")
    ctl.ground([("step2", [])])
    n = 0
    for m in ctl.solve(yield_=True):
        n += 1
        print("--- Found model[{}] ---".format(m.thread_id))
    print("MODELS", n)
