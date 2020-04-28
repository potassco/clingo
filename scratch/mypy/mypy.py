"""
Simple script to generate stub files for the clingo python module.

TODO:
- currently there is no information which interfaces are implemented by a class
- no stubs are generated for the ast module
- compare with hand-written stub file
"""

import types
import sys
import inspect

from mako.template import Template
import clingo


CLASS_TEMPLATE = """\
class ${name}:
% for x in functions:
    ${x.stub()}
% endfor
% for x in variables:
    ${x.stub()}
% endfor
"""


TEMPLATE = """\
from typing import *
from clingo_abc import Application, Observer, Propagator


% for x in variables:
${x.stub()}
% endfor


% for x in functions:
${x.stub()}
% endfor


% for x in classes:
${x.stub()}
% endfor
"""


def get_sig(value):
    """
    Extract the signature of a docstring.
    """
    return value.__doc__.strip().splitlines()[0]


class Function:
    """
    Something that resembles a function.
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def stub(self):
        """
        Generate stub for function.
        """
        return "def {}: ...".format(get_sig(self.value))


class Property:
    """
    Something that resembles a property (with a docstring).
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def stub(self):
        """
        Generate stub for property.
        """
        return "{}".format(get_sig(self.value))


class Variable:
    """
    Something that resembles a member variable.
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def stub(self):
        """
        Generate stub for member variable.
        """
        return "{}: {}".format(self.name, self.value.__class__.__name__)


class Class:
    """
    Something that resembles a class.
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def stub(self):
        """
        Generate stub for class.
        """

        functions = []
        variables = []
        for name, value in sorted(self.value.__dict__.items(), key=lambda x: x[0]):
            if name.startswith("_"):
                continue
            if inspect.ismethoddescriptor(value):
                functions.append(Function(name, value))
            elif inspect.isgetsetdescriptor(value):
                variables.append(Property(name, value))
            else:
                variables.append(Variable(name, value))

        return Template(CLASS_TEMPLATE).render(
            name=self.name,
            variables=variables,
            functions=functions)


class Module:
    """
    Something that resembles a module.
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def stub(self):
        """
        Print stub for module.
        """
        functions = []
        classes = []
        modules = []
        variables = []

        for name, value in sorted(self.value.__dict__.items(), key=lambda x: x[0]):
            if name.startswith("_") and name != "__version__":
                continue
            if isinstance(value, type):
                classes.append(Class(name, value))
            elif isinstance(value, types.BuiltinFunctionType):
                functions.append(Function(name, value))
            elif isinstance(value, types.ModuleType):
                modules.append(Module(name, value))
            else:
                variables.append(Variable(name, value))

        return Template(TEMPLATE).render(
            classes=classes,
            functions=functions,
            variables=variables)


if __name__ == "__main__":
    sys.stdout.write(Module("clingo", clingo).stub())
