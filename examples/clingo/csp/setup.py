#!/usr/bin/env python

from setuptools import setup

setup(
    version='0.1',
    name='csp',
    description='An answer set solver for constraint logic programs.',
    author='Roland Kaminski, Max Ostrowski',
    license='MIT',
    packages=['csp'],
    test_suite='csp.tests',
    zip_safe=False,
    entry_points={
        'console_scripts': [
            'csp=csp:main',
        ]
    }
)
