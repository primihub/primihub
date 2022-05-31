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

import primihub_channel

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


def xgb_tree_fl(X, f_t, xgb, m_dpth, gh):
    if m_dpth > xgb.max_depth:
        return
    X = X.reset_index(drop='True')
    gh = gh.reset_index(drop='True')

    ios = primihub_channel.IOService(0)
    # TODO read address and port from context
    server = primihub_channel.Session(
                ios, "localhost:1213", primihub_channel.SessionMode.Server, "endpoint")
    s_channel = server.addChannel("TestChannel", "TestChannel")
    s_channel.send(gh)
    print("host send gh: ", gh)

    print("waitting recv ......")
    GH_guest = s_channel.recv()
    
    print("recv GH guest:", type(GH_guest), GH_guest)
    X_gh = pd.concat([X, gh], axis=1)
    gh_sum = xgb.get_GH(X_gh)
    var, cut, best = xgb.find_split(gh_sum, GH_guest)
    tree_structure = {(var, cut): {}}
    if var not in [x for x in X.columns]:
        # TODO send data
        ios = MockIOService()
        server = MockSession(ios, "localhost:1213", "server", "endpoint")
        s_channel = server.addChannel("TestChannel", "TestChannel")
        s_channel.send(var, cut, best)
        print("host send split parameter: ", f_t, var, cut, best)
        f_t, id_right, id_left, w_right, w_left = s_channel.recv()
        while True:
            if f_t is not None:
                break
            f_t, id_right, id_left, w_right, w_left = s_channel.recv()
            time.sleep(3)
            print("waitting recv, will sleep 3s ......")
        print("recv GH guest:", type(f_t, id_right, id_left, w_right, w_left), \
              f_t, id_right, id_left, w_right, w_left)
    else:
        f_t, id_right, id_left, w_right, w_left = xgb.split(X, var, cut, best, f_t)
    tree_structure[(var, cut)][('left', w_left)] = xgb_tree_fl(X.loc[id_left], \
                                                f_t, xgb, m_dpth+1, gh.loc[id_left])
    tree_structure[(var, cut)][('right', w_right)] = xgb_tree_fl(X.loc[id_right], \
                                                f_t, xgb, m_dpth + 1, gh.loc[id_right])
    return tree_structure



@ph.function(role='host', protocol='xgboost', datasets=["label_dataset"])
def xgb_host_logic():
    print("start xgb host logic...")

    data = ph.dataset.read(dataset_key="label_data", names=columnNames).df_data
    labels = ['Sample code number', 'Clump Thickness', 'Uniformity of Cell Size', 'Class']
    X_host = data[
        [x for x in data.columns if x not in labels]
    ]
    Y = data['Class'].values
    X_host = X_host.reset_index(drop='True')  # reset the index
    xgb_host = XGB_HOST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')
    y_hat = np.array([xgb_host.base_score] * Y.shape[0])
    for t in range(xgb_host.n_estimators):
        gh = xgb_host.get_gh(y_hat, Y)
        f_t = pd.Series([0] * Y.shape[0])
        xgb_host.tree_structure[t + 1] = xgb_tree_fl(X_host, f_t, xgb_host, 1, gh)
        y_hat = y_hat + xgb_host.learning_rate * f_t
    print("end xgb host logic...")
    print("HOST EXIT")


@ph.function(role='guest', protocol='xgboost', datasets=["guest_dataset"])
def xgb_guest_logic():
    print("start xgx guest logic...")
    data = ph.dataset.read(dataset_key="label_data", names=columnNames).df_data
    X_guest = data[['Clump Thickness', 'Uniformity of Cell Size']]
    X_guest = X_guest.reset_index(drop='True')  # reset the index
    xgb_guest = XGB_GUEST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')

    # TODO read address and port from context
    ios = primihub_channel.IOService(0)
    client = primihub_channel.Session(
        ios, "localhost:1213", primihub_channel.SessionMode.Client, "endpoint")
    c_channel = client.addChannel("TestChannel", "TestChannel")

    print("waitting recv ......")
    gh_host = c_channel.recv()
    print("guest rec host gh: ", type(gh_host), gh_host)

    X_guest_gh = pd.concat([X_guest, gh_host], axis=1)
    print("X_guest_gh columns: ", X_guest_gh.columns)
    gh_sum = xgb_guest.get_GH(X_guest_gh)

    c_channel.send(gh_sum)
    print("guest send gh sum: ", gh_sum)


    f_t_guest, var_guest, cut_guest, best_guest = c_channel.recv()

    while True:
        if var_guest is not None:
            break
        f_t_guest, var_guest, cut_guest, best_guest = c_channel.recv()
        time.sleep(3)
        print("waitting recv, will sleep 3s ......")

    print("guest rec host gh: ", type(f_t_guest, var_guest, cut_guest, best_guest), \
          f_t_guest, var_guest, cut_guest, best_guest)
    f_t, id_right, id_left, w_right, w_left = xgb_guest.split(X_guest, var_guest, cut_guest, best_guest, f_t_guest)
    c_channel.send(f_t, id_right, id_left, w_right, w_left)
    print("end xgb guest logic...")
    print("GUEST EXIT")

