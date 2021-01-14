'''
Script to build binary wheels on windows.
'''

from re import finditer, escape, match, sub
from subprocess import check_output, check_call
from urllib.request import urlopen

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
    adjust_version()
    check_call(['pip', 'wheel', './', '-w', 'wheelhouse'])
