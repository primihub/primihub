#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# disxgb.py
# @Author : Victory (lzw9560@163.com)
# @Link   :
# @Date   : 5/27/2022, 10:37:57 AM

import sys
from os import path

import pandas as pd
import numpy as np

import primihub as ph
import time

from primihub.FL.model.xgboost.xgb_host import XGB_HOST
from primihub.FL.model.xgboost.xgb_guest import XGB_GUEST

sys.path.append(path.abspath(path.join(path.dirname(__file__), "../")))

from channel.mock_channel import MockIOService, MockSession


DATA_PATH = path.abspath(path.join(path.dirname(__file__), "../tests/data/wisconsin.data"))
# DATA_PATH = "../tests/data/wisconsin.data"
columnNames = (
    'Sample code number',
    'Clump Thickness',
    'Uniformity of Cell Size',
    'Uniformity of Cell Shape',
    'Marginal Adhesion',
    'Single Epithelial Cell Size',
    'Bare Nuclei',
    'Bland Chromatin',
    'Normal Nucleoli',
    'Mitoses',
    'Class'
)


print(DATA_PATH)
print(columnNames)

ph.dataset.define("guest_dataset")
ph.dataset.define("label_dataset")
ph.context.Context.dataset_map["label_data"] = DATA_PATH


@ph.function(role='host', protocol='xgboost', datasets=["label_dataset"])
def xgb_host_logic():
    print("start xgb host logic...")
    
    data = ph.dataset.read(dataset_key="label_data", names=columnNames).df_data
    labels = ['Sample code number', 'Clump Thickness', 'Uniformity of Cell Size', 'Class']
    X_host = data[
            [x for x in data.columns if x not in labels]
        ]
    Y = data['Class'].values
    X_host = X_host.reset_index(drop='True')    # reset the index
    xgb_host = XGB_HOST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')
    y_hat_t = np.array([0.5] * Y.shape[0])
    gh = xgb_host.get_gh(y_hat_t, Y)

    # TODO send data
    ios = MockIOService()
    server = MockSession(ios, "localhost:1213", "server", "endpoint")
    s_channel = server.addChannel("TestChannel", "TestChannel")
    s_channel.send(gh)

    print("host send gh: ", gh)
    GH_guest = s_channel.recv()
    while True:
        if GH_guest is not None:
            break 
        GH_guest = s_channel.recv()
        time.sleep(3)
        print("waitting recv, will sleep 3s ......")
    print("recv GH guest:", type(GH_guest), GH_guest)

    X_host_gh = pd.concat([X_host, gh], axis=1)
    gh_sum = xgb_host.get_GH(X_host_gh)
    var, cut, best = xgb_host.find_split(gh_sum, GH_guest)
    print("find split var, cut, best: ", var, cut, best)
    print("end xgb host logic...")
    print("HOST EXIT")


@ph.function(role='guest', protocol='xgboost', datasets=["guest_dataset"])
def xgb_guest_logic():
    print("start xgx guest logic...")
    data = ph.dataset.read(dataset_key="label_data", names=columnNames).df_data
    X_guest = data[['Clump Thickness', 'Uniformity of Cell Size']]
    X_guest = X_guest.reset_index(drop='True')  # reset the index
    xgb_guest = XGB_GUEST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')

    # TODO send data instead mock
    ios = MockIOService()
    server = MockSession(ios, "localhost:1213", "server", "endpoint")
    c_channel = server.addChannel("TestChannel", "TestChannel")

    gh_host = c_channel.recv()
    
    while True:
        if gh_host is not None:
            break 
        gh_host = c_channel.recv()
        time.sleep(3)
        print("waitting recv, will sleep 3s ......")

    print("guest rec host gh: ", type(gh_host), gh_host)

    X_guest_gh = pd.concat([X_guest, gh_host], axis=1)
    print("X_guest_gh columns: ", X_guest_gh.columns)
    gh_sum = xgb_guest.get_GH(X_guest_gh)

    c_channel.send(gh_sum)
    print("guest send gh sum: ", gh_sum)
    print("end xgb guest logic...")
    print("GUEST EXIT")





# =============================================================================
# print (ph.context.Context.nodes_context)

