"""
Collect models in the background with a time limit.

Models are intercepted with a callback, which runs on the solver thread. The
callback copies the symbols because the model is only valid during the call.
"""

import asyncio
from collections.abc import Sequence

from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Symbol

PROGRAM = "{ p(1..40) }."


async def heartbeat() -> None:
    """
    Show that the event loop stays responsive while solving.
    """
    while True:
        await asyncio.sleep(0.25)
        print("event loop is alive")


async def main() -> None:
    """
    Solve with a time limit.
    """
    loop = asyncio.get_running_loop()
    models: list[Sequence[Symbol]] = []
    lib = Library()
    ctl = Control(lib, ["0"])
    ctl.parse_string(PROGRAM)
    await ctl.ground_async(loop)
    beat = asyncio.create_task(heartbeat())
    try:
        await asyncio.wait_for(
            ctl.solve_async(
                loop, on_model=lambda mdl: models.append(mdl.symbols(shown=True))
            ),
            timeout=1.0,
        )
        print("search finished")
    except TimeoutError:
        print("search timed out")
    finally:
        beat.cancel()
    print(f"collected {len(models)} models")


asyncio.run(main())
