from distutils.core import setup, Extension
import os

path = os.path.normpath(os.path.abspath(os.path.join(os.getcwd(), "build", "release", "bin")))

os.environ["LDFLAGS"] = "-Wl,-rpath={}".format(path)

module = Extension('clingo',
                   sources = ['app/pyclingo/src/main.cc'],
                   include_dirs = ['libclingo', 'libpyclingo'],
                   libraries = ['pyclingo', 'clingo'],
                   library_dirs = ['build/release/bin'],
                   )

setup (name = 'clingo',
       version = '5.2.0',
       description = 'The clingo python module',
       ext_modules = [module])
