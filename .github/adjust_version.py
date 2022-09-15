'''
Script to build binary wheels in manylinux 2014 docker container.
'''

import argparse
from re import finditer, escape, match, sub, search
from subprocess import check_output, check_call, CalledProcessError
from os import unlink, environ, path, mkdir
from glob import glob

ARCH = check_output(['uname', '-m']).decode().strip()
print(ARCH)


def adjust_version(url, source):
    '''
    Adjust version in setup.py.
    '''
    with open('setup.py') as fr:
        setup = fr.read()

    package_name = search(r'''name[ ]*=[ ]*['"]([^'"]*)['"]''', setup).group(1)
    package_regex = package_name.replace('-', '[-_]')

    pip = check_output(['curl', '-sL', '{}/{}'.format(url, package_name)]).decode()
    version = None
    with open('libclingo/clingo.h') as fh:
        for line in fh:
            m = match(r'#define CLINGO_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)
    assert version is not None

    post = 0
    for m in finditer(r'{}-{}\.post([0-9]+)\.tar\.gz'.format(package_regex, escape(version)), pip):
        post = max(post, int(m.group(1)) + (1 if source else 0))


    print(f"version: {version}")
    print(f"post: {post}")
    return

    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))


def run():
    parser = argparse.ArgumentParser(description='Build source package.')
    parser.add_argument('--release', action='store_true', help='Build release package.')
    parser.add_argument('--source', action='store_true', help='Adjust version for source package.')
    args = parser.parse_args()

    if args.release:
        url = 'https://pypi.org/simple'
        idx = None
    else:
        url = 'https://test.pypi.org/simple'
        idx = 'https://test.pypi.org/simple/'

    adjust_version(url, args.source)


if __name__ == "__main__":
    run()
