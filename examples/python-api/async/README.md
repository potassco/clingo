## Solving with asyncio

These examples show how to ground and solve with Python's `asyncio` using
`Control.ground_async` and `Control.solve_async`. Grounding and solving run in
the background, so the event loop stays responsive and several searches can
run concurrently.

- `race.py` races several solver configurations and cancels the slower ones.
- `timeout.py` collects models with a callback and stops after a time limit.
- `stream.py` streams models to a slow consumer, which throttles the search.
- `tsp.py` optimizes a traveling salesperson tour with a portfolio of solver
  configurations that share the best tour, a time limit, a cutoff that is
  restarted with each better tour, and Ctrl-C to stop early.

Callbacks like `on_model` run on the solver thread. Their arguments are only
valid during the call and they must not interact with the event loop. Use
`yield_=True` and `async for` to process models on the event loop instead.

### Usage

```bash
python race.py
python timeout.py
python stream.py
python tsp.py
```
