"""
Test clingo's Application class.
"""

import os
import re
from multiprocessing import Process, Queue
from tempfile import NamedTemporaryFile
from typing import Any, Callable, List, Sequence, Tuple

import pytest
from clingo.app import App, AppOptions, Flag, clingo_main
from clingo.control import Control
from clingo.core import Library, MessageType
from util import MCB


class AppTest(App):
    """
    Test application covering most of the Application related API.

    Note that I did not find a nice way to test model printing.
    """

    _queue: Queue
    program_name = "test"
    version = "1.2.3"
    message_limit = 17

    def __init__(self, queue: Queue):
        super().__init__(AppTest.program_name, AppTest.version)
        self._queue = queue
        self._flag = Flag()

    def _parse_test(self, value):
        self._queue.put(("parse", value))
        return True

    def register_options(self, options: AppOptions) -> None:
        """
        Register test options.
        """
        self._queue.put("register")
        group = "Clingo.Test"
        options.add(group, "test", "test description", self._parse_test)
        options.add_flag(group, "flag", "test description", self._flag)

    def validate_options(self) -> None:
        """
        Validate the options.
        """
        self._queue.put("validate")
        self._queue.put(("flag", self._flag.value))

    def main(
        self,
        control: Control,
        files: Sequence[str],
    ) -> None:
        """
        Run the main loop.
        """
        self._queue.put("main")
        control.parse_files(files)
        control.ground(control.parts)
        mcb = MCB()
        control.solve(on_model=mcb)
        self._queue.put(("models", mcb.symbols))


def _run_process(
    app: Callable[[Queue], App], program: str, queue: Queue, args: Sequence[str]
) -> None:
    """
    Run clingo application with given program and intercept results.
    """
    with NamedTemporaryFile(mode="wt", delete=False) as fp:
        name = fp.name
        fp.write(program)
    try:
        # Note: The multiprocess module does not allow for intercepting the
        # output. Thus, the output is simply disabled and we use the Queue
        # class to communicate results.
        def logger(code: MessageType, msg: str):
            queue.put((code, re.sub("^.*:(?=[0-9]+:)", "", msg)))

        with Library(logger=logger) as lib:
            ret = clingo_main(lib, [name, "--outf=3"] + list(args), app(queue))
            queue.put(int(ret))
            queue.close()
    finally:
        os.unlink(name)


AppResult = Tuple[int, List[Any]]


def run_app(
    app: Callable[[Queue], App], program: str, *args: Sequence[str]
) -> AppResult:
    """
    Run clingo application in subprocess via multiprocessing module.
    """
    q: Queue
    q = Queue()
    p = Process(target=_run_process, args=(app, program, q, tuple(args)))

    p.start()
    seq: List[Any]
    seq, ret = [], -1
    while True:
        ret = q.get()
        if isinstance(ret, int):
            status = ret
            break
        seq.append(ret)
    p.join()
    q.close()

    return status, seq


class TestApplication:
    # pylint: disable=too-few-public-methods
    """
    Tests for clingo's application class.
    """

    @pytest.mark.filterwarnings("ignore::DeprecationWarning")
    def test_app(self):
        """
        Test application.
        """
        ret, seq = run_app(AppTest, "1 {a; b; c(1/0)}.", "0", "--test=x", "--flag")
        assert ret == 30
        assert seq == [
            "register",
            ("parse", "x"),
            "validate",
            ("flag", True),
            "main",
            (
                MessageType.OperationUndefined,
                "1:12-15: info: operation undefined:\n  1/0\n",
            ),
            ("models", [["a"], ["a", "b"], ["b"]]),
        ]
