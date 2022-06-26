#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# mpc_client.py
# @Author :  ()
# @Link   : 
# @Date   : 6/24/2022, 5:10:10 PM

from cli import Cli
from ph_grpc import common_pb2

class MPCCli(Cli):
    """mpc client stragety.

    Args:
        Cli (_type_): _description_
    """

    def __init__(self, node: str, cert: str) -> None:
        super(MPCCli, self).__init__(node, cert)

    