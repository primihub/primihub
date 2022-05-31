#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# test_disxgb.py
# @Author : Victory (lzw9560@163.com)
# @Link   : 
# @Date   : 5/27/2022, 10:37:57 AM

import sys
from os import path
import threading
import primihub as ph
import pandas as pd
import numpy as np

sys.path.append(path.abspath(path.join(path.dirname(__file__), "../")))


from examples.disxgb import xgb_host_logic, xgb_guest_logic


def run_xgb_host_logic():
    xgb_host_logic()

def run_xgb_guest_logic():
    xgb_guest_logic()

    
if __name__ == "__main__":
    print("- " * 30)
     
        
    # xgb_host_logic()
    # xgb_guest_logic()
    host = threading.Thread(target=run_xgb_host_logic)
    guest = threading.Thread(target=run_xgb_guest_logic)
    
    print("* " * 30, host)
    host.start()
    print("* " * 30, guest)
    guest.start()

    host.join()
    guest.join()
