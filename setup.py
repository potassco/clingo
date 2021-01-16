import sys
import site
from textwrap import dedent
from skbuild import setup

if not site.ENABLE_USER_SITE and "--user" in sys.argv[1:]:
    site.ENABLE_USER_SITE = True

setup(
    version = '5.5.0',
    name = 'clingo-cffi',
    description = 'CFFI-based bindings to the clingo solver.',
    long_description = dedent('''\
        This package provides CFFI-based bindings to the clingo solver.

        Clingo is part of the [Potassco](https://potassco.org) project for *Answer Set Programming* (ASP).
        ASP offers a simple and powerful modeling language to describe combinatorial problems as *logic programs*.
        The *clingo* system then takes such a logic program and computes *answer sets* representing solutions to the given problem.
        To get an idea, check our [Getting Started](https://potassco.org/doc/start/) page and the [online version](https://potassco.org/clingo/run/) of clingo.

        Temporarily, the API documentation of this project can be found [here](https://www.cs.uni-potsdam.de/~kaminski/pyclingo-cffi/).
        '''),
    long_description_content_type='text/markdown',
    author = 'Roland Kaminski',
    author_email = 'kaminski@cs.uni-potsdam.de',
    license = 'MIT',
    url = 'https://github.com/potassco/clingo',
    install_requires=[ 'cffi' ],
    cmake_args=[ '-DCLINGO_INSTALL_LIB=OFF',
                 '-DCLINGO_MANAGE_RPATH=OFF',
                 '-DCLINGO_BUILD_APPS=OFF',
                 '-DCLINGO_BUILD_STATIC=OFF',
                 '-DCLINGO_BUILD_SHARED=ON',
                 '-DCLINGO_BUILD_SHARED_INTERFACE=ON',
                 '-DCLINGO_REQUIRE_PYTHON=ON',
                 '-DCLINGO_BUILD_WITH_LUA=OFF',
                 '-DPYCLINGO_USE_CFFI=ON',
                 '-DPYCLINGO_USER_INSTALL=OFF',
                 '-DPYCLINGO_USE_INSTALL_PREFIX=ON',
                 '-DPYCLINGO_FORCE_OLD_MODULE=ON',
                 '-DPYCLINGO_INSTALL_DIR=libpyclingo_cffi' ],
    packages=[ 'clingo' ],
    package_data={ 'clingo': [ 'py.typed', 'import__clingo.lib' ] },
    package_dir={ '': 'libpyclingo_cffi' },
    python_requires=">=3.6"
)
