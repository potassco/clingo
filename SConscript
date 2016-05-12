#!/usr/bin/python
# {{{1 GPL License

# This file is part of gringo - a grounder for logic programs.
# Copyright (C) 2013  Roland Kaminski

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# {{{1 Preamble

import os
import types
from os.path import join

# {{{1 Auxiliary functions

def find_files(env, path):
    oldcwd = os.getcwd()
    try:
        os.chdir(Dir('#').abspath)
        sources = []
        for root, dirnames, filenames in os.walk(path):
            for filename in filenames:
                if filename.endswith(".c") or filename.endswith(".cc") or filename.endswith(".cpp"):
                    sources.append(os.path.join(root, filename))
                if filename.endswith(".yy"):
                    target = os.path.join(root, filename[:-3],  "grammar.cc")
                    source = "#"+os.path.join(root, filename)
                    sources.append(target)
                    env.Bison(target, source)
                if filename.endswith(".xh"):
                    target = os.path.join(root, filename[:-3] + ".hh")
                    source = "#"+os.path.join(root, filename)
                    env.Re2c(target, source)
                if filename.endswith(".xch"):
                    target = os.path.join(root, filename[:-4] + ".hh")
                    source = "#"+os.path.join(root, filename)
                    env.Re2cCond(target, source)
        return sources
    finally:
        os.chdir(oldcwd)

def shared(env, sources):
    return [env.SharedObject(x) for x in sources]

def bison_emit(target, source, env):
    path = os.path.split(str(target[0]))[0];
    target += [os.path.join(path, "grammar.hh"), os.path.join(path, "grammar.out")]
    return target, source

def CheckBison(context):
    context.Message('Checking for bison 2.5... ')
    (result, output) = context.TryAction("${BISON} ${SOURCE} -o ${TARGET}", '%require "2.5"\n%%\nstart:', ".y")
    context.Result(result)
    return result

def CheckRe2c(context):
    context.Message('Checking for re2c... ')
    (result, output) = context.TryAction("${RE2C} ${SOURCE}", '', ".x")
    context.Result(result)
    return result

def CheckMyFun(context, name, code, header):
    source = header + "\nint main() {\n" + code + "\nreturn 0; }"
    context.Message('Checking for C++ function ' + name + '()... ')
    result = context.TryLink(source, '.cc')
    context.Result(result)
    return result

def CheckLibs(context, name, libs, header):
    context.Message("Checking for C++ library {0}... ".format(name))
    libs = [libs] if isinstance(libs, types.StringTypes) else libs
    old = context.env["LIBS"][:]
    for lib in libs:
        if os.path.isabs(lib):
            context.env.Append(LIBS=File(lib))
        else:
            context.env.Append(LIBS=lib)
    result = context.TryLink("#include <{0}>\nint main() {{ }}\n".format(header), '.cc')
    context.Result(result)
    if result == 0:
        context.env["LIBS"] = old
    return result

def CheckWithPkgConfig(context, name, versions):
    context.Message("Auto-detecting {0} ({1})... ".format(name, context.env["PKG_CONFIG"]))
    result = False
    if context.env['PKG_CONFIG'] is not None and \
       context.TryAction('${PKG_CONFIG} --atleast-pkgconfig-version=0')[0]:
        for version in versions:
            if context.TryAction('${{PKG_CONFIG}} --exists "{0}"'.format(version))[0]:
                context.env.ParseConfig('${{PKG_CONFIG}} --cflags --libs {0}'.format(version))
                result = True
                break
    context.Result(result)
    return result

def CheckPythonConfig(context):
    context.Message("Auto-detecting python ({0})... ".format(context.env['PYTHON_CONFIG']))
    if context.env['PYTHON_CONFIG'] is not None:
        result = context.TryAction('${PYTHON_CONFIG} --ldflags --includes')[0]
        if result:
            content = context.env.backtick('{0} --ldflags --includes'.format(context.env['PYTHON_CONFIG']))
            flags = []
            for option in content.split():
                if option.startswith("-I"):
                    flags.append(option)
                if option.startswith("-L"):
                    flags.append(option)
                if option.startswith("-l"):
                    flags.append(option)
            context.env.MergeFlags(' '.join(flags))
            result = context.TryLink("#include <Python.h>\nint main() { }\n", ".cc")
    else:
        result = False
    context.Result(result)
    return result

