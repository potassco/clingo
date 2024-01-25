import os
import re
import subprocess
import sysconfig

libpath = sysconfig.get_path("purelib")
clingo = os.path.join(libpath, "clingo")

subprocess.check_call(
    [
        "pybind11-stubgen",
        "-o",
        libpath,
        "--stub-extension",
        "py",
        "--enum-class-locations",
        "ProjectionMode:clingo.ast",
        "clingo",
    ]
)


with open(os.path.join(clingo, "py.typed"), "w"):
    pass

for root, dirs, files in os.walk(clingo):
    for f in files:
        if not f.endswith(".py"):
            continue
        with open(os.path.join(root, f), "r") as hnd:
            content = re.sub(
                r'^(def .*:\n    """(.|\n)*?""")',
                r"\1\n    raise RuntimeError('stub')",
                hnd.read(),
                0,
                re.M,
            )
        with open(os.path.join(root, f), "w") as hnd:
            hnd.write(content)
