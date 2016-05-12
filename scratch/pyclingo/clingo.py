import ctypes

# {{{1 Private

_clingo = ctypes.cdll.LoadLibrary("libcclingo.so")

class _VALUE(ctypes.Structure):
    _fields_ = [("a", ctypes.c_uint)
               ,("b", ctypes.c_uint)]
class _VALUE_SPAN(ctypes.Structure):
    _fields_ = [("data", ctypes.POINTER(_VALUE))
               ,("size", ctypes.c_uint)]

class _SYMBOLIC_LITERAL(ctypes.Structure):
    _fields_ = [("atom", _VALUE)
               ,("sign", ctypes.c_int)]
class _SYMBOLIC_LITERAL_SPAN(ctypes.Structure):
    _fields_ = [("data", ctypes.POINTER(_SYMBOLIC_LITERAL))
               ,("size", ctypes.c_uint)]

class _PART(ctypes.Structure):
    _fields_ = [("name", ctypes.c_char_p)
               ,("params", _VALUE_SPAN)]
class _PART_SPAN(ctypes.Structure):
    _fields_ = [("data", ctypes.POINTER(_PART))
               ,("size", ctypes.c_uint)]

class _LOCATION(ctypes.Structure):
    _fields_ = [("begin_file", ctypes.c_char_p)
               ,("end_file", ctypes.c_char_p)
               ,("begin_line", ctypes.c_size_t)
               ,("end_line", ctypes.c_size_t)
               ,("begin_column", ctypes.c_size_t)
               ,("end_column", ctypes.c_size_t)]

class _AST(ctypes.Structure):
    pass
class _AST_SPAN(ctypes.Structure):
    _fields_ = [("data", ctypes.POINTER(_AST))
               ,("size", ctypes.c_uint)]
_AST._fields_ = [("location", _LOCATION)
               ,("value", _VALUE)
               ,("children", _AST_SPAN)]

_ONMODEL_FUNC = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)
_CALL_FUNC = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p, _VALUE_SPAN, ctypes.c_void_p, ctypes.POINTER(_VALUE_SPAN))
_PARSE_CALLBACK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(_AST), ctypes.c_void_p)
_ADD_AST_CALLBACK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.POINTER(_AST)), ctypes.c_void_p)

# {{{2 error

#char const *clingo_error_what(clingo_error_t err);
_clingo.clingo_error_str.restype = ctypes.c_char_p
_clingo.clingo_error_str.argtypes = [ctypes.c_int]

# {{{2 value

#void clingo_value_new_num(int num, clingo_value_t *val);
_clingo.clingo_value_new_num.restype = None
_clingo.clingo_value_new_num.argtypes = [ctypes.c_int, ctypes.POINTER(_VALUE)]
#void clingo_value_new_sup(clingo_value_t *val);
_clingo.clingo_value_new_sup.restype = None
_clingo.clingo_value_new_sup.argtypes = [ctypes.POINTER(_VALUE)]
#void clingo_value_new_inf(clingo_value_t *val);
_clingo.clingo_value_new_inf.restype = None
_clingo.clingo_value_new_inf.argtypes = [ctypes.POINTER(_VALUE)]
#clingo_error_t clingo_value_new_str(char const *str, clingo_value_t *val);
_clingo.clingo_value_new_str.restype = ctypes.c_int
_clingo.clingo_value_new_str.argtypes = [ctypes.c_char_p, ctypes.POINTER(_VALUE)]
#clingo_error_t clingo_value_new_id(char const *id, int sign, clingo_value_t *val);
_clingo.clingo_value_new_id.restype = ctypes.c_int
_clingo.clingo_value_new_id.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(_VALUE)]
#clingo_error_t clingo_value_new_fun(char const *name, clingo_value_span_t args, int sign, clingo_value_t *val);
_clingo.clingo_value_new_fun.restype = ctypes.c_int
_clingo.clingo_value_new_fun.argtypes = [ctypes.c_char_p, _VALUE_SPAN, ctypes.c_int, ctypes.POINTER(_VALUE)]
#int clingo_value_num(clingo_value_t val);
_clingo.clingo_value_num.restype = ctypes.c_int
_clingo.clingo_value_num.argtypes = [_VALUE]
#char const *clingo_value_name(clingo_value_t val);
_clingo.clingo_value_name.restype = ctypes.c_char_p
_clingo.clingo_value_name.argtypes = [_VALUE]
#char const *clingo_value_str(clingo_value_t val);
_clingo.clingo_value_str.restype = ctypes.c_char_p
_clingo.clingo_value_str.argtypes = [_VALUE]
#int clingo_value_sign(clingo_value_t val);
_clingo.clingo_value_sign.restype = ctypes.c_int
_clingo.clingo_value_sign.argtypes = [_VALUE]
#clingo_value_span_t clingo_value_args(clingo_value_t val);
_clingo.clingo_value_args.restype = _VALUE_SPAN
_clingo.clingo_value_args.argtypes = [_VALUE]
#clingo_value_type_t clingo_value_type(clingo_value_t val);
_clingo.clingo_value_type.restype = ctypes.c_int
_clingo.clingo_value_type.argtypes = [_VALUE]
#int clingo_value_eq(clingo_value_t a, clingo_value_t b);
_clingo.clingo_value_eq.restype = ctypes.c_int
_clingo.clingo_value_eq.argtypes = [_VALUE, _VALUE]
#int clingo_value_lt(clingo_value_t a, clingo_value_t b);
_clingo.clingo_value_lt.restype = ctypes.c_int
_clingo.clingo_value_lt.argtypes = [_VALUE, _VALUE]
#clingo_error_t clingo_value_to_string(clingo_value_t val, char **str);
_clingo.clingo_value_to_string.restype = ctypes.c_int
_clingo.clingo_value_to_string.argtypes = [_VALUE, ctypes.POINTER(ctypes.c_char_p)]

