#!/usr/bin/env python
'''
Simple script to call conda build with the current revision and version.
'''

import argparse
import subprocess
import json
from re import match
import os

NAME = 'clingo'

def get_build_number(channels, version):
    '''
    Get the next build number.
    '''
    pkgs = json.loads(subprocess.check_output(['conda', 'search', '--json', '-c', channels[-1], NAME]))

    build_number = -1
    for pkg in pkgs[NAME]:
        if pkg['channel'].find(channels[-1]) >= 0 and pkg["version"] == version:
            build_number = max(build_number, pkg['build_number'])

    return build_number + 1

def run():
    '''
    Compile and upload conda packages.
    '''

    parser = argparse.ArgumentParser(description='Build conda packages.')
    parser.add_argument('--release', action='store_true', help='Build release packages.')
    args = parser.parse_args()
    if args.release:
        label = None
        channels = ['potassco', 'potassco/label/dev']
    else:
        label = "dev"
        channels = ['potassco']

    version = None
    with open('libclingo/clingo.h') as fh:
        for line in fh:
            m = match(r'#define CLINGO_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)
    build_number = get_build_number(channels, version)

    build_env = os.environ.copy()
    build_env.pop("BUILD_RELEASE", "1" if args.release else None)
    build_env["VERSION_NUMBER"] = version
    build_env["BUILD_NUMBER"] = str(build_number)
    if 'GITHUB_SHA' in os.environ:
        build_env["BUILD_REVISION"] = os.environ['GITHUB_SHA']

    recipe_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'conda')
    options = ['conda', 'build']
    if label is not None:
        options.extend(['--label', label])

    for c in channels:
        options.extend(['-c', c])
    options.append(recipe_path)

    subprocess.call(options, env=build_env)

if __name__ == '__main__':
    run()
