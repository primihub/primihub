import glob
import os
import shlex
import subprocess
import sys
from distutils.sysconfig import get_python_lib
from os import path
from shutil import copy
import site
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

print(sys.executable)

def is_pkg(line):
    return line and not line.startswith(('--', 'git', '#'))


with open(path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()


# class PostDevelopCommand(develop):
#     """Post-installation for development mode."""

#     def run(self):
#         # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
#         develop.run(self)


# class PostInstallCommand(install):
#     """Post-installation for installation mode."""

#     def run(self):
#         # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
#         install.run(self)


with open('requirements.txt', encoding='utf-8') as reqs:
    install_requires = [l for l in reqs.read().split('\n') if is_pkg(l)]
    print("install requires: ", install_requires)


def add_primihub_so_site():
    # add ../bazel-bin to PYTHONPATH on LINUX
    print("ADD ../bazel-bin to PYTHONPATH on LINUX")
    SO_LIB_PATH = path.abspath(path.join(path.dirname(__file__), "../bazel-bin"))  # noqa
    print("* " * 30)
    site_packages_path = get_python_lib()
    print(site_packages_path)
    pth_file = site_packages_path + "/primihub_so.pth"
    print(pth_file) 
    try:
        with open(pth_file, "w") as pth:
            print("--", SO_LIB_PATH)
            pth.write(SO_LIB_PATH)
        site.addsitedir(SO_LIB_PATH)
        print("sys path: ", sys.path)
    except Exception as e:
        print(e)
        


def compile_proto():
    print("compile proto...")
    print(os.getcwd())
    os.chdir("..")
    print(os.getcwd())
    cmd = shlex.split("""{pyexe} -m grpc_tools.protoc \
                        --python_out=python/primihub/client/ph_grpc \
                        --grpc_python_out=python/primihub/client/ph_grpc -I. \
                        src/primihub/protos/common.proto  \
                        src/primihub/protos/pir.proto \
                        src/primihub/protos/psi.proto \
                        src/primihub/protos/service.proto \
                        src/primihub/protos/worker.proto """.format(pyexe=sys.executable))
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        raise RuntimeError("Error compiling proto file: {0}".format(err))
    os.chdir("python")
    print(os.getcwd())


class PostDevelopCommand(develop):
    """Post-installation for development mode."""

    def run(self):
        # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
        develop.run(self)
        add_primihub_so_site()
        compile_proto()


class PostInstallCommand(install):
    """Post-installation for installation mode."""

    def run(self):
        # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
        install.run(self)
        add_primihub_so_site()
        compile_proto()


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
