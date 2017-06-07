# Table of Contents

- [Requirements](#requirements)
  - [Development Dependencies](#development-dependencies)
  - [Optional Dependencies](#optional-dependencies)
- [Build, Install, and Test](#build-install-and-test)
  - [Build Options](#build-options)
    - [Generic Options](#generic-options)
    - [Python Support](#python-support)
    - [Lua Support](#lua-support)
- [Troubleshooting](#troubleshooting)
  - [Notes for Windows Users](#notes-for-windows-users)

# Requirements

- a c++14 conforming compiler
  - *at least* [gcc](https://gcc.gnu.org/) version 4.9
  - [clang](http://clang.llvm.org/) version 3.1 (using either libstdc++
    provided by gcc 4.9 or libc++)
  - *at least* msvc++ 14.0 ([Visual Studio](https://www.visualstudio.com/) 2015
    Update 3)
  - other compilers might work
- the [cmake](https://www.cmake.org/) build system
  - at least version 3.3 is recommended
  - at least version 3.1 is *required*

## Development Dependencies

The following dependencies are only required when compiling a development
branch. Releases already include the necessary generated files.

- the [bison](https://www.gnu.org/software/bison/) parser generator
  - *at least* version 2.5
  - version 3.0 produces harmless warnings
    (to stay backwards-compatible)
- the [re2c]() lexer generator
  - version 0.15.3 is used for development
  - the earliest tested version is 0.13.5

## Optional Dependencies

- the [Python](https://www.python.org/) script language
  - version 2.7 is tested
- the [Lua](https://www.lua.org/) script language
  - version 5.1 is used for development
  - version 5.2 and 5.3 should work

# Build, Install, and Test

When cloning the git repository, do not forget to update the submodules (with
source releases, you can skip this step):

    git submodule update --init --recursive

To build gringo, clingo, and reify in their default configurations in release
mode, run:

    cmake -H<SOURCE_DIR> -B<BUILD_DIR> -DCMAKE_BUILD_TYPE=Release
    cmake --build <BUILD_DIR>

The resulting binaries and shared libraries will be in `<BUILD_DIR>/bin` and
are ready to use.

To install all binaries and development files under cmake's install
prefix (see the [build options](#build-options)), run:

    cmake --build <BUILD_DIR> --target install

To run the tests, enable option `CLINGO_BUILD_TESTS` (see [build
options](#build-options)) and run:

    cmake --build <BUILD_DIR> --target test

## Build Options

Cmake's `-L` option can be used to get an overview over the variables that can
be set for building gringo/clingo. To get gringo/clingo specific options, run

    cmake -H<SOURCE_DIR> -B<BUILD_DIR> -DCMAKE_BUILD_TYPE=Release -LH
    
or, to also print important cmake specific configuration variables

    cmake -H<SOURCE_DIR> -B<BUILD_DIR> -DCMAKE_BUILD_TYPE=Release -LAH

Options and variables can be passed to
cmake on the command line using `-D<VARIABLE>=<VALUE>` or by editing
`<BUILD_DIR>/CMakeCache.txt` after running cmake.

The build scripts by default try to detect optional dependencies, like Python
and Lua scripting support.

Clingo uses [libpotassco](https://github.com/potassco/libpotassco) and
[clasp](https://github.com/potassco/potassco).  Both components have their own
sets of configuration variables:
- [building libpotassco](https://github.com/potassco/libpotassco#installation)
- [building clasp](https://github.com/potassco/clasp#building--installing)

In the following, the most important options to control the build are listed.

### Generic Options

- Variable `CMAKE_BUILD_TYPE` should be set to `Release`.
- Variable `CMAKE_INSTALL_PREFIX` controls where to install clingo.
- Option `CLINGO_BUILD_APPS` controls whether to build the applications gringo,
  clingo, and reify.
  (Default: `ON`)
- Option `CLINGO_BUILD_EXAMPLES` controls whether to build the clingo API
  examples.
  (Default: `OFF`)
- Option `CLINGO_BUILD_TESTS` controls whether to build the clingo tests and
  enable the test target running unit as well as acceptance tests.
  (Default: `OFF`)
- Option `CLINGO_MANAGE_RPATH` controls how to find libraries on platforms
  where this is supported, like Linux, macOS, or BSD but not Windows. This
  option should be enabled if clingo is installed in a non-default location,
  like the users home directory; otherwise it has no effect.
  (Default: `ON`)

### Python Support

With the default configuration, Python support will be auto-detected if the
Python development packages are installed.

- Option `CLINGO_BUILD_WITH_PYTHON` can be used to enable or disable Python
  support.
  (Default: `ON`)
- If option `CLINGO_REQUIRE_PYTHON` is enabled, configuration will fail if no
  Python support is detected; otherwise, Python support will simply be disabled
  if not detected.
  (Default: `OFF`)
- If option `PYCLINGO_USER_INSTALL` is enabled, the clingo Python module is
  installed in the users home directory; otherwise it is installed in the
  system's Python library directory.
  (Default: `ON`)
- Variable `PYCLINGO_INSTALL_DIR` can be used to customize where to install the
  python module.
  (Default: automatically detected)

Note that it can happen that the found Python interpreter does not match the
found Python libraries if the development headers for the interpreter are not
installed. Make sure to install them before running cmake (or remove or adjust
the `CMakeCache.txt` file). It is also possible to explicitely select a Python
installation by pointing the variable `PYTHON_EXECUTABLE` to the desired Python
interpreter.

### Lua Support

With the default configuration, Lua support will be auto-detected if the Lua
development packages are installed.

- Option `CLINGO_BUILD_WITH_LUA` can be used to enable or disable Lua support.
  (Default: `ON`)
- If option `CLINGO_REQUIRE_LUA` is enabled, configuration will fail if no Lua
  support is detected; otherwise, Lua support will simply be disabled if not
  detected. (Default: `OFF`)
- If variable `LUACLINGO_INSTALL_DIR` is set, the clingo lua module will be
  installed there.
  (Default: not set)

# Troubleshooting

After installing the required packages clingo should compile on most \*nixes.
If a dependency is missing or a software version too old, then there are
typically community repositories that provide the necessary packages. To list a
few:
- the [ToolChain](https://wiki.ubuntu.com/ToolChain) repository for Ubuntu
  14.04 and earlier (later versions should include all required packages)
- the [Developer
  Toolset](https://wiki.centos.org/SpecialInterestGroup/SCLo/CollectionsList)
  for CentOS
- the [Cygwin](http://cygwin.org) project under Windows (re2c must be compiled
  by hand)
- both [Homebrew](https://brew.sh/) and [MacPorts](https://www.macports.org/)
  provide all the software necessary to compile clingo

And, well, you can compile a recent gcc version yourself. Even on ancient Linux
systems. ;)

## Notes for Windows Users

clingo can be compiled using the
[Mingw-w64](https://mingw-w64.sourceforge.net/) compiler, the Cygwin project,
or Visual Studio 2015 Update 3. For development,
[bison](http://cs.uni-potsdam.de/~kaminski/win_flex_bison-latest.zip) from the
[Win flex-bison](https://sourceforge.net/projects/winflexbison/) project and a
self compiled [re2c](http://cs.uni-potsdam.de/~kaminski/re2c.exe) executable
can be used.
