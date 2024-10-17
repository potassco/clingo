"""
Example showing the use of scripts.
"""

import __main__
from clingo.control import Control

# pylint: disable=import-error
from clingo.core import Library
from clingo.script import Script, register
from clingo.symbol import Number, Symbol


class PyScript(Script):
    """
    Example script calling functions from the main scope.
    """

    def execute(self, code: str) -> None:
        """
        Execute code in the main scope.
        """
        exec(code, __main__.__dict__, __main__.__dict__)  # pylint: disable=exec-used

    def call(self, lib: Library, name: str, arguments: list[Symbol]) -> list[Symbol]:
        """
        Call the function with the given name and arguments from the main scope.
        """
        return [getattr(__main__, name)(lib, *arguments)]

    def callable(self, name: str, args: int) -> bool:
        """
        Check if there is a function with the given name in the main scope.
        """
        # pylint: disable=unused-argument
        return name in __main__.__dict__ and callable(__main__.__dict__[name])

    def main(self, lib: Library, control: Control) -> None:
        """
        Run the main function from the main scope.
        """
        # pylint: disable=c-extension-no-member
        __main__.main(lib, control)


def fun(lib: Library, num: Symbol) -> Symbol:
    """
    Test function to generate large numbers.
    """
    return Number(
        lib,
        num.number
        * 242357902759023475928437592438759234752049375294375293457902759247590275902745,
    )


def main(lib: Library):
    """
    The main function.
    """
    ctl = Control(lib, [])
    ctl.parse_string("#program one(k). p(k).")
    ctl.ground([("one", [Number(lib, 1)])])

    ctl.parse_string("#program ext(k). p(@fun(k)).")
    ctl.ground([("ext", [Number(lib, i)]) for i in range(1, 1000, 37)])


if __name__ == "__main__":
    LIB = Library()
    register(LIB, PyScript())
    main(LIB)
