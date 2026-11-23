"""
Stream models to a slow consumer.

With yield_=True, the solver pauses after each model until the next one is
requested. A slow consumer thus throttles the search instead of letting models
pile up in a queue.
"""

import asyncio

from clingo.control import Control
from clingo.core import Library

PROGRAM = "1 { p(1..5) } 2."


async def consume(model: str) -> None:
    """
    Simulate sending a model over a slow connection.
    """
    await asyncio.sleep(0.1)
    print(f"sent: {model}")


async def main() -> None:
    """
    Stream all models.
    """
    loop = asyncio.get_running_loop()
    with Library() as lib:
        ctl = Control(lib, ["0"])
        ctl.parse_string(PROGRAM)
        await ctl.ground_async(loop)
        async with ctl.solve_async(loop, yield_=True) as hnd:
            async for mdl in hnd:
                await consume(" ".join(str(sym) for sym in mdl.symbols(shown=True)))
            print(await hnd)


asyncio.run(main())
