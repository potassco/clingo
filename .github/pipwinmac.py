'''
Script to build binary wheels on windows.

TODO:
- version detection has to take python version into account
- this script should mac binaries too
'''

from re import finditer, escape, match, sub
from subprocess import check_call
from urllib.request import urlopen
from glob import glob
from platform import python_implementation
from distutils.util import get_platform
from sysconfig import get_config_var

NAMES = {
    "cpython": "cp",
    "pypy": "pp",
}

def python_tag():
    name = NAMES[python_implementation().lower()]
    return '{}{}'.format(name, get_config_var("py_version_nodot"))

def platform_tag():
    return get_platform().replace("-", "_").replace(".", "_")

def adjust_version():
    '''
    Adjust version in setup.py.
    '''
    with urlopen('https://test.pypi.org/simple/clingo-cffi') as uh:
        pip = uh.read().decode()

    version = None
    with open('libclingo/clingo.h') as fh:
        for line in fh:
            m = match(r'#define CLINGO_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)

    assert version is not None

    post = 0
    for m in finditer(r'clingo[_-]cffi-{}.post([0-9]+).tar.gz'.format(escape(version)), pip):
        print('source', m)
        post = max(post, int(m.group(1)))

    for m in finditer(r'clingo[_-]cffi-{}-{}-.*-{}'.format(escape(version), escape(python_tag()), escape(platform_tag())), pip):
        print('init', m)
        post = max(post, 1)

    for m in finditer(r'clingo[_-]cffi-{}.post([0-9]+)-{}-.*-{}'.format(escape(version), escape(python_tag()), escape(platform_tag())), pip):
        print('update', m)
        post = max(post, int(m.group(1)) + 1)

    with open('setup.py') as fr:
        setup = fr.read()
    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))

    print('the current version post is: ', post)


if __name__ == "__main__":
    adjust_version()
    #check_call(['python', 'setup.py', 'bdist_wheel'])
    #for wheel in glob('dist/*.whl'):
    #    check_call(['python', '-m', 'twine', 'upload', wheel])
