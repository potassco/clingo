'''
Script to build binary wheels in manylinux 2014 docker container.
'''

from re import finditer, escape, match, sub
from subprocess import check_output, check_call, CalledProcessError
from os import unlink, environ, path, mkdir
from glob import glob

ARCH = check_output(['uname', '-m']).decode().strip()


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
    for m in finditer(r'clingo[_-]cffi-{}\.post([0-9]+)\.tar\.gz'.format(escape(version)), pip):
        post = max(post, int(m.group(1)))

    for m in finditer(r'clingo[_-]cffi-{}.*manylinux2014_{}'.format(escape(version), escape(ARCH)), pip):
        post = max(post, 1)

    for m in finditer(r'clingo[_-]cffi-{}\.post([0-9]+).*manylinux2014_{}'.format(escape(version), escape(ARCH)), pip):
        post = max(post, int(m.group(1)) + 1)

    with open('setup.py') as fr:
        setup = fr.read()
    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))


def install():
    '''
    Install required software.
    '''
    if ARCH == "ppc64le":
        mkdir('re2c_source')
        check_call(['curl', '-LJ', '-o', 're2c.tar.gz', 'https://github.com/skvadrik/re2c/archive/2.0.3.tar.gz'])
        check_call(['tar', 'xzf', 're2c.tar.gz', '-C', 're2c_source', '--strip-components=1'])
        check_call(['cmake', '-G', 'Ninja', '-Hre2c_source', '-Bre2c_build', '-DRE2C_BUILD_RE2GO=OFF'])
        check_call(['cmake', '--build', 're2c_build', '--target', 'install'])
    else:
        check_call(['yum', 'install', '-y', 're2c', 'bison'])


def compile_wheels():
    '''
    Compile binary wheels for different python versions.
    '''
    for pybin in glob('/opt/python/*/bin'):
        # Requires Py3.6 or greater - on the docker image 3.5 is cp35-cp35m
        if "35" not in pybin:
            check_call([path.join(pybin, 'pip'), 'wheel', './', '--no-deps', '-w', 'wheelhouse/'])


def repair_wheels():
    '''
    Bundle external shared libraries into the wheels.
    '''
    for wheel in glob('wheelhouse/*.whl'):
        try:
            check_call(['auditwheel', 'show', wheel])
        except CalledProcessError:
            print("Skipping non-platform wheel {}".format(wheel))
            continue

        check_call(['auditwheel', 'repair', wheel, '--plat', environ['PLAT'], '-w', 'wheelhouse/'])
        unlink(wheel)


if __name__ == "__main__":
    install()
    adjust_version()
    compile_wheels()
    repair_wheels()
