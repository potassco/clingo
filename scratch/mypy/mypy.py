#!/usr/bin/env python
"""
Simple script to generate stub files for the clingo python module.
"""

import re
import types
import inspect
import os.path

from mako.lookup import TemplateLookup
from mako.template import Template
import clingo

BASE = os.path.dirname(__file__)
LOOKUP = TemplateLookup(directories=[BASE], input_encoding="utf-8")

CLASS_TEMPLATE = Template("""\
class ${sig}:
% for x in functions:
    ${x.stub()}
% endfor
% for x in variables:
    ${x.stub()}
% endfor
% if not functions and not variables:
    pass
% endif
""", lookup=LOOKUP)


CLINGO_TEMPLATE = Template('''\
from typing import AbstractSet, Any, Callable, ContextManager, Iterable, Iterator, List, Mapping, MutableSequence, Optional, Sequence, Tuple, ValuesView, Union
from abc import ABC, ABCMeta, abstractmethod

from . import ast

<%include file="abc.py"/>

% for x in variables:
${x.stub()}
% endfor


% for x in functions:
${x.stub()}
% endfor


% for x in classes:
${x.stub()}
% endfor
''', lookup=LOOKUP)


AST_TEMPLATE = Template('''\
from typing import Any, List, Mapping, Tuple
from abc import ABCMeta


% for x in variables:
${x.stub()}
% endfor


% for x in functions:
${x.stub()}
% endfor


% for x in classes:
${x.stub()}
% endfor
''', lookup=LOOKUP)


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
        if self.value.__doc__ is None:
            return "def {}(*args: List[Any], **kwargs: Mapping[str,Any]) -> Any: ...".format(self.name)
        return "def {}: ...".format(get_sig(self.value))


class Other:
    """
    Something arbitrary.
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def stub(self):
        """
        Generate stub.
        """
        return self.value


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

    def declare(self):
        """
        Generate a forward declaration for the class.
        """
        return '{} = ForwardRef("{}")'.format(self.name, self.name)

    def stub(self):
        """
        Generate stub for class.
        """
        doc = self.value.__doc__.strip()
        abstract = not doc.startswith("{}(".format(self.name))

        anc = []
        match = re.search(r"Implements: `[^`]*`(, `[^`]*`)*", doc)
        if match is not None:
            for match in re.finditer(r"`([^`]*)`", match.group(0)):
                anc.append(match.group(1))
        if abstract:
            anc.append("metaclass=ABCMeta")

        sig = "{}({})".format(self.name, ", ".join(anc)) if anc else self.name

        functions = []
        variables = []

        if not abstract:
            init = doc.splitlines()[0].strip()
            init = init.replace(self.name + "(", "__init__(self, ", 1)
            init = init.replace(" -> {}".format(self.name), "")
            functions.append(Other("__init__", "def {}: ...".format(init)))

        for name, value in sorted(self.value.__dict__.items(), key=lambda x: x[0]):
            if name.startswith("_"):
                continue
            if inspect.ismethoddescriptor(value):
                functions.append(Function(name, value))
            elif inspect.isgetsetdescriptor(value):
                variables.append(Property(name, value))
            else:
                variables.append(Variable(name, value))

        return CLASS_TEMPLATE.render(
            sig=sig,
            variables=variables,
            functions=functions)


class Module:
    """
    Something that resembles a module.
    """
    def __init__(self, template, name, value):
        self.template = template
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
                modules.append((name, value))
            else:
                variables.append(Variable(name, value))

        return self.template.render(
            classes=classes,
            functions=functions,
            variables=variables).rstrip() + "\n"


if __name__ == "__main__":
    with open(os.path.join(BASE, "..", "..", "libpyclingo", "clingo", "__init__.pyi"), "w") as handle:
        handle.write(Module(CLINGO_TEMPLATE, "clingo", clingo).stub())

    with open(os.path.join(BASE, "..", "..", "libpyclingo", "clingo", "ast.pyi"), "w") as handle:
        handle.write(Module(AST_TEMPLATE, "clingo.ast", clingo.ast).stub())
