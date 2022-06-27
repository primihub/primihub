#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# visitor.py
# @Author :  ()
# @Link   : 
# @Date   : 6/26/2022, 12:13:31 AM


import ast
import sys
from os import path

from grpc_client import GRPCClient

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
        node = MyTransformer().visit(node)
        # fix locations
        node = ast.fix_missing_locations(node)
        print("Here are the extracted content:")
        code_str = ast.unparse(node)
        print(code_str)

        # print("- * -" * 20)
        # sys.exit(0)
        return code_str

    def visit_interactive(self):
        # TODO
        pass


# ast transformer
class MyTransformer(ast.NodeTransformer):

    def visit_ImportFrom(self, node):
        """for ImportFrom 
        - remove visitor
        
        Keyword arguments:
        argument -- description
        Return: return_description
        """
        # print("visiting: "+node.__class__.__name__)

        for name in [a.name for a in node.names]:
            if "Visitor" == name:
                print("removing `from visitor import Visitor`")
                # print(node.__dict__)
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
            return node


def upload_code():
    visitor = Visitor()
    code = visitor.visit_file()
    grpc_client = GRPCClient(node="localhost:50050", cert="")
    grpc_client.set_task_map(code=code.encode('utf-8'))
    res = grpc_client.submit()
    print("- * -")
    print("res: ",res)


# visitor = Visitor()
# code = visitor.visit_file()


upload_code()
