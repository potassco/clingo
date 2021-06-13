'''
Simple helper module to be used in scripts building pip packages that depend on
clingo.
'''
import re
from os.path import isfile

def rename_clingo_cffi():
    '''
    Replace all occurences of 'clingo' package by 'clingo-cffi' in pip package files.
    '''
    for n in ('setup.py', 'pyproject.toml'):
        if not isfile(n):
            continue
        with open(n, 'r+') as f:
            content = ""
            for line in f:
                if re.match(r'^ *(install_requires|requires) *= *\[', line) or re.match(r'''^ *name *= *['"]''', line):
                    content += re.sub(r'''(['"])clingo(['"])''', r'\1clingo-cffi\2', line)
                else:
                    content += line
            f.seek(0)
            f.write(content)
            f.truncate()

if __name__ == '__main__':
    rename_clingo_cffi()
