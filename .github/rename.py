'''
Simple helper module to be used in scripts building pip packages that depend on
clingo.
'''
import re
from os.path import isfile

def rename_clingo_cffi():
    '''
    Replace all occurences of 'clingo-cffi' by 'clingo' pip package files.
    '''
    for n in ('setup.py', 'pyproject.toml'):
        if not isfile(n):
            continue
        with open(n, 'r+') as f:
            content = re.sub(r'''(['"])clingo-cffi(['"])''', r'\1clingo\2', f.read())
            f.seek(0)
            f.write(content)
            f.truncate()

if __name__ == '__main__':
    rename_clingo_cffi()
