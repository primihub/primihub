#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# pir_cli.py
# @Author :  ()
# @Link   : 
# @Date   : 6/24/2022, 4:15:29 PM

from cli import Cli


class PIRCli(Cli):
    """PIR client stragegy.

    Args:
        Cli (_type_): _description_
    """

    def __init__(self, node: str, cert: str) -> None:
        super(PIRCli, self).__init__(node, cert)

    def set_task_map(self, ):
        self.task_map = {
            "type": 2,
            "language": common_pb2.Language.PYTHON,
        }
