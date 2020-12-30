import sys
import site
from skbuild import setup

if not site.ENABLE_USER_SITE and "--user" in sys.argv[1:]:
    site.ENABLE_USER_SITE = True

setup(
    version = '5.5.0.post11',
    name = 'clingo-cffi',
    description = 'A grounder and solver for logic programs.',
    author = 'Roland Kaminski',
    license = 'MIT',
    url = 'https://github.com/potassco/clingo',
    install_requires=[ 'cffi' ],
    cmake_args=[ '-DCLINGO_INSTALL_LIB=OFF',
                 '-DCLINGO_MANAGE_RPATH=OFF',
                 '-DCLINGO_BUILD_APPS=OFF',
                 '-DCLINGO_BUILD_STATIC=OFF',
                 '-DCLINGO_BUILD_SHARED=OFF',
                 '-DCLINGO_REQUIRE_PYTHON=ON',
                 '-DCLINGO_BUILD_WITH_LUA=OFF',
                 '-DPYCLINGO_USE_CFFI=ON',
                 '-DPYCLINGO_USER_INSTALL=OFF',
                 '-DPYCLINGO_USE_INSTALL_PREFIX=ON',
                 '-DPYCLINGO_INSTALL_DIR=libpyclingo_cffi' ],
    packages=[ 'clingo' ],
    package_data={ 'clingo': [ 'py.typed' ] },
    package_dir={ '': 'libpyclingo_cffi' },
    python_requires=">=3.6"
)
