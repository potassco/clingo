#!/usr/bin/env python
'''
Simple script to call conda build with the current revision and version.
'''

import subprocess
import json
import re
import os

LABEL = "dev"
CHANNELS = ["potassco", "potassco/label/dev"]
NAME = "clingo"

def get_version():
    '''
    Get the current version.
    '''
    with open('libclingo/clingo.h') as f:
        text = f.read()
    m = next(re.finditer(r'#define CLINGO_VERSION "([0-9]*\.[0-9]*\.[0-9*])"', text))
    return m.group(1)

def get_build_number(version):
    '''
    Get the next build number.
    '''
    pkgs = json.loads(subprocess.check_output(['conda', 'search', '--json', '-c', CHANNELS[-1], NAME]))

    build_number = -1
    for pkg in pkgs[NAME]:
        if pkg['channel'].find(CHANNELS[-1]) >= 0 and pkg["version"] == version:
            build_number = max(build_number, pkg['build_number'])

    return build_number + 1

def run():
    '''
    Compile and upload conda packages.
    '''
    version = get_version()
    build_number = get_build_number(version)

    build_env = os.environ.copy()
    build_env.pop("BUILD_RELEASE", None)
    build_env["VERSION_NUMBER"] = version
    build_env["BUILD_NUMBER"] = str(build_number)
    if 'GITHUB_SHA' in os.environ:
        build_env["BUILD_REVISION"] = os.environ['GITHUB_SHA']

    recipe_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'conda')
    options = ['conda', 'build', '--label', LABEL]
    for c in CHANNELS:
        options.extend(['-c', c])
    options.append(recipe_path)

    subprocess.call(options, env=build_env)

if __name__ == '__main__':
    run()
