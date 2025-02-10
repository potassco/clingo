#include "statistics.hh"
#include "util.hh"

namespace Clingo::Python {

auto Statistics::as_dict() -> py::dict {
    static_cast<void>(key_);
    static_cast<void>(stats_);
    return py::dict{};
}

void register_statistics(pybind11::module &m) {
    auto stats = m.def_submodule("statistics", R"(
Functions and classes related to statistics.

Examples
--------
The following example shows how to add custom statistics and dump the
statistics in json format:

```python
>>> from json import dumps
>>> from clingo.control import Control
>>>
>>> def on_statistics(step, accu):
...     accu["example"] = 42
...
>>> ctl = Control(['--stats'])
>>> ctl.add("base", [], "{a}.")
>>> ctl.ground([("base", [])])
>>> print(ctl.solve(on_statistics=on_statistics))
SAT
>>> print(dumps(ctl.statistics['user_accu'], sort_keys=True,
...             indent=4, separators=(',', ': ')))
{
    "example": 42.0
}
>>> print(dumps(ctl.statistics['summary']['times'], sort_keys=True,
...             indent=4, separators=(',', ': ')))
{
    "cpu": 0.000785999999999995,
    "sat": 7.867813110351562e-06,
    "solve": 2.288818359375e-05,
    "total": 0.0007848739624023438,
    "unsat": 0.0
}
```

Functions and classes related to configuration.

Examples
--------
The following example shows how inpsect the configuration and modify it to
enumerate all models:
)"_d);
}

} // namespace Clingo::Python
