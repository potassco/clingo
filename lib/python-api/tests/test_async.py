"""
Unit tests for solving and grounding with asyncio.
"""

import asyncio
import gc
import threading

import pytest
from clingo.control import Control
from clingo.core import Library
from clingo.ground import GroundResult
from clingo.symbol import Function, Number
from util import MCB, needs_threads

# a program with too many models to enumerate
LARGE = "{ a(1..64) }."


@needs_threads
class TestAsync:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for solving and grounding with asyncio.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()

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

    def control(self, program: str, options: list[str] | None = None) -> Control:
        """
        Create a grounded control object.
        """
        ctl = Control(self.lib, ["0"] if options is None else options)
        ctl.parse_string(program)
        ctl.ground()
        return ctl

    def test_await(self):
        """
        Test awaiting the result intercepting models with a callback.
        """

        async def run():
            ctl = self.control("1 { a; b } 1.")
            mcb = MCB()
            res = await ctl.solve_async(asyncio.get_running_loop(), on_model=mcb)
            assert res.satisfiable and res.exhausted
            assert mcb.symbols == [["a"], ["b"]]

        asyncio.run(run())

    def test_yield(self):
        """
        Test iterating over models.
        """

        async def run():
            ctl = self.control("1 { a; b } 1.")
            mcb = MCB()
            mcc = MCB()
            async with ctl.solve_async(
                asyncio.get_running_loop(), on_model=mcb, yield_=True
            ) as hnd:
                async for mdl in hnd:
                    mcc(mdl)
                res = await hnd
            assert res.satisfiable and res.exhausted
            assert mcb.symbols == [["a"], ["b"]]
            assert mcc.symbols == [["a"], ["b"]]

        asyncio.run(run())

    def test_yield_break(self):
        """
        Test that awaiting the result continues the search after breaking.
        """

        async def run():
            ctl = self.control("1 { a; b; c } 1.")
            mcb = MCB()
            mcc = MCB()
            async with ctl.solve_async(
                asyncio.get_running_loop(), on_model=mcb, yield_=True
            ) as hnd:
                async for mdl in hnd:
                    mcc(mdl)
                    break
                res = await hnd
                # iteration has finished once the result has been awaited
                assert [mdl async for mdl in hnd] == []
            assert res.satisfiable and res.exhausted
            assert len(mcc.symbols) == 1
            assert mcb.symbols == [["a"], ["b"], ["c"]]

        asyncio.run(run())

    def test_no_yield(self):
        """
        Test that models cannot be iterated without yield.
        """

        async def run():
            ctl = self.control("1 { a; b } 1.")
            async with ctl.solve_async(asyncio.get_running_loop()) as hnd:
                with pytest.raises(TypeError):
                    async for _ in hnd:
                        pass
                assert (await hnd).satisfiable

        asyncio.run(run())

    def test_last(self):
        """
        Test getting the last model after awaiting the result.
        """

        async def run():
            ctl = self.control("1 { p(1..3) } 1. #minimize { X: p(X) }.", [])
            async with ctl.solve_async(asyncio.get_running_loop()) as hnd:
                assert hnd.last() is None
                res = await hnd
                mdl = hnd.last()
                assert mdl is not None
                assert mdl.optimality_proven
                assert [str(sym) for sym in mdl.symbols(shown=True)] == ["p(1)"]
            assert res.satisfiable

        asyncio.run(run())

    def test_core(self):
        """
        Test getting an unsatisfiable core.
        """

        async def run():
            ctl = self.control("{ a; b }. :- a, b.")
            a = Function(self.lib, "a")
            b = Function(self.lib, "b")
            async with ctl.solve_async(
                asyncio.get_running_loop(), assumptions=[(a, True), (b, True)]
            ) as hnd:
                res = await hnd
                assert res.unsatisfiable
                assert len(hnd.core()) == 2

        asyncio.run(run())

    def test_cancel(self):
        """
        Test cancelling a search.
        """

        async def run():
            ctl = self.control(LARGE)
            async with ctl.solve_async(
                asyncio.get_running_loop(), on_model=lambda _: None
            ) as hnd:
                await asyncio.sleep(0.01)
                hnd.cancel()
                res = await hnd
            assert res.interrupted

        asyncio.run(run())

    def test_cancel_yield(self):
        """
        Test cancelling a search while a model is pending.
        """

        async def run():
            ctl = self.control(LARGE)
            async with ctl.solve_async(asyncio.get_running_loop(), yield_=True) as hnd:
                async for _ in hnd:
                    hnd.cancel()
                    break
                res = await hnd
            assert res.interrupted

        asyncio.run(run())

    def test_cancel_finished(self):
        """
        Test that cancelling a finished or cancelled search has no effect.
        """

        async def run():
            ctl = self.control(LARGE)
            async with ctl.solve_async(asyncio.get_running_loop(), yield_=True) as hnd:
                async for _ in hnd:
                    break
                hnd.cancel()
                hnd.cancel()
                assert [mdl async for mdl in hnd] == []
                res = await hnd
                hnd.cancel()
                assert res.interrupted
                assert hnd.last() is not None

        asyncio.run(run())

    def test_timeout(self):
        """
        Test that cancelling the awaiting task interrupts the search.
        """

        async def run():
            ctl = self.control(LARGE)
            async with ctl.solve_async(
                asyncio.get_running_loop(), on_model=lambda _: None
            ) as hnd:
                with pytest.raises(TimeoutError):
                    await asyncio.wait_for(hnd, 0.01)
                # the cancellation waits until the search stopped
                assert hnd.last() is not None
                # the result can still be awaited
                res = await hnd
            assert res.interrupted

        asyncio.run(run())

    def test_timeout_yield(self):
        """
        Test that cancelling the wait for a model keeps the result available.
        """

        async def run():
            ctl = self.control(LARGE)
            n = 0
            async with ctl.solve_async(asyncio.get_running_loop(), yield_=True) as hnd:
                # the first model is received without timeout to not depend on
                # the speed of the machine
                models = aiter(hnd)
                await anext(models)
                n += 1
                with pytest.raises(TimeoutError):
                    async with asyncio.timeout(0.01):
                        async for _ in models:
                            n += 1
                # the cancellation waits until the search stopped
                assert hnd.last() is not None
                res = await hnd
            assert n > 0
            assert res.interrupted

        asyncio.run(run())

    def test_exit(self):
        """
        Test leaving the context without awaiting the result.
        """

        async def run():
            ctl = self.control(LARGE)
            async with ctl.solve_async(asyncio.get_running_loop(), yield_=True) as hnd:
                async for _ in hnd:
                    break
            with pytest.raises(RuntimeError):
                hnd.last()
            # the control object can be used again
            assert (
                await ctl.solve_async(
                    asyncio.get_running_loop(), on_model=lambda _: False
                )
            ).satisfiable

        asyncio.run(run())

    def test_drop(self):
        """
        Test dropping a handle without awaiting it.
        """

        async def run():
            ctl = self.control(LARGE)
            hnd = ctl.solve_async(asyncio.get_running_loop(), on_model=lambda _: None)
            await asyncio.sleep(0.01)
            del hnd
            gc.collect()
            assert (
                await ctl.solve_async(
                    asyncio.get_running_loop(), on_model=lambda _: False
                )
            ).satisfiable

        asyncio.run(run())

    def test_error(self):
        """
        Test that errors in callbacks are raised when awaiting the result.
        """

        def on_model(_):
            raise RuntimeError("on_model")

        async def run():
            ctl = self.control("1 { a; b } 1.")
            with pytest.raises(RuntimeError, match="on_model"):
                await ctl.solve_async(asyncio.get_running_loop(), on_model=on_model)

        asyncio.run(run())

    def test_concurrent(self):
        """
        Test running several searches concurrently.
        """

        async def solve(n: int) -> list[list[str]]:
            ctl = Control(self.lib, ["0"])
            ctl.parse_string(f"1 {{ p(1..{n}) }} 1.")
            await ctl.ground_async(asyncio.get_running_loop())
            mcb = MCB()
            async with ctl.solve_async(asyncio.get_running_loop(), yield_=True) as hnd:
                async for mdl in hnd:
                    mcb(mdl)
                assert (await hnd).satisfiable
            return mcb.symbols

        async def run():
            res = await asyncio.gather(*(solve(n) for n in range(1, 6)))
            assert res == [[[f"p({i})"] for i in range(1, n + 1)] for n in range(1, 6)]

        asyncio.run(run())

    def test_ground(self):
        """
        Test grounding with a context.
        """

        class Context:
            """
            Context providing external functions.
            """

            def __init__(self, lib: Library):
                self.lib = lib

            def inc(self, x):
                """
                Increment a number.
                """
                return Number(self.lib, x.number + 1)

        async def run():
            ctl = Control(self.lib)
            ctl.parse_string("p(@inc(1)).")
            res = await ctl.ground_async(
                asyncio.get_running_loop(), context=Context(self.lib)
            )
            assert res == GroundResult.Ok
            mcb = MCB()
            assert (
                await ctl.solve_async(asyncio.get_running_loop(), on_model=mcb)
            ).satisfiable
            assert mcb.symbols == [["p(2)"]]

        asyncio.run(run())

    def test_ground_error(self):
        """
        Test that errors in context functions are raised when awaiting grounding.
        """

        class Context:
            """
            Context providing a failing function.
            """

            def fail(self):
                """
                Raise an error.
                """
                raise RuntimeError("fail")

        async def run():
            ctl = Control(self.lib)
            ctl.parse_string("p(@fail()).")
            with pytest.raises(RuntimeError, match="fail"):
                await ctl.ground_async(asyncio.get_running_loop(), context=Context())

        asyncio.run(run())

    def test_ground_cancel(self):
        """
        Test that cancelling grounding waits until it stopped and the result can still be awaited.
        """
        event = threading.Event()

        class Context:
            """
            Context providing a blocking function.
            """

            def block(self, x):
                """
                Block until the event is set.
                """
                event.wait(5)
                return x

        async def run():
            ctl = Control(self.lib)
            ctl.parse_string("p(1..3). q(@block(X)) :- p(X).")
            loop = asyncio.get_running_loop()
            hnd = ctl.ground_async(loop, context=Context())
            loop.call_later(0.05, event.set)
            with pytest.raises(TimeoutError):
                await asyncio.wait_for(hnd, 0.01)
            # the cancellation waits until the context function returned
            assert event.is_set()
            assert await hnd == GroundResult.Interrupted
            assert await hnd == GroundResult.Interrupted

        asyncio.run(run())

    def test_ground_cancel_repeated(self):
        """
        Test that cancelling the awaiting task again keeps waiting until grounding stopped.
        """
        event = threading.Event()

        class Context:
            """
            Context providing a blocking function.
            """

            def block(self, x):
                """
                Block until the event is set.
                """
                event.wait(5)
                return x

        async def run():
            ctl = Control(self.lib)
            ctl.parse_string("p(1..3). q(@block(X)) :- p(X).")
            hnd = ctl.ground_async(asyncio.get_running_loop(), context=Context())

            async def wait():
                return await hnd

            task = asyncio.create_task(wait())
            await asyncio.sleep(0.01)
            task.cancel()
            await asyncio.sleep(0.01)
            task.cancel()
            await asyncio.sleep(0.01)
            assert not task.done()
            event.set()
            with pytest.raises(asyncio.CancelledError):
                await task
            assert await hnd == GroundResult.Interrupted

        asyncio.run(run())
