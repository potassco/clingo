try:
    from setuptools.sysconfig import get_python_lib, get_config_vars
except ImportError:
    from distutils.sysconfig import get_python_lib, get_config_vars
import sys

if sys.argv[1] == "prefix":
    print(get_python_lib(True, False, sys.argv[2] if len(sys.argv) > 2 else None))
elif sys.argv[1] == "suffix":
    SO, SOABI, EXT_SUFFIX = get_config_vars("SO", "SOABI", "EXT_SUFFIX")
    if EXT_SUFFIX is not None:
        ext = EXT_SUFFIX
    elif SOABI is not None:
        ext = ''.join('.', SOABI, SO)
    else:
        ext = SO
    print(ext)
