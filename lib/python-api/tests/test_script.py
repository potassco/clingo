"""
Unit tests for clingo.script module.
"""

from textwrap import dedent
from unittest import TestCase

from clingo.control import Control
from clingo.core import Library
from clingo.script import Script, register
from clingo.symbol import Symbol


class MyScript(Script):
    """
    Example embedded script.
    """

    def __init__(self):
        super().__init__()
        self._scope = {}

    def name(self) -> str:
        """
        Get the name of the script.
        """
        return "myScript"

    def execute(self, code: str) -> None:
        """
        Execute code.
        """
        exec(code, self._scope, self._scope)  # pylint: disable=exec-used

    def call(self, lib: Library, name: str, arguments: list[Symbol]) -> list[Symbol]:
        """
        Call the function with the given name and arguments.
        """
        return [self._scope[name](lib, *arguments)]

    def callable(self, name: str, arguments: int) -> bool:
        """
        Check if there is a function with the given name in the scope.
        """
        # pylint: disable=unused-argument

        return name in self._scope and callable(self._scope[name])

    def main(self, lib: Library, control: Control) -> None:
        """
        Run the main function from the main scope.
        """
        self._scope["main"](lib, control)


class TestScript(TestCase):
    """
    Test scripting functionality.
    """

    def setUp(self):
        self._lib = Library()
        register(self.lib, MyScript())

    def tearDown(self):
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    def test_script(self):
        """
        The main function.
        """
        ctl = Control(self.lib, ["--text-buffer", "--mode=ground"])
        ctl.parse_string(
            dedent(
                """\
                #script (myScript)
                from clingo.core import Library
                from clingo.symbol import Symbol, Number

                def fun(lib: Library, num: Symbol) -> Symbol:
                    return Number(
                        lib,
                        num.number
                        * 242357902759023475928437592438759234752049375294375293457902759247590275902745,
                    )

                def main(lib, ctl):
                    ctl.parse_string("#program one(k). p(k).")
                    ctl.ground([("one", [Number(lib, 1)])])

                    ctl.parse_string("#program ext(k). p(@fun(k)).")
                    ctl.ground([("ext", [Number(lib, i)]) for i in range(1, 1000, 257)])
                #end.
                """
            )
        )
        ctl.main()

        self.assertEqual(
            ctl.buffer,
            dedent(
                """\
                p(1).
                #show p(1): p(1).
                #show.
                p(242357902759023475928437592438759234752049375294375293457902759247590275902745).
                p(62528338911828056789536898849199882566028738825948825712138911885878291182908210).
                p(124814319920897090103145360105961005897305428276603276130819921012508992089913675).
                p(187100300929966123416753821362722129228582117727257726549500930139139692996919140).
                #show p(242357902759023475928437592438759234752049375294375293457902759247590275902745): p(242357902759023475928437592438759234752049375294375293457902759247590275902745).
                #show p(62528338911828056789536898849199882566028738825948825712138911885878291182908210): p(62528338911828056789536898849199882566028738825948825712138911885878291182908210).
                #show p(124814319920897090103145360105961005897305428276603276130819921012508992089913675): p(124814319920897090103145360105961005897305428276603276130819921012508992089913675).
                #show p(187100300929966123416753821362722129228582117727257726549500930139139692996919140): p(187100300929966123416753821362722129228582117727257726549500930139139692996919140).
                """
            ),
        )
