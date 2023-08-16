"""
Copyright 2022 PrimiHub

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https: //www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""


import ast
import sys
from os import path


sys.path.append(path.abspath(path.join(path.dirname(__file__), "..")))


class Visitor(object):

    def __init__(self) -> None:
        pass

    def visit_file(self):
        cur_path = sys.argv[0]
        print("current file path is: %s" % cur_path)
        with open(cur_path) as f:
            code = f.read()
        node = ast.parse(code)
        # do ast manipulation
        node = CLiTransformer().visit(node)
        # fix locations
        node = ast.fix_missing_locations(node)
        print("Here are the extracted content:")
        code_str = ast.unparse(node)
        print(code_str)
        return code_str

    def visit_interactive(self):
        # TODO
        pass

    def trans_remote_execute(self, code):
        node = ast.parse(code)
        # print(ast.dump(node))
        # do ast manipulation
        node = RemoteExecuteTransformer().visit(node)
        # fix locations
        node = ast.fix_missing_locations(node)
        # print("Here are the extracted content:")
        code_str = ast.unparse(node)
        # print(code_str)
        return code_str

# ast transformer


class CLiTransformer(ast.NodeTransformer):

    def generic_visit(self, node):
        for field, old_value in ast.iter_fields(node):
            if isinstance(old_value, list):
                new_values = []
                for value in old_value:
                    if isinstance(value, ast.AST):
                        value = self.visit(value)
                        if value is None:
                            continue
                        elif not isinstance(value, ast.AST):
                            new_values.extend(value)
                            continue
                    new_values.append(value)
                old_value[:] = new_values
            elif isinstance(old_value, ast.AST):
                new_node = self.visit(old_value)
                if new_node is None:
                    delattr(node, field)
                else:
                    setattr(node, field, new_node)
        return node

    def visit_ImportFrom(self, node):
        """for ImportFrom
        - remove visitor

        Keyword arguments:
        argument -- description
        Return: return_description
        """
        for name in [a.name for a in node.names]:
            print("name: ", name)
            if "primihub_cli" == name:
                print(
                    "removing `from primihub.client.client import primihub_cli as cli`")
                node = None
            if "PrimiHubClient" == name:
                print("removing `from primihub.client import PrimiHubClient`")
                node = None
            return node

    def visit_Import(self, node):
        """for Import
        - remove visitor

        Keyword arguments:
        argument -- description
        Return: return_description
        """

        for name in [a.name for a in node.names]:
            if "visitor" == name:
                print("removing `import visitor`")
                node = None
            if "primihub.client" == name:
                print("removing `import primihub_client`")
                node = None
            return node

    def visit_Expr(self, node: ast.Expr):
        if isinstance(node.value, ast.Call):
            # print(node.value.func)
            # print(node.value.func.__dict__)
            if node.value.func.__dict__.get("attr", None) == "init":
                return None
        return node


class RemoteExecuteTransformer(ast.NodeTransformer):

    def generic_visit(self, node):
        for field, old_value in ast.iter_fields(node):
            if isinstance(old_value, list):
                new_values = []
                for value in old_value:
                    if isinstance(value, ast.AST):
                        value = self.visit(value)
                        if value is None:
                            continue
                        elif not isinstance(value, ast.AST):
                            new_values.extend(value)
                            continue
                    new_values.append(value)
                old_value[:] = new_values
            elif isinstance(old_value, ast.AST):
                new_node = self.visit(old_value)
                if new_node is None:
                    delattr(node, field)
                else:
                    setattr(node, field, new_node)
        return node

    def visit_Expr(self, node: ast.Expr):
        if isinstance(node.value, ast.Call):
            # print(node.value.func)
            # print(node.value.func.__dict__)
            if node.value.func.__dict__.get("attr", None) == "async_remote_execute":
                return None
            if node.value.func.__dict__.get("attr", None) == "remote_execute":
                return None
            if node.value.func.__dict__.get("attr", None) == "start":
                return None
        return node
