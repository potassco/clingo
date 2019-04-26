import clingo

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

    @property
    def statistics(self):
        return self.ctl.statistics

    @property
    def _to_c(self):
        return self.ctl._to_c

