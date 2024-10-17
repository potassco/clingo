import os
import subprocess
import sysconfig

libpath = sysconfig.get_path("purelib")
clingo = os.path.join(libpath, "clingo")

subprocess.check_call(
    [
        "pybind11-stubgen",
        "-o",
        libpath,
        "clingo",
    ]
)
