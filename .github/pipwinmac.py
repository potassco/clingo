'''
Script to build binary wheels on windows.
'''

from re import finditer, escape, match, sub, search
from subprocess import check_call
from urllib.request import urlopen
from glob import glob
from platform import python_implementation
from sysconfig import get_config_var
from os import environ, pathsep
import os
import argparse

from rename import rename_clingo_cffi

NAMES = {
    "cpython": "cp",
    "pypy": "pp",
}

def python_tag():
    name = NAMES[python_implementation().lower()]
    return '{}{}'.format(name, get_config_var("py_version_nodot"))

def platform_tag():
    return 'macosx' if os.name == 'posix' else 'win'

def adjust_version(url):
    '''
    Adjust version in setup.py.
    '''
    with open('setup.py') as fr:
        setup = fr.read()

    package_name = search(r'''name[ ]*=[ ]*['"]([^'"]*)['"]''', setup).group(1)
    package_regex = package_name.replace('-', '[-_]')

    version = None
    with open('libclingo/clingo.h') as fh:
        for line in fh:
            m = match(r'#define CLINGO_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)
    assert version is not None

    with urlopen('{}/{}'.format(url, package_name)) as uh:
        pip = uh.read().decode()
    post = 0
    for m in finditer(r'{}-{}\.post([0-9]+)\.tar\.gz'.format(package_regex, escape(version)), pip):
        post = max(post, int(m.group(1)))

    for m in finditer(r'{}-{}-{}-.*-{}'.format(package_regex, escape(version), escape(python_tag()), escape(platform_tag())), pip):
        post = max(post, 1)

    for m in finditer(r'{}-{}\.post([0-9]+)-{}-.*-{}'.format(package_regex, escape(version), escape(python_tag()), escape(platform_tag())), pip):
        post = max(post, int(m.group(1)) + 1)

    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))

def run():
    parser = argparse.ArgumentParser(description='Build source package.')
    parser.add_argument('--release', action='store_true', help='Build release package.')
    args = parser.parse_args()

    if args.release:
        rename_clingo_cffi()
        url = 'https://pypi.org/simple'
        idx = None
    else:
        url = 'https://test.pypi.org/simple'
        idx = 'https://test.pypi.org/simple'

    adjust_version(url)

    if os.name == 'posix':
        environ['PATH'] = '/usr/local/opt/bison/bin' + pathsep + environ["PATH"]
        environ['MACOSX_DEPLOYMENT_TARGET'] = '10.9'
    args = ['pip', 'wheel', '-v', '--no-deps', '-w', 'dist']
    if idx is not None:
        args.extend(['--extra-index-url', idx])
    args.extend(['./'])
    check_call(args)

    for wheel in glob('dist/*.whl'):
        check_call(['python', '-m', 'twine', 'upload', wheel])

if __name__ == "__main__":
    run()
