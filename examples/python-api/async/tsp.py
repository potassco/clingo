"""
Optimize a traveling salesperson tour with a portfolio of solvers.

Several solver configurations search concurrently and the best tour found by
any of them is kept. The search stops when the time limit expires, when no
configuration has found a better tour for the cutoff time, when one
configuration proved its tour optimal, or when Ctrl-C is pressed. The best
tour found so far is printed in each case.
"""

import asyncio
import math
import random
import signal

from clingo.control import Control
from clingo.core import Library
from clingo.solve import AsyncSolveHandle, Model

ENCODING = """
edge(X,Y) :- cost(X,Y,_).
node(X) :- edge(X,_).

{ cycle(X,Y) : edge(X,Y) } = 1 :- node(X).
{ cycle(X,Y) : edge(X,Y) } = 1 :- node(Y).

reached(Y) :- cycle(1,Y).
reached(Y) :- cycle(X,Y), reached(X).
:- node(Y), not reached(Y).

#minimize { C,X,Y : cycle(X,Y), cost(X,Y,C) }.
#show cycle/2.
"""

CONFIGS = {
    "trendy": ["--configuration=trendy"],
    "crafty": ["--configuration=crafty"],
    "jumpy": ["--configuration=jumpy"],
    "usc": ["--opt-strategy=usc"],
}


def instance(n: int, seed: int = 42) -> str:
    """
    Generate a traveling salesperson instance with n random cities.
    """
    rng = random.Random(seed)
    pts = [(rng.randint(0, 100), rng.randint(0, 100)) for _ in range(n)]
    return "\n".join(
        f"cost({i},{j},{round(math.dist(p, q))})."
        for i, p in enumerate(pts, 1)
        for j, q in enumerate(pts, 1)
        if i != j
    )


def tour(mdl: Model) -> list[int]:
    """
    Get the cities of the tour in the model starting from city 1.
    """
    succ = {
        sym.arguments[0].number: sym.arguments[1].number
        for sym in mdl.symbols(shown=True)
    }
    cities = [1]
    while (city := succ[cities[-1]]) != 1:
        cities.append(city)
    return cities


class Portfolio:
    """
    Keep the best tour of several searches and stop them all together.
    """

    def __init__(self, cutoff_time: float):
        self.loop = asyncio.get_running_loop()
        self.start = self.loop.time()
        self.cutoff_time = cutoff_time
        self.cutoff = self.loop.call_later(cutoff_time, self.stop, "cutoff")
        self.handles: list[AsyncSolveHandle] = []
        self.stopped = False
        self.best: tuple[int, str, list[int]] | None = None

    def log(self, msg: str) -> None:
        """
        Print a message with the elapsed time.
        """
        print(f"{self.loop.time() - self.start:5.2f}s: {msg}")

    def stop(self, reason: str) -> None:
        """
        Cancel all searches.
        """
        if not self.stopped:
            self.stopped = True
            self.cutoff.cancel()
            self.log(f"stopped by {reason}")
            for hnd in self.handles:
                hnd.cancel()

    def improve(self, name: str, mdl: Model) -> None:
        """
        Keep the tour in the model if it is better and restart the cutoff.
        """
        cost = mdl.cost[0]
        if self.best is None or cost < self.best[0]:
            self.best = (cost, name, tour(mdl))
            self.log(f"tour with cost {cost} by {name}")
            self.cutoff.cancel()
            self.cutoff = self.loop.call_later(self.cutoff_time, self.stop, "cutoff")

    async def run(self, name: str, ctl: Control) -> None:
        """
        Search with the given control object until it is stopped.
        """
        async with ctl.solve_async(self.loop, yield_=True) as hnd:
            self.handles.append(hnd)
            if self.stopped:
                hnd.cancel()
            async for mdl in hnd:
                self.improve(name, mdl)
            if (await hnd).exhausted:
                self.stop(f"{name} proving optimality")


async def main(n: int, solve_time: float, cutoff_time: float) -> None:
    """
    Run the portfolio.
    """
    loop = asyncio.get_running_loop()
    program = instance(n)
    lib = Library()
    ctls = {name: Control(lib, args) for name, args in CONFIGS.items()}
    for ctl in ctls.values():
        ctl.parse_string(ENCODING)
        ctl.parse_string(program)
    await asyncio.gather(*(ctl.ground_async(loop) for ctl in ctls.values()))

    portfolio = Portfolio(cutoff_time)
    timer = loop.call_later(solve_time, portfolio.stop, "time limit")
    loop.add_signal_handler(signal.SIGINT, portfolio.stop, "Ctrl-C")
    try:
        await asyncio.gather(*(portfolio.run(name, ctl) for name, ctl in ctls.items()))
    finally:
        loop.remove_signal_handler(signal.SIGINT)
        timer.cancel()

    if portfolio.best is not None:
        cost, name, cities = portfolio.best
        print(f"best tour with cost {cost} by {name}:")
        print(" ".join(map(str, cities)))


asyncio.run(main(60, solve_time=10.0, cutoff_time=2.0))
