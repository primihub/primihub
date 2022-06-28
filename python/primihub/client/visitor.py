"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

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
        node = Transformer().visit(node)
        # fix locations
        node = ast.fix_missing_locations(node)
        print("Here are the extracted content:")
        code_str = ast.unparse(node)
        print(code_str)
        return code_str

    def visit_interactive(self):
        # TODO
        pass


# ast transformer
class Transformer(ast.NodeTransformer):

    def visit_ImportFrom(self, node):
        """for ImportFrom 
        - remove visitor
        
        Keyword arguments:
        argument -- description
        Return: return_description
        """
        for name in [a.name for a in node.names]:
            print("name: ", name)
            if "Visitor" == name:
                print("removing `from visitor import Visitor`")
                node = None
            if "PrimihubClient" == name:
                print("removing `from primihub.client import PrimihubClient`")
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
