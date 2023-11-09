import os
import sys
import shlex
import subprocess
import shutil
import sys
import distutils
import tempfile
from distutils.sysconfig import get_python_lib
from os import path
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


with open('requirements.txt', encoding='utf-8') as reqs:
    install_requires = [l for l in reqs.read().split('\n') if is_pkg(l)]
    print("install requires: ", install_requires)


def solib2sitepackage(solib_path=None):
    if '--user' in sys.argv:
        paths = site.getusersitepackages()
    else:
        paths = get_python_lib()
    module_map = {}
    py_so_root_path = "../bazel-bin/src/primihub/pybind_warpper"
    module_list = ["opt_paillier_c2py.so", "linkcontext.so"]
    for module_name in module_list:
      module_map[module_name] = f"{py_so_root_path}/{module_name}"
    module_list = ["ph_secure_lib.so"]
    so_root_path = "../bazel-bin/src/primihub/task/pybind_wrapper"
    for module_name in module_list:
      module_map[module_name] = f"{so_root_path}/{module_name}"
    for module_name, module_path in module_map.items():
        module_installed = False
        if os.path.isfile(module_path):
            shutil.copyfile(module_path, "{}/{}".format(paths, module_name))
            print("Install {} finish, copy file from: {}.".format(module_name, module_path))
            module_installed = True
        else:
            print("Can't not find file {}, try to find ./{}.".format(module_path, module_name))
        if module_installed:
            continue
        if os.path.isfile("./{}".format(module_name)):
            shutil.copyfile("./{}".format(module_name),
                    paths + "/{}".format(module_name))
            print("Install {} finish, file found in './'.".format(module_name))
        else:
            print("Can't not find file ./{}.".format(module_name))
            print("Ignore {} due to file not found.".format(module_name))


def clean_proto():
    os.chdir("../")
    python_out_dir = "python/primihub/client/ph_grpc"
    proto_dir = python_out_dir + "/src/primihub/protos/"

    dir_files = os.listdir(proto_dir)
    file_to_remove = []
    for f_name in dir_files:
        if f_name.find("pb2.py") != -1:
            file_to_remove.append(f_name)
        if f_name.find("pb2_grpc.py") != -1:
            file_to_remove.append(f_name)

    for fname in file_to_remove:
        os.remove(proto_dir + fname)
        print("Remove {}.".format(fname))

    os.chdir("python")
    print(os.getcwd())


def compile_proto():
    print("compile proto...")
    os.chdir("../")
    print(os.getcwd())

    python_out_dir = "python/primihub/client/ph_grpc"
    grpc_python_out = "python/primihub/client/ph_grpc"

    cmd_str = """{pyexe} -m grpc_tools.protoc \
                        --python_out={python_out} \
                        --grpc_python_out={grpc_python_out} -I. \
                        src/primihub/protos/common.proto  \
                        src/primihub/protos/service.proto \
                        src/primihub/protos/worker.proto \
                        src/primihub/protos/metadatastream.proto \
                        src/primihub/protos/pir.proto \
                        src/primihub/protos/psi.proto""".format(python_out=python_out_dir,
                                                                grpc_python_out=grpc_python_out,
                                                                pyexe=sys.executable)

    print("cmd: %s" % cmd_str)
    cmd = shlex.split(cmd_str)
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
        compile_proto()
        develop.run(self)
        solib2sitepackage()


class PostInstallCommand(install):
    """Post-installation for installation mode."""

    def run(self):
        # PUT YOUR POST-INSTALL SCRIPT HERE or CALL A FUNCTION
        compile_proto()
        install.run(self)
        solib2sitepackage()
        clean_proto()


class ProtoCommand(distutils.cmd.Command):
    """compile proto"""

    # `python setup.py --help` description
    description = 'compile proto'
    user_options = []

    def initialize_options(self):
        ...

    def finalize_options(self):
        ...

    def run(self):
        print("======= command: compile proto is running =======")
        compile_proto()


class AddSoLibCommand(distutils.cmd.Command):
    """Run command:
        add so lib path to Python path.
    """

    # `python setup.py --help` description
    description = 'Add so lib path to Python path'
    user_options = [
        ('solib-path=', 's', 'solib path'),
    ]

    def initialize_options(self):
        self.solib_path = None

    def finalize_options(self):
        if self.solib_path is None:
            raise Exception("Parameter --solib-path is missing")
        if not os.path.isdir(self.solib_path):
            raise Exception(
                "Solib path does not exist: {0}".format(self.solib_path))

    def run(self):
        print("======= command:add solib is running =======")
        solib2sitepackage(self.solib_path)


setup(
    name='primihub',
    version=primihub.__version__,
    description=long_description,
    author=primihub.__author__,
    author_email=primihub.__contact__,
    url=primihub.__homepage__,
    packages=find_packages(),
    install_requires=install_requires,
    package_data = {'': ['*.json']},
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
        'proto': ProtoCommand,
        'solib': AddSoLibCommand,
    },
    entry_points={
        # 'console_scripts': [
        #     'edge=primihub.__main__:main'
        # ]
    },
    scripts=[
        'bin/submit',
    ],
)
