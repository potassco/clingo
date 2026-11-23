"""
Race several solver configurations and keep the result of the fastest one.

The searches run concurrently in the background. Once the first one finishes,
the remaining ones are cancelled, which interrupts their searches.
"""

import asyncio
import random
import time

from clingo.control import Control
from clingo.core import Library

ENCODING = """
color(1..3).
node(X) :- edge(X,_).
node(Y) :- edge(_,Y).
1 { assign(X,C) : color(C) } 1 :- node(X).
:- edge(X,Y), assign(X,C), assign(Y,C).
"""

CONFIGS = ["frumpy", "jumpy", "tweety", "trendy", "crafty", "handy"]


def instance(n: int, ratio: float, seed: int = 1) -> str:
    """
    Generate a random graph with n nodes to be colored with three colors.
    """
    rng = random.Random(seed)
    edges: dict[tuple[int, int], None] = {}
    while len(edges) < int(ratio * n):
        a, b = rng.sample(range(1, n + 1), 2)
        edges[min(a, b), max(a, b)] = None
    return "\n".join(f"edge({a},{b})." for a, b in edges)


async def solve(lib: Library, program: str, config: str) -> tuple[str, str]:
    """
    Solve the program with the given configuration.
    """
    loop = asyncio.get_running_loop()
    ctl = Control(lib, [f"--configuration={config}"])
    ctl.parse_string(ENCODING)
    ctl.parse_string(program)
    await ctl.ground_async(loop)
    start = time.perf_counter()
    try:
        res = await ctl.solve_async(loop)
    except asyncio.CancelledError:
        print(f"{config:>6}: cancelled after {time.perf_counter() - start:.2f}s")
        raise
    print(f"{config:>6}: {res} after {time.perf_counter() - start:.2f}s")
    return config, str(res)


async def main() -> None:
    """
    Run the race.
    """
    program = instance(800, 2.25)
    with Library() as lib:
        tasks = [asyncio.create_task(solve(lib, program, config)) for config in CONFIGS]
        done, pending = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
        for task in pending:
            task.cancel()
        await asyncio.gather(*pending, return_exceptions=True)
        config, res = done.pop().result()
        print(f"winner: {config} ({res})")


asyncio.run(main())
