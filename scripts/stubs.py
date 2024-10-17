import os
import subprocess
import sysconfig
from glob import glob

libpath = sysconfig.get_path("purelib")
clingo = os.path.join(libpath, "clingo")

env = os.environ.copy()
env["PYTHONPATH"] = "./build/debug/lib/python-api"
subprocess.check_call(
    [
        "pybind11-stubgen",
        "-o",
        libpath,
        "clingo",
    ],
    env=env,
)

try:
    for lib in glob("./build/debug/lib/python-api/clingo*.so"):
        name = os.path.basename(lib)
        src = os.path.relpath(lib, clingo)
        dst = os.path.relpath(os.path.join(clingo, name), ".")
        os.symlink(src, dst)
except FileExistsError:
    pass

# better ideas are welcome!
init = os.path.join(libpath, "clingo", "__init__.py")
with open(init, "w") as hnd:
    hnd.write(
        """\
import sys
from . import clingo as _clingo

# we remap the included modules
for key in list(sys.modules.keys()):
    if key.startswith("clingo.clingo"):
        mod = sys.modules[key]
        del sys.modules[key]
        sys.modules[key[7:]] = mod
"""
    )

typed = os.path.join(libpath, "clingo", "py.typed")
with open(typed, "w") as hnd:
    hnd.write("\n")
