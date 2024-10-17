"""
Install the package and stubs into the virtual environment `.venv`.
"""

import os
import subprocess
import sysconfig
from glob import glob

# https://typing.readthedocs.io/en/latest/spec/distributing.html

libpath = sysconfig.get_path("purelib")
clingo_stubs = os.path.join(libpath, "clingo-stubs")

env = os.environ.copy()
env["PYTHONPATH"] = "./build/debug/lib/python-api"
subprocess.check_call(
    [
        "pybind11-stubgen",
        "--root-suffix=-stubs",
        "-o",
        libpath,
        "clingo",
    ],
    env=env,
)

try:
    for lib in glob("./build/debug/lib/python-api/clingo*.so"):
        name = os.path.basename(lib)
        src = os.path.relpath(lib, libpath)
        dst = os.path.relpath(os.path.join(libpath, name), ".")
        os.symlink(src, dst)
except FileExistsError:
    pass
