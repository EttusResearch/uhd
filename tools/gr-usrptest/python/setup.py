#!/usr/bin/env python
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup


setup(name='urptest_automation',
        version='0.0.1',
        description='usrptest integration into RTS and Labview',
        packages=['usrptest_automation'],
        install_requires=['labview-automation>=15.0.0.dev1','hoplite>=15.0.0.dev1']
        )