# {{{2 module

#clingo_error_t clingo_module_new(clingo_module_t **mod);
_clingo.clingo_module_new.restype = ctypes.c_int
_clingo.clingo_module_new.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
#void clingo_module_free(clingo_module_t *mod);
_clingo.clingo_module_free.restype = None
_clingo.clingo_module_free.argtypes = [ctypes.c_void_p]

# {{{2 solve iter
#clingo_error_t clingo_solve_iter_next(clingo_solve_iter_t *it, clingo_model **m);
_clingo.clingo_solve_iter_next.restype = ctypes.c_int
_clingo.clingo_solve_iter_next.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
#clingo_error_t clingo_solve_iter_get(clingo_solve_iter_t *it, clingo_solve_result_t *ret);
_clingo.clingo_solve_iter_get.restype = ctypes.c_int
_clingo.clingo_solve_iter_get.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int)]
#clingo_error_t clingo_solve_iter_close(clingo_solve_iter_t *it);
_clingo.clingo_solve_iter_close.restype = ctypes.c_int
_clingo.clingo_solve_iter_close.argtypes = [ctypes.c_void_p]

# {{{2 model

#int clingo_model_contains(clingo_model_t *m, clingo_value_t atom);
_clingo.clingo_model_contains.restype = ctypes.c_int
_clingo.clingo_model_contains.argtypes = [ctypes.c_void_p, _VALUE]
#clingo_error_t clingo_model_atoms(clingo_model_t *m, unsigned show, clingo_value_span_t *ret);
_clingo.clingo_model_atoms.restype = ctypes.c_int
_clingo.clingo_model_atoms.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(_VALUE_SPAN)]

# {{{2 control

