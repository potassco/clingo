'''
Script to build pip source package.
'''

from re import finditer, escape, match, sub
from subprocess import check_call, check_output


def adjust_version():
    '''
    Adjust version in setup.py.
    '''
    pip = check_output(['curl', '-sL', 'https://test.pypi.org/simple/clingo-cffi']).decode()
    version = None
    with open('libclingo/clingo.h') as fh:
        for line in fh:
            m = match(r'#define CLINGO_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)

    assert version is not None

    post = 0
    for m in finditer(r'clingo[-_]cffi-{}\.tar\.gz'.format(escape(version)), pip):
        post = max(post, 1)

    for m in finditer(r'clingo[-_]cffi-{}\.post([0-9]+)\.tar\.gz'.format(escape(version)), pip):
        post = max(post, int(m.group(1)) + 1)

    for m in finditer(r'clingo[-_]cffi-{}\.post([0-9]+).*\.whl'.format(escape(version)), pip):
        post = max(post, int(m.group(1)))

    with open('setup.py') as fr:
        setup = fr.read()
    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))

if __name__ == "__main__":
    adjust_version()
    check_call(['cmake', '-G', 'Ninja', '-H.', '-Bbuild'])
    check_call(['cmake', '--build', 'build', '--target', 'gen'])
    check_call(['mkdir', '-p', 'libgringo/gen/src/'])
    check_call(['rsync', '-ra', 'build/libgringo/src/input', 'libgringo/gen/src/'])
    check_call(['python3', 'setup.py', 'sdist'])

