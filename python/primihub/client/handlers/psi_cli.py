#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# psi_cli.py
# @Author :  ()
# @Link   : 
# @Date   : 6/24/2022, 4:21:11 PM


from cli import Cli


class PSICli(Cli):
    """PSI client stragegy.

    Args:
        Cli (_type_): _description_
    """

    def __init__(self, node: str, cert: str) -> None:
        super(PSICli, self).__init__(node, cert)