# {{{1 Basic environment

Import('env')

base_env = env.Clone()

bison_action = Action("${BISON} -r all --report-file=${str(TARGET)[:-3]}.out -o ${TARGET} ${SOURCE} ${test}")

bison_builder = Builder(
    action = bison_action,
    emitter = bison_emit,
    suffix = '.cc',
    src_suffix = '.yy'
    )

re2c_action = Action("${RE2C} -o ${TARGET} ${SOURCE}")

re2c_builder = Builder(
    action = re2c_action,
    suffix = '.hh',
    src_suffix = '.xh'
    )

re2c_cond_action = Action("${RE2C} -c -o ${TARGET} ${SOURCE}")

re2c_cond_builder = Builder(
    action = re2c_cond_action,
    suffix = '.hh',
    src_suffix = '.xch'
    )

env['ENV']['PATH'] = os.environ['PATH']
if 'LD_LIBRARY_PATH' in os.environ: env['ENV']['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH']
env['BUILDERS']['Bison'] = bison_builder
env['BUILDERS']['Re2c'] = re2c_builder
env['BUILDERS']['Re2cCond'] = re2c_cond_builder

# {{{1 Gringo specific configuration

log_file = join("build", GetOption('build_dir') + ".log")
conf = Configure(env, custom_tests={'CheckBison' : CheckBison, 'CheckRe2c' : CheckRe2c, 'CheckMyFun' : CheckMyFun, 'CheckLibs' : CheckLibs, 'CheckWithPkgConfig' : CheckWithPkgConfig, 'CheckPythonConfig' : CheckPythonConfig}, log_file=log_file)
DEFS = {}
failure = False

if not conf.CheckBison():
    print 'error: no usable bison version found'
    failure = True

if not conf.CheckRe2c():
    print 'error: no usable re2c version found'
    failure = True

if not conf.CheckCXX():
    print 'error: no usable C++ compiler found'
    print "Please check the log file for further information: " + log_file
    Exit(1)

if not conf.CheckSHCXX():
    print 'error: no usable (shared) C++ compiler found'
    print "Please check the log file for further information: " + log_file
    Exit(1)

with_python = False
if env['WITH_PYTHON'] == "auto":
    if conf.CheckPythonConfig() or \
       conf.CheckWithPkgConfig("python", ["python", "python2", "python-2.7", "python-2.6", "python-2.5", "python-2.4", "python3", "python-3.4", "python-3.3", "python-3.2", "python-3.1", "python-3.0"]):
        with_python = True
        DEFS["WITH_PYTHON"] = 1
elif env['WITH_PYTHON']:
    if not conf.CheckLibs("python", env['WITH_PYTHON'], "Python.h"):
        print 'error: python library not found'
        failure = True
    else:
        with_python = True
        DEFS["WITH_PYTHON"] = 1

with_lua = False
if env['WITH_LUA'] == "auto":
    if conf.CheckWithPkgConfig("lua", ["lua", "lua5.1", "lua-5.1", "lua5.2", "lua-5.2", "lua5.3", "lua-5.3"]):
        with_lua = True
        DEFS["WITH_LUA"] = 1
elif env['WITH_LUA']:
    if not conf.CheckLibs("lua", env['WITH_LUA'], "lua.hpp"):
        print 'error: lua library not found'
        failure = True
    else:
        with_lua = True
        DEFS["WITH_LUA"] = 1

if not conf.CheckMyFun('snprintf', 'char buf[256]; snprintf (buf,256,"");', '#include <cstdio>'):
    if conf.CheckMyFun('__builtin_snprintf', 'char buf[256]; __builtin_snprintf (buf,256,"");', '#include <cstdio>'):
        DEFS['snprintf']='__builtin_snprintf'

