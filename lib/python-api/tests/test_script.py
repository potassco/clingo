"""
Unit tests for clingo.script module.
"""

from textwrap import dedent
from typing import Sequence

from clingo.control import Control
from clingo.core import Library
from clingo.script import Script, register
from clingo.symbol import Number, Symbol
from util import MCB


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

    def call(
        self, lib: Library, name: str, arguments: Sequence[Symbol]
    ) -> Sequence[Symbol]:
        """
        Call the function with the given name and arguments.
        """
        return [self._scope[name](lib, *arguments)]

    def callable(self, name: str, arguments: int) -> bool:
        """
        Check if there is a function with the given name in the scope.
        """
        assert arguments >= 0

        return name in self._scope and callable(self._scope[name])

    def main(
        self,
        lib: Library,
        control: Control,
    ) -> None:
        """
        Run the main function from the main scope.
        """
        self._scope["main"](lib, control)


class TestScript:
    # pylint: disable=attribute-defined-outside-init
    """
    Test scripting functionality.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()
        register(self.lib, MyScript())

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    def _add_script(self, ctl):
        ctl.parse_string(dedent("""\
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
                """))

    def test_script_ground(self):
        """
        The main function.
        """
        ctl = Control(self.lib, ["--convert=text"])
        self._add_script(ctl)
        ctl.main()

        assert ctl.buffer == dedent("""\
            p(1).
            #show p/1.
            #show.
            p(242357902759023475928437592438759234752049375294375293457902759247590275902745).
            p(62528338911828056789536898849199882566028738825948825712138911885878291182908210).
            p(124814319920897090103145360105961005897305428276603276130819921012508992089913675).
            p(187100300929966123416753821362722129228582117727257726549500930139139692996919140).
            """)

    def test_script_solve(self):
        """
        Test incremental program with script.
        """
        ctl = Control(self.lib, [])
        self._add_script(ctl)

        ctl.parse_string("#program one(k). p(k).")
        ctl.ground([("one", [Number(self.lib, 1)])])
        mcb = MCB()
        assert ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [["p(1)"]]

        ctl.parse_string("#program ext(k). p(@fun(k)).")
        ctl.ground([("ext", [Number(self.lib, i)]) for i in range(1, 1000, 257)])
        mcb = MCB()
        assert ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [
            [
                "p(1)",
                "p(242357902759023475928437592438759234752049375294375293457902759247590275902745)",
                "p(62528338911828056789536898849199882566028738825948825712138911885878291182908210)",
                "p(124814319920897090103145360105961005897305428276603276130819921012508992089913675)",
                "p(187100300929966123416753821362722129228582117727257726549500930139139692996919140)",
            ]
        ]
