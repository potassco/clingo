"""
Module to add custom scripts that can be embedded into logic programs.

# Examples

The following example shows how to register a custom python script that executes functions
from the main context (just like the embedded one in the standalone clingo).

```python
>>> import __main__
>>> from clingo.control import Control
>>> from clingo.core import Library
>>> from clingo.script import Script, register
>>> from clingo.symbol import Number, Symbol
...
>>> class PyScript(Script):
...     def execute(self, code) -> None:
...         exec(code, __main__.__dict__, __main__.__dict__)
...     def call(self, lib, name, arguments):
...         return [getattr(__main__, name)(lib, *arguments)]
...     def callable(self, name, arguments) -> bool:
...         return name in __main__.__dict__ and callable(__main__.__dict__[name])
...     def main(self, lib, control) -> None:
...         __main__.main(lib, control)
...     def name(self):
...         return "python"
...
>>> def f(lib, x):
...     return Number(lib, x.number * 3)
...
>>> lib = Library()
>>> register(lib, PyScript())
>>> ctl = Control(lib, ["--mode=ground"])
>>> ctl.parse_string(\"\"\"\\
... #script (python)
...
... from clingo.symbol import Number, Symbol
...
... def g(lib, x):
...     return Number(lib, x.number * 4)
...
... #end.
... p(@f(1)).
... q(@g(2)).
... \"\"\")
>>> ctl.ground()
>>> ctl.buffer
\"\"\"\\
p(3).
q(8).
#show p/1.
#show q/1.
#show.
\"\"\"
```
"""

from __future__ import annotations

import typing

import clingo.control
import clingo.core
import clingo.symbol

__all__: list[str] = ["Script", "enable_python", "register"]

def enable_python(lib: clingo.core.Library) -> None:
    """
    Enable embedded python scripts.

    Args:
        lib:
            The library to register the script with.
    """

def register(lib: clingo.core.Library, script: Script) -> None:
    """
    Registers a script language which can then be embedded into a logic program.

    Args:
        lib:
            The library to register the script with.
        script:
            The script to register.
    """

class Script:
    """
    ABC for custom scripts.
    """

    def __init__(self) -> None:
        """
        Construct a script object.
        """

    def call(
        self,
        lib: clingo.core.Library,
        name: str,
        arguments: typing.Sequence[clingo.symbol.Symbol],
    ) -> typing.Sequence[clingo.symbol.Symbol]:
        """
        Call the function with the given name and arguments.

        Args:
            lib:
                The library object to store symbols.
            name:
                The name of the function.
            arguments:
                The arguments of the function.

        Returns:
            A list of symbols.
        """

    def callable(self, name: str, arguments: int) -> bool:
        """
        Check if a function with the given signature is callable.

        Args:
            name:
                The name of the function.
            arguments:
                The number of arguments of the function.

        Returns:
            Whether the function is callable.
        """

    def execute(self, code: str) -> None:
        """
        Execute the given code.

        Args:
            code:
                The code to execute.
        """

    def main(self, lib: clingo.core.Library, control: clingo.control.Control) -> None:
        """
        Run the main function.

        Args:
            lib:
                The (main) library object.
            control:
                The (main) control object.
        """

    def name(self) -> str:
        """
        Get the name of the script.
        """

    def version(self) -> str:
        """
        Get the version of the script.
        """

class _MainScript:
    pass