#clingo_error_t clingo_control_new(clingo_module_t *mod, int argc, char const **argv, clingo_control_t **);
_clingo.clingo_control_new.restype = ctypes.c_int
_clingo.clingo_control_new.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_void_p)]
#void clingo_control_free(clingo_control_t *ctl);
_clingo.clingo_control_free.restype = None
_clingo.clingo_control_free.argtypes = [ctypes.c_void_p]
#clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const **params, char const *part);
_clingo.clingo_control_add.restype = ctypes.c_int
_clingo.clingo_control_add.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_char_p]
#clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_span_t vec, clingo_ground_callback_t *cb, void *data);
_clingo.clingo_control_ground.restype = ctypes.c_int
_clingo.clingo_control_ground.argtypes = [ctypes.c_void_p, _PART_SPAN, _CALL_FUNC, ctypes.c_void_p]
#clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_symbolic_literal_span_t *assumptions, clingo_model_handler_t *mh, void *data, clingo_solve_result_t *ret);
_clingo.clingo_control_solve.restype = ctypes.c_int
_clingo.clingo_control_solve.argtypes = [ctypes.c_void_p, ctypes.POINTER(_SYMBOLIC_LITERAL_SPAN), _ONMODEL_FUNC, ctypes.c_void_p, ctypes.POINTER(ctypes.c_int)]
#clingo_error_t clingo_control_solve_iter(clingo_control_t *ctl, clingo_symbolic_literal_span_t *assumptions, clingo_solve_iter_t **it);
_clingo.clingo_control_solve_iter.restype = ctypes.c_int
_clingo.clingo_control_solve_iter.argtypes = [ctypes.c_void_p, ctypes.POINTER(_SYMBOLIC_LITERAL_SPAN), ctypes.POINTER(ctypes.c_void_p)]
#clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_value_t atom, clingo_truth_value_t value);
_clingo.clingo_control_assign_external.restype = ctypes.c_int
_clingo.clingo_control_assign_external.argtypes = [ctypes.c_void_p, _VALUE, ctypes.c_int]
#clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_value_t atom);
_clingo.clingo_control_release_external.restype = ctypes.c_int
_clingo.clingo_control_release_external.argtypes = [ctypes.c_void_p]
#clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_parse_callback_t *cb, void *data);
_clingo.clingo_control_parse.restype = ctypes.c_int
_clingo.clingo_control_parse.argtypes = [ctypes.c_void_p, ctypes.c_char_p, _PARSE_CALLBACK, ctypes.c_void_p]
#clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data);
_clingo.clingo_control_add_ast.restype = ctypes.c_int
_clingo.clingo_control_add_ast.argtypes = [ctypes.c_void_p, _ADD_AST_CALLBACK, ctypes.c_void_p]

# }}}2

def handle_error(err):
    if err != 0:
        raise RuntimeError(_clingo.clingo_error_str(err))
    return False

class _Module:
    def __init__(self):
        self.__clingo = _clingo
        self.__mod = ctypes.c_void_p()
        handle_error(self.__clingo.clingo_module_new(ctypes.byref(self.__mod)))

    @property
    def _as_parameter_(self):
        return self.__mod

    def __del__(self):
        if self.__mod != 0:
            self.__clingo.clingo_module_free(self)

    def clingo(self):
        return self.__clingo

    def new_control(self, args):
        arg_t = ctypes.c_char_p * (len(args) + 1)
        args_c = arg_t()
        i = 0
        for arg in args:
            args_c[i] = arg
            i+= 1
        args_c[i] = None
        ctl = ctypes.c_void_p()
        handle_error(self.__clingo.clingo_control_new(self, len(args), args_c, ctypes.byref(ctl)))
        return ctl

_module = _Module()

# {{{1 Public

# {{{2 solve value

class Value:
    INF = 0
    NUM = 1
    ID  = 2
    STR = 3
    FUN = 4
    SUP = 6

    def __init__(self, val, byref=False):
        if byref:
            self.__val = val
        else:
            self.__val = _VALUE()
            self.__val.a = val.a
            self.__val.b = val.b

    @property
    def _as_parameter_(self):
        return self.__val

    def type(self):
        return _clingo.clingo_value_type(self)

    def num(self):
        return _clingo.clingo_value_num(self)

    def name(self):
        return _clingo.clingo_value_name(self)

    def str(self):
        return _clingo.clingo_value_str(self)

    def sign(self):
        return bool(_clingo.clingo_value_sign(self))

    def args(self):
        args_c = _clingo.clingo_value_args(self)
        args = []
        for i in range(0, args_c.size):
            args.append(Value(args_c.data[i]))
        return args

    def __eq__(self, b):
        # NOTE: python-2 would typically not raise a type error
        if not isinstance(b, Value):
            raise TypeError("unorderable types")
        return bool(_clingo.clingo_value_eq(self, b))

    def __lt__(self, b):
        # NOTE: python-2 would typically not raise a type error
        if not isinstance(b, Value):
            raise TypeError("unorderable types")
        return bool(_clingo.clingo_value_lt(self, b))

    def __repr__(self):
        str_c = ctypes.c_char_p()
        try:
            handle_error(_clingo.clingo_value_to_string(self, ctypes.byref(str_c)))
            ret = str_c.value
        finally:
            if str_c.value is not None:
                _clingo.clingo_free(str_c)
        return ret

    def __str__(self):
        return repr(self)