if not conf.CheckMyFun('vsnprintf', 'char buf[256]; va_list args; vsnprintf (buf,256,"", args);', "#include <cstdio>\n#include <cstdarg>"):
    if conf.CheckMyFun('__builtin_vsnprintf', 'char buf[256]; va_list args; __builtin_vsnprintf (buf,256,"", args);', "#include <cstdio>\n#include <cstdarg>"):
        DEFS['vsnprintf']='__builtin_vsnprintf'

if not conf.CheckMyFun('std::to_string', 'std::to_string(10);', "#include <string>"):
    DEFS['MISSING_STD_TO_STRING']=1

env = conf.Finish()
env.PrependUnique(LIBPATH=[Dir('.')])
env.Append(CPPDEFINES=DEFS)

# {{{1 Clasp specific configuration

claspEnv  = env.Clone()
claspConf = Configure(claspEnv, custom_tests = {'CheckLibs' : CheckLibs, 'CheckWithPkgConfig' : CheckWithPkgConfig}, log_file = join("build", GetOption('build_dir') + ".log"))
DEFS = {}

DEFS["WITH_THREADS"] = 0
if env['WITH_THREADS'] is not None:
    DEFS["WITH_THREADS"] = 1
    DEFS["CLASP_USE_STD_THREAD"] = 1
    if env['WITH_THREADS'] == "posix":
        # Note: configuration differs between gcc and clang here
        #       gcc needs -pthread, clang needs -lpthread
        claspConf.env.Append(CPPFLAGS=["-pthread"])
        claspConf.env.Append(LIBS=["pthread"])
    elif env['WITH_THREADS'] == "windows":
        pass # nohing to do
    else:
        print 'error: unknown thread model'
        failure = True


claspEnv = claspConf.Finish()
claspEnv.Append(CPPDEFINES=DEFS)

# {{{1 Test specific configuration

with_cppunit = False
if env['WITH_CPPUNIT']:
    testEnv  = claspEnv.Clone()
    testConf = Configure(testEnv, custom_tests = {'CheckBison' : CheckBison, 'CheckRe2c' : CheckRe2c, 'CheckLibs' : CheckLibs, 'CheckWithPkgConfig' : CheckWithPkgConfig}, log_file = join("build", GetOption('build_dir') + ".log"))
    if env['WITH_CPPUNIT'] == "auto":
        if testConf.CheckWithPkgConfig("cppunit", ["cppunit"]):
            with_cppunit = True
    elif testConf.CheckLibs("cppunit", env['WITH_CPPUNIT'], 'cppunit/TestFixture.h'):
        with_cppunit = True
    else:
        print 'error: cppunit library not found'
        failure = True
    testEnv  = testConf.Finish()

# {{{1 Check configuration

if failure:
    print "Please check the log file for further information: " + log_file
    Exit(1)

# {{{1 Opts: Library

LIBOPTS_SOURCES = find_files(env, 'libprogram_opts/src')
LIBOPTS_HEADERS = [Dir('#libprogram_opts'), Dir('#libprogram_opts/src')]

optsEnv = env.Clone()
optsEnv.Append(CPPPATH = LIBOPTS_HEADERS)

optsLib  = optsEnv.StaticLibrary('libprogram_opts', LIBOPTS_SOURCES)
optsLibS = optsEnv.StaticLibrary('libprogram_opts_shared', shared(optsEnv, LIBOPTS_SOURCES))

# {{{1 Lp: Library

LIBLP_SOURCES = find_files(env, 'liblp/src')
LIBLP_HEADERS = [Dir('#liblp'), Dir('#liblp/src')]

lpEnv = env.Clone()
lpEnv.Append(CPPPATH = LIBLP_HEADERS)

lpLib  = lpEnv.StaticLibrary('liblp', LIBLP_SOURCES)
lpLibS = lpEnv.StaticLibrary('liblp_shared', shared(lpEnv, LIBLP_SOURCES))

