#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# cli.py
# @Author :  ()
# @Link   :
# @Date   : 6/22/2022, 5:46:28 PM

import grpc


class Cli(object):
    """Cli stragegy.

    Args:
        object (_type_): _description_
    """

    def __init__(self, node: str, cert: str) -> None:
        """Constructor

        Keyword arguments:
        argument --  str: The grpc server address. cert: local cert path. 
        Return: return_description
        """
        if node is not None:
            self.channel = grpc.insecure_channel(node)
            
        if cert is not None:
            # TODO
            pass


    # @property
    # def task_map(self):
    #     """The task map property - the getter.
    #     """
    #     return self._task_map

    # @property.setter
    # def task_map(self, value: dict):
    #     """The setter of the task_map

    #     Args:
    #         value (dict): The task map.
    #     """
    #     # code = f = open(r"disxgb.py", "rb")
    #     # task_map = {
    #     #     "type": 0,
    #     #     "language": common_pb2.Language.PYTHON,
    #     #     "code": f.read()
    #     # }

    #     self._task_map = value

    def set_task_map(self):
        raise NotImplemented()
        
    def submit(self):
        raise NotImplemented()

    def execute(self):
        raise NotImplemented()
