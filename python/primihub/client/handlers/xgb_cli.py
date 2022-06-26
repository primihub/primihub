#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# xgb_cli.py
# @Author :  ()
# @Link   : 
# @Date   : 6/23/2022, 2:20:26 PM

from cli import Cli
from ph_grpc import common_pb2



class XGBCli(Cli):
    """XGB client stragegy.

    Args:
        Cli (_type_): _description_
    """


    def __init__(self, node: str, cert: str) -> None:
        super(XGBCli, self).__init__(node=node, cert=cert)
    
    
    def set_task_map(self, code_path: str)-> dict():
        """set tack_map

        Args:
            code_path (str): code file path.

        Returns:
            _type_: _description_
        """
        f = open(code_path, "rb")
        self.task_map = {
            "type": 0,
            "language": common_pb2.Language.PYTHON,
            "code": f.read()
        }
        return self.task_map

    def set_params(self, *args, **kwargs):
        self.params = dict(**kwargs)
        return self.params
        
    def submit(self):
        return super().submit()

    def execute(self):
        return super().execute()