# {{{1 Clasp: Library

LIBCLASP_SOURCES = find_files(env, 'libclasp/src')
LIBCLASP_HEADERS = [Dir('#libclasp'), Dir('#libclasp/src')] + LIBOPTS_HEADERS + LIBLP_HEADERS

claspEnv.Append(CPPPATH = LIBCLASP_HEADERS)

claspLib  = claspEnv.StaticLibrary('libclasp', LIBCLASP_SOURCES)
claspLibS = claspEnv.StaticLibrary('libclasp_shared', shared(claspEnv, LIBCLASP_SOURCES))

# {{{1 Gringo: Library

LIBGRINGO_SOURCES = find_files(env, 'libgringo/src')
LIBGRINGO_HEADERS = [Dir('#libgringo'), 'libgringo/src'] + LIBLP_HEADERS

gringoEnv = env.Clone()
gringoEnv.Append(CPPPATH = LIBGRINGO_HEADERS)

gringoLib  = gringoEnv.StaticLibrary('libgringo', LIBGRINGO_SOURCES)
gringoLibS = gringoEnv.StaticLibrary('libgringo_shared', shared(gringoEnv, LIBGRINGO_SOURCES))

# {{{1 Clingo: Library

LIBCLINGO_SOURCES = find_files(env, 'libclingo/src')
LIBCLINGO_HEADERS = [Dir('#libclingo')] + LIBGRINGO_HEADERS + LIBCLASP_HEADERS

clingoEnv = claspEnv.Clone()
clingoEnv.Append(CPPPATH = LIBCLINGO_HEADERS)

clingoLib  = clingoEnv.StaticLibrary('libclingo', LIBCLINGO_SOURCES)
clingoLibS = clingoEnv.StaticLibrary('libclingo_shared', shared(clingoEnv, LIBCLINGO_SOURCES))

clingoSharedEnv = clingoEnv.Clone()
clingoSharedEnv.Prepend(LIBS = [gringoLibS, claspLibS, optsLibS, lpLibS])
clingoSharedLib = clingoSharedEnv.SharedLibrary('libclingo', shared(clingoEnv, LIBCLINGO_SOURCES))
clingoSharedEnv.Alias('libclingo', clingoSharedLib)

# {{{1 Clingo: C-Library

LIBCCLINGO_SOURCES = find_files(env, 'libcclingo/src')
LIBCCLINGO_HEADERS = [Dir('#libcclingo')] + LIBCLINGO_HEADERS

cclingoSharedEnv = claspEnv.Clone()
cclingoSharedEnv.Append(CPPPATH = LIBCCLINGO_HEADERS)
cclingoSharedEnv.Prepend(LIBS = ["clingo"])
cclingoSharedLib = cclingoSharedEnv.SharedLibrary('libcclingo', shared(cclingoSharedEnv, LIBCCLINGO_SOURCES))
cclingoSharedEnv.Alias('libcclingo', cclingoSharedLib)

# {{{1 Reify: Library

LIBREIFY_SOURCES = find_files(env, 'libreify/src')
LIBREIFY_HEADERS = [Dir('#libreify'), 'libreify/src'] + LIBGRINGO_HEADERS

reifyEnv = env.Clone()
reifyEnv.Append(CPPPATH = LIBREIFY_HEADERS)

reifyLib  = reifyEnv.StaticLibrary('libreify', LIBREIFY_SOURCES)
#reifyLibS = reifyEnv.StaticLibrary('libreify_shared', shared(reifyEnv, LIBREIFY_SOURCES))

# {{{1 Gringo: Program

GRINGO_SOURCES = find_files(env, 'app/gringo')

gringoProgramEnv = gringoEnv.Clone()
gringoProgramEnv.Append(CPPPATH = LIBOPTS_HEADERS)
gringoProgramEnv.Prepend(LIBS=[ gringoLib, optsLib, lpLib ])

gringoProgram = gringoProgramEnv.Program('gringo', GRINGO_SOURCES)
gringoProgramEnv.Alias('gringo', gringoProgram)

