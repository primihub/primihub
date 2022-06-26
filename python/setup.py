import glob
import os
import sys
from distutils.sysconfig import get_python_lib
from os import path
from shutil import copy

import primihub
from setuptools import setup, find_packages
from setuptools.command.develop import develop
from setuptools.command.install import install

here = path.abspath(path.dirname(__file__))

"""
##### How #####
For Dev:
python setup.py develop
For Prod:
python setup.py install
"""


def is_pkg(line):
    return line and not line.startswith(('--', 'git', '#'))


with open(path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()


class PostDevelopCommand(develop):
    """Post-installation for development mode."""

    def run(self):
        # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
        develop.run(self)


class PostInstallCommand(install):
    """Post-installation for installation mode."""

    def run(self):
        # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
        install.run(self)


with open('requirements.txt', encoding='utf-8') as reqs:
    install_requires = [l for l in reqs.read().split('\n') if is_pkg(l)]


class PostDevelopCommand(develop):
    """Post-installation for development mode."""

    def run(self):
        # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
        develop.run(self)
        SO_LIB = path.abspath(path.join(path.dirname(__file__), "../bazel-bin/*.so"))  # noqa
        print("* " * 30)
        target = get_python_lib()
        for f in glob.glob(SO_LIB):

            dest = os.path.join(target, os.path.basename(f))
            try:
                os.remove(dest)
            except:
                pass

            try:
                print("cp {} to {}".format(f, dest))
                copy(f, target)
            except:
                print("Failed to copy file: ", sys.exc_info())
        print("* " * 30)


class PostInstallCommand(install):
    """Post-installation for installation mode."""

    def run(self):
        # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
        install.run(self)


setup(
    name='primihub',
    version=primihub.__version__,
    description=long_description,
    author=primihub.__author__,
    author_email=primihub.__contact__,
    url=primihub.__homepage__,
    packages=find_packages(),
    install_requires=install_requires,
    # package_data={
    #     '': [
    #         '*.yaml', 
    #         '*.yml',
    #         'schema/*'
    #     ], 
    #     'primihub.tests': [ 
    #         '*',
    #         'demo/*' 
    #         'formula/*',
    #         'swagger_ref/*',
    #         'wireload/*',
    #         'component_template/*'
    #     ]
    # },    
    include_package_data=True,
    cmdclass={
        'develop': PostDevelopCommand,
        'install': PostInstallCommand,
    },
    entry_points={
        # 'console_scripts': [
        #     'edge=primihub.__main__:main'
        # ]
    },

)
