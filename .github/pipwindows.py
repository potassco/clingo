'''
Script to build binary wheels on windows.
'''

from re import finditer, escape, match, sub
from subprocess import check_call
from urllib.request import urlopen
from os import environ
from glob import glob

def setup():
    with open('twine.cfg', 'w') as fh:
        fh.write('''\
[distutils]
index-servers=clingo-cffi
[clingo-cffi]
repository = https://test.pypi.org/legacy/
username=__token__
password={}
'''.format(environ['PYPI_API_TOKEN']))

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
        post = max(post, int(m.group(1)))

    for m in finditer(r'clingo[_-]cffi-{}.post([0-9]+).*win'.format(escape(version)), pip):
        post = max(post, int(m.group(1)) + 1)

    with open('setup.py') as fr:
        setup = fr.read()
    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))


if __name__ == "__main__":
    setup()
    adjust_version()
    check_call(['python', 'setup.py', 'bdist_wheel'])
    for wheel in glob('dist/*.whl'):
        check_call(['python', '-m', 'twine', '--config-file', "twine.cfg", '--repository', 'clingo-cffi', wheel])