def create_num(num):
    val = _VALUE()
    _clingo.clingo_value_new_num(num, ctypes.byref(val))
    return Value(val, True)

def create_sup():
    val = _VALUE()
    _clingo.clingo_value_new_sup(ctypes.byref(val))
    return Value(val, True)
Sup = create_sup()

def create_inf():
    val = _VALUE()
    _clingo.clingo_value_new_inf(ctypes.byref(val))
    return Value(val, True)
Inf = create_inf()

def create_id(name, sign = False):
    val = _VALUE()
    _clingo.clingo_value_new_id(name, sign, ctypes.byref(val))
    return Value(val, True)

def create_str(name):
    val = _VALUE()
    handle_error(_clingo.clingo_value_new_str(name, ctypes.byref(val)))
    return Value(val, True)

def create_fun(name, args, sign = False):
    val = _VALUE()
    args_t = _VALUE * len(args)
    args_c = _VALUE_SPAN()
    args_c.data = args_t()
    args_c.size = len(args)
    for i, arg in zip(range(0, args_c.size), args):
        args_c.data[i] = arg._as_parameter_
    handle_error(_clingo.clingo_value_new_fun(name, args_c, sign, ctypes.byref(val)))
    return Value(val, True)

# {{{2 solve model

class Model:
    CSP = 1
    SHOWN = 2
    ATOMS = 4
    TERMS = 8
    COMP = 16

    def __init__(self, model):
        self.__model = model

    @property
    def _as_parameter_(self):
        return self.__model

    def atoms(self, show):
        atoms_c = _VALUE_SPAN()
        handle_error(_clingo.clingo_model_atoms(self, show, ctypes.byref(atoms_c)))
        atoms = []
        for i in range(0, atoms_c.size):
            atoms.append(Value(atoms_c.data[i]))
        return atoms

    def __str__(self):
        return str(self.atoms(Model.SHOWN))

# {{{2 solve result

class SolveResult:
    UNKNOWN = 0
    SAT     = 1
    UNSAT   = 2

# {{{2 solve iter

class SolveIter:
    def __init__(self, it):
        self.__it = it

    @property
    def _as_parameter_(self):
        return self.__it

    def __iter__(self):
        return self

    def __next__(self):
        model = ctypes.c_void_p()
        handle_error(_clingo.clingo_solve_iter_next(self, ctypes.byref(model)))
        if model.value is None:
            raise StopIteration()
        return Model(model)

    def next(self):
        return self.__next__()

    def get(self):
        ret = ctypes.c_int()
        handle_error(_clingo.clingo_solve_iter_get(self, ctypes.byref(ret)))
        return ret.value

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        handle_error(_clingo.clingo_solve_iter_close(self))

# {{{2 AST

class Position:
    def __init__(self, filename, line, column):
        self.filename = filename
        self.line = line
        self.column = column

class Location:
    def __init__(self, begin, end):
        self.begin = begin
        self.end = end

class AST:
    def __init__(self, value, children, location):
        self.value = value
        self.children = children
        self.location = location

    def __str_prefix(self, prefix):
        ret = prefix + str(self.value)
        if len(self.children) > 0:
            ret += ":\n"
            for child in self.children:
                ret += child.__str_prefix(prefix + "  ");
        else:
            ret += "\n"
        return ret

    def __str__(self):
        return self.__str_prefix("")

# {{{2 control

