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

from pyparsing import Any

sys.path.append(path.abspath(path.join(path.dirname(__file__), "..")))


class Visitor(object):

    def __init__(self) -> None:
        pass

    def visit_file(self):
        cur_path = sys.argv[0]
        print(cur_path)
        print(__file__)
        print("* " * 30)
        with open(cur_path) as f:
            code = f.read()

        node = ast.parse(code)

        # do ast manipulation
        node = MyTransformer().visit(node)
        # fix locations
        node = ast.fix_missing_locations(node)
        # print("- * -" * 20)
        print("Here are the extracted content:")
        code_str = ast.unparse(node)
        print(code_str)
        # print("- * -" * 20)
        sys.exit(0)


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


visitor = Visitor()
visitor.visit_file()
