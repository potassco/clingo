'''
Script to build pip source package.
'''

import argparse
from re import finditer, escape, match, sub, search
from subprocess import check_call, check_output
from os.path import exists

def adjust_version(url):
    '''
    Adjust version in setup.py.
    '''
    if exists('setup.cfg'):
        setup_type = 'cfg'
        setup_path = 'setup.cfg'
    else:
        setup_type = 'py'
        setup_path = 'setup.py'

    with open(setup_path) as fr:
        setup = fr.read()

    if setup_type == 'cfg':
        package_name = search(r'name[ ]*=[ ]*(.*)[ ]*', setup).group(1)
        package_regex = package_name.replace('-', '[-_]')
    else:
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
    for m in finditer(r'{}-{}\.tar\.gz'.format(package_regex, escape(version)), pip):
        post = max(post, 1)

    for m in finditer(r'{}-{}\.post([0-9]+)\.tar\.gz'.format(package_regex, escape(version)), pip):
        post = max(post, int(m.group(1)) + 1)

    for m in finditer(r'{}-{}\.post([0-9]+).*\.whl'.format(package_regex, escape(version)), pip):
        post = max(post, int(m.group(1)))

    with open(setup_path, 'w') as fw:
        if setup_type == 'cfg':
            if post > 0:
                fw.write(sub('version( *)=.*', 'version = {}.post{}'.format(version, post), setup, 1))
            else:
                fw.write(sub('version( *)=.*', 'version = {}'.format(version), setup, 1))
        else:
            if post > 0:
                fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
            else:
                fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))

def run():
    parser = argparse.ArgumentParser(description='Build source package.')
    parser.add_argument('--release', action='store_true', help='Build release package.')
    args = parser.parse_args()
    if args.release:
        url = 'https://pypi.org/simple'
    else:
        url = 'https://test.pypi.org/simple'

    adjust_version(url)
    check_call(['cmake', '-G', 'Ninja', '-H.', '-Bbuild'])
    check_call(['cmake', '--build', 'build', '--target', 'gen'])
    check_call(['mkdir', '-p', 'libgringo/gen/src/'])
    check_call(['rsync', '-ra', 'build/libgringo/src/input', 'libgringo/gen/src/'])
    check_call(['python3', 'setup.py', 'sdist'])

if __name__ == "__main__":
    run()
