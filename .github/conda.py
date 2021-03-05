#!/usr/bin/env python

import subprocess
import json
import re
import os
import locale

LABEL = "dev"
CHANNELS = ["potassco", "potassco/label/dev"]
NAME = "clingo"

def get_version():
    with open('libclingo/clingo.h') as f:
        text = f.read()
    m = next(re.finditer(r'#define CLINGO_VERSION "([0-9]*\.[0-9]*\.[0-9*])"', text))
    return m.group(1)

def get_build_number(version):
    pkgs = json.loads(subprocess.check_output(['conda', 'search', '--json', '-c', CHANNELS[-1], NAME]))

    build_number = -1
    for pkg in pkgs[NAME]:
        if pkg['channel'].find(CHANNELS[-1]) >= 0 and pkg["version"] == version:
            build_number = max(build_number, pkg['build_number'])

    return build_number + 1

def compile():
    version = get_version()
    build_number = get_build_number(version)

    build_env = os.environ.copy()
    build_env.pop("BUILD_RELEASE", None)
    build_env["VERSION_NUMBER"] = version
    build_env["BUILD_NUMBER"] = str(build_number)

    options = ['conda', 'build']
    for c in CHANNELS:
        options.extend(['-c', c])

    data = subprocess.check_output(options + ['--output', '.'], env=build_env)
    files = data.decode(locale.getpreferredencoding()).splitlines()
    assert files

    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'conda'))
    subprocess.call(options + ['.'], env=build_env)

    return files

def upload(files):
    for f in files:
        print('uploading:', ['anaconda', 'upload', f, '--label', LABEL])
        #subprocess.call(['anaconda', 'upload', f, '--label', LABEL])

if __name__ == '__main__':
    upload(compile())