if not env.GetOption('clean'):
    Default(gringoProgram)

# {{{1 Clingo: Program

CLINGO_SOURCES = find_files(env, 'app/clingo/src')

clingoProgramEnv = claspEnv.Clone()
clingoProgramEnv.Prepend(LIBS=[ clingoLib, gringoLib, claspLib, optsLib, lpLib ])
clingoProgramEnv.Append(CPPPATH = LIBCLINGO_HEADERS)

clingoProgram  = clingoProgramEnv.Program('clingo', CLINGO_SOURCES)
clingoProgramEnv.Alias('clingo', clingoProgram)

if not env.GetOption('clean'):
    Default(clingoProgram)

# {{{1 Web: Program

WEB_SOURCES = [ clingoProgramEnv.Object(x) for x in find_files(env, 'app/clingo/src') if x != 'app/clingo/src/main.cc' ] + find_files(env, 'app/web')

webProgramEnv = claspEnv.Clone()
webProgramEnv.Prepend(LIBS=[ clingoLib, gringoLib, claspLib, optsLib, lpLib ])
webProgramEnv.Append(CPPPATH = LIBCLINGO_HEADERS + [Dir('#app/clingo/src')])

webProgram  = webProgramEnv.Program('clingo.html', WEB_SOURCES)
webProgramEnv.Alias('web', webProgram)

# {{{1 Example: Program

EXAMPLE_SOURCES = find_files(env, 'app/example')

exampleProgramEnv = claspEnv.Clone()
exampleProgramEnv.Prepend(LIBS=["clingo"])
exampleProgramEnv.Append(CPPPATH = LIBCLINGO_HEADERS)

exampleProgram  = exampleProgramEnv.Program('example', EXAMPLE_SOURCES)
exampleProgramEnv.Alias('example', exampleProgram)

# {{{1 C-Example: Program

CEXAMPLE_SOURCES = find_files(env, 'app/cexample')

cexampleProgramEnv = base_env.Clone()
cexampleProgramEnv.Prepend(LIBPATH=[Dir(".")])
cexampleProgramEnv.Prepend(LIBS=["cclingo"])
cexampleProgramEnv["LINKFLAGS"] = base_env["CLINKFLAGS"]
cexampleProgramEnv.Prepend(LINKFLAGS=["-Wl,-rpath-link=" + Dir(".").path])
cexampleProgramEnv.Append(CPPPATH = ["libcclingo"])

cexampleProgram = cexampleProgramEnv.Program('cexample', CEXAMPLE_SOURCES)
cexampleProgramEnv.Alias('cexample', cexampleProgram)

# {{{1 Reify: Program

REIFY_SOURCES = find_files(env, 'app/reify')

reifyProgramEnv = reifyEnv.Clone()
reifyProgramEnv.Prepend(LIBS=[ reifyLib, optsLib ])
reifyProgramEnv.Append(CPPPATH = LIBOPTS_HEADERS)

reifyProgram = reifyProgramEnv.Program('reify', REIFY_SOURCES)
reifyProgramEnv.Alias('reify', reifyProgram)

if not env.GetOption('clean'):
    Default(reifyProgram)

# {{{1 Lpconvert: Program

LPCONVERT_SOURCES = find_files(env, 'app/lpconvert')
LPCONVERT_HEADERS = [Dir('#libprogram_opts')]

lpconvertProgramEnv = lpEnv.Clone()
lpconvertProgramEnv.Prepend(LIBS=[ lpLib, optsLib ])
lpconvertProgramEnv.Append(CPPPATH = LPCONVERT_HEADERS)

lpconvertProgram = lpconvertProgramEnv.Program('lpconvert', LPCONVERT_SOURCES)
lpconvertProgramEnv.Alias('lpconvert', lpconvertProgram)

if not env.GetOption('clean'):
    Default(lpconvertProgram)

# {{{1 PyClingo + LuaClingo