class Control:
    def __init__(self, args):
        self.__ctl = None
        self.__ctl = _module.new_control(args)

    @property
    def _as_parameter_(self):
        return self.__ctl

    def add(self, name, params, program):
        params_t = ctypes.c_char_p * (len(params) + 1)
        params_c = params_t()
        i = 0
        for arg in params:
            params_c[i] = arg
            i+= 1
        params_c[i] = None
        handle_error(_clingo.clingo_control_add(self, name, params_c, program))

    def ground(self, parts, context=None):
        parts_t = _PART * len(parts)
        part_span_c = _PART_SPAN()
        part_span_c.size = len(parts)
        part_span_c.data = parts_t()
        i = 0
        for name, params in parts:
            part_span_c.data[i] = _PART()
            part_span_c.data[i].name = name
            part_span_c.data[i].params = _VALUE_SPAN()
            part_span_c.data[i].params.size = len(params)
            param_t = _VALUE * len(params)
            part_span_c.data[i].params.data = param_t()
            for j, val in zip(range(0, len(params)), params):
                part_span_c.data[i].params.data[j] = val._as_parameter_
            i+= 1
        callback_p = None
        if context is not None:
            vals_c = [None]
            def call(name, args_c, data, ret_c):
                args = []
                for i in range(args_c.size):
                    args.append(Value(args_c.data[i], True))
                vals = getattr(context, name)(*args)
                if isinstance(vals, Value):
                    vals = [vals]
                ret_t = _VALUE * len(vals)
                vals_c[0] = ret_t()
                ret_c[0] = _VALUE_SPAN()
                ret_c[0].size = len(vals)
                ret_c[0].data = vals_c[0]
                for i, val in zip(range(len(vals)), vals):
                    ret_c[0].data[i] = val._as_parameter_
                return 0
            callback_p = _CALL_FUNC(call)
        handle_error(_clingo.clingo_control_ground(self, part_span_c, callback_p, None))

    def __on_model(self, on_model, m):
        ret = on_model(Model(m))
        return 1 if ret or ret is None else 0

    def __to_ass(self, assumptions):
        assumptions_c = _SYMBOLIC_LITERAL_SPAN()
        assumptions_c.size = len(assumptions)
        for i, arg in zip(range(0, assumptions_c.size), assumptions):
            assumptions_c.data[i] = _SYMBOLIC_LITERAL()
            assumptions_c.data[i].atom = arg[0]._as_parameter_
            assumptions_c.data[i].sign = ctypes.c_int(arg[1])
        return ctypes.byref(assumptions_c)

    def solve(self, on_model, assumptions = []):
        ret = ctypes.c_int()
        handle_error(_clingo.clingo_control_solve(self, self.__to_ass(assumptions), _ONMODEL_FUNC(lambda m, d: self.__on_model(on_model, m)), None, ctypes.byref(ret)))
        return ret.value

    def solve_iter(self, assumptions = []):
        it = ctypes.c_void_p()
        handle_error(_clingo.clingo_control_solve_iter(self, self.__to_ass(assumptions), it))
        return SolveIter(it)

    def assign_external(self, atom, value):
        value_c = 2
        if value is None:
            value_c = 0
        elif value:
            value_c = 1
        handle_error(_clingo.clingo_control_assign_external(self, atom, value_c))

    def release_external(self, atom, value):
        handle_error(_clingo.clingo_control_release_external(self, atom))

    def __convert_ast(self, node):
        children = []
        for i in range(node.children.size):
            children.append(self.__convert_ast(node.children.data[i]))
        begin = Position(node.location.begin_file, node.location.begin_line, node.location.begin_column)
        end = Position(node.location.end_file, node.location.end_line, node.location.end_column)
        return AST(Value(node.value), children, Location(begin, end))

    def __on_statement(self, node, tree):
        tree.append(self.__convert_ast(node.contents))
        return 0

    def parse(self, program):
        tree = []
        handle_error(_clingo.clingo_control_parse(self, program, _PARSE_CALLBACK(lambda node, d: self.__on_statement(node, tree)), None))
        return tree

    def add_ast(self, ast):
        class Adder:
            def __init__(self, ast):
                self.__current = []
                self.__ast = ast
            def conv(self, node):
                ret = _AST()
                ret.location.begin_file = node.location.begin.filename
                ret.location.begin_line = node.location.begin.line
                ret.location.begin_column = node.location.begin.column
                ret.location.end_file = node.location.end.filename
                ret.location.end_line = node.location.end.line
                ret.location.end_column = node.location.end.column
                self.__current.append(ret)
                ret.value = node.value._as_parameter_
                size = len(node.children)
                type = _AST * size
                ret.children.size = size
                ret.children.data = type()
                i = 0
                for child in node.children:
                    ret.children.data[i] = self.conv(child)
                    i+= 1
                return ret
            def add(self, node, d):
                try:
                    self.__current = []
                    x = self.__ast.next()
                    print x
                    n = self.conv(x)
                    t = ctypes.POINTER(_AST)
                    node[0] = t(n)
                except StopIteration:
                    node[0] = None
                return 0
        adder = Adder(iter(ast))
        handle_error(_clingo.clingo_control_add_ast(self, _ADD_AST_CALLBACK(adder.add), None))

    def __del__(self):
        if self.__ctl is not None:
            _clingo.clingo_control_free(self)