if with_python:
    PYCLINGO_SOURCES = find_files(env, 'app/pyclingo/src')

    pyclingoEnv = clingoEnv.Clone()
    pyclingoEnv["LIBPREFIX"] = ""
    pyclingoEnv.Prepend(LIBS   = [clingoLibS, gringoLibS, claspLibS, optsLibS, lpLibS])

    pyclingo = pyclingoEnv.SharedLibrary('python/clingo.so', PYCLINGO_SOURCES)
    pyclingoEnv.Alias('pyclingo', pyclingo)
    if not env.GetOption('clean'):
        Default(pyclingo)

if with_lua:
    LUACLINGO_SOURCES = find_files(env, 'app/luaclingo/src')

    luaclingoEnv = clingoEnv.Clone()
    luaclingoEnv["LIBPREFIX"] = ""
    luaclingoEnv.Prepend(LIBS   = [clingoLibS, gringoLibS, claspLibS, optsLibS, lpLibS])

    luaclingo = luaclingoEnv.SharedLibrary('lua/clingo.so', LUACLINGO_SOURCES)
    luaclingoEnv.Alias('luaclingo', luaclingo)
    if not env.GetOption('clean'):
        Default(luaclingo)

# {{{1 Gringo: Tests

if with_cppunit and 'libgringo' in env['TESTS']:
    TEST_LIBGRINGO_SOURCES  = find_files(env, 'libgringo/tests')

    gringoTestEnv           = testEnv.Clone()
    gringoTestEnv.Append(CPPPATH = LIBGRINGO_HEADERS + LIBCLASP_HEADERS)
    gringoTestEnv.Prepend(LIBS   = [gringoLib, claspLib, lpLib])

    testGringoProgram = gringoTestEnv.Program('test_libgringo', TEST_LIBGRINGO_SOURCES)
    testGringoAlias   = gringoTestEnv.Alias('test', [testGringoProgram], testGringoProgram[0].path + (" " + GetOption("test_case") if GetOption("test_case") else ""))
    AlwaysBuild(testGringoAlias)

# {{{1 Reify: Tests

if with_cppunit and 'libreify' in env['TESTS']:
    TEST_LIBREIFY_SOURCES  = find_files(env, 'libreify/tests')

    reifyTestEnv                = testEnv.Clone()
    reifyTestEnv.Append(CPPPATH = LIBREIFY_HEADERS)
    reifyTestEnv.Prepend(LIBS   = [reifyLib])

    testReifyProgram = reifyTestEnv.Program('test_libreify', TEST_LIBREIFY_SOURCES)
    testReifyAlias   = reifyTestEnv.Alias('test', [testReifyProgram], testReifyProgram[0].path)
    AlwaysBuild(testReifyAlias)

# {{{1 Liplp: Tests

if "liblp" in env["TESTS"]:
    TEST_LIBLP_SOURCES  = find_files(env, 'liblp/tests')

    lpTestEnv                = env.Clone()
    lpTestEnv.Append(CPPPATH = LIBLP_HEADERS)
    lpTestEnv.Prepend(LIBS   = [lpLib])

    testLpProgram = lpTestEnv.Program('test_liblp', TEST_LIBLP_SOURCES)
    testLpAlias   = lpTestEnv.Alias('test', [testLpProgram], testLpProgram[0].path)
    AlwaysBuild(testLpAlias)

# {{{1 Clingo: Tests

clingoTestCommand = env.Command('clingo-test', clingoProgram, '/bin/zsh app/clingo/tests/run.sh $SOURCE' + (" -- -t8" if env["WITH_THREADS"] is not None else ""))
clingoTest        = env.Alias('test-clingo', [clingoTestCommand])
env.AlwaysBuild(clingoTest)

# {{{1 Clingo: Configure

clingoConfigure = env.Alias('configure', [])

# {{{1 Ctags

ctagsCommand = env.Command('ctags', [], 'ctags --c++-kinds=+p --fields=+imaS --extra=+q -R libgringo app')
ctagsAlias   = env.Alias('tags', [ctagsCommand])
env.AlwaysBuild(ctagsCommand)

