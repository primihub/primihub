#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """

import numpy as np
import pandas as pd
import primihub as ph
from primihub.FL.model.xgboost.xgb_guest import XGB_GUEST
from primihub.FL.model.xgboost.xgb_guest_en import XGB_GUEST_EN
from primihub.FL.model.xgboost.xgb_host import XGB_HOST
from primihub.FL.model.xgboost.xgb_host_en import XGB_HOST_EN
from primihub.channel.zmq_channel import IOService, Session
from primihub.primitive.opt_paillier_c2py_warpper import *

ph.dataset.define("guest_dataset")
ph.dataset.define("label_dataset")
ph.dataset.define("test_dataset")


@ph.function(role='host', protocol='xgboost', datasets=["label_dataset"], next_peer="*:5555")
def xgb_host_logic(cry_pri):
    print("start xgb host logic...")

    next_peer = ph.context.Context.nodes_context["host"].next_peer
    ip, port = next_peer.split(":")
    ios = IOService()
    server = Session(ios, ip, port, "server")
    channel = server.addChannel()

    data = ph.dataset.read(dataset_key="label_dataset").df_data
    data_test = ph.dataset.read(dataset_key="test_dataset").df_data
    labels = ['Class']  # noqa
    X_host = data[
        [x for x in data.columns if x not in labels]
    ]
    Y = data['Class'].values

    if cry_pri == "paillier":
        xgb_host = XGB_HOST_EN(n_estimators=1, max_depth=2, reg_lambda=1,
                               min_child_weight=1, objective='linear', channel=channel)
        y_hat = np.array([0.5] * Y.shape[0])
        for t in range(xgb_host.n_estimators):
            f_t = pd.Series([0] * Y.shape[0])
            gh = xgb_host.get_gh(y_hat, Y)
            print("recv guest: ", xgb_host.channel.recv())
            xgb_host.channel.send(xgb_host.pub)
            print(xgb_host.channel.recv())
            gh_en = pd.DataFrame(columns=['g', 'h'])
            for item in gh.columns:
                for index in gh.index:
                    gh_en.loc[index, item] = opt_paillier_encrypt_crt(xgb_host.pub, xgb_host.prv,
                                                                      int(gh.loc[index, item]))
            xgb_host.channel.send(gh_en)
            GH_guest_en = xgb_host.channel.recv()
            GH_guest = pd.DataFrame(
                columns=['G_left', 'G_right', 'H_left', 'H_right', 'var', 'cut'])
            for item in [x for x in GH_guest_en.columns if x not in ['cut', 'var']]:
                for index in GH_guest_en.index:
                    if GH_guest_en.loc[index, item] == 0:
                        GH_guest.loc[index, item] = 0
                    else:
                        GH_guest.loc[index, item] = opt_paillier_decrypt_crt(xgb_host.pub, xgb_host.prv,
                                                                             GH_guest_en.loc[index, item])
            for item in [x for x in GH_guest_en.columns if x not in ['G_left', 'G_right', 'H_left', 'H_right']]:
                for index in GH_guest_en.index:
                    GH_guest.loc[index, item] = GH_guest_en.loc[index, item]
            print(GH_guest)
            xgb_host.tree_structure[t + 1] = xgb_host.xgb_tree(X_host, GH_guest, gh, f_t, 0)  # noqa
            y_hat = y_hat + xgb_host.learning_rate * f_t

        output_path = ph.context.Context.get_output()
        return xgb_host.predict_prob(data_test).to_csv(output_path)
    elif cry_pri == "plaintext":
        xgb_host = XGB_HOST(n_estimators=1, max_depth=2, reg_lambda=1,
                            min_child_weight=1, objective='linear', channel=channel)
        y_hat = np.array([0.5] * Y.shape[0])
        for t in range(xgb_host.n_estimators):
            f_t = pd.Series([0] * Y.shape[0])
            gh = xgb_host.get_gh(y_hat, Y)

            print("recv guest: ", xgb_host.channel.recv())
            xgb_host.channel.send(gh)
            GH_guest = xgb_host.channel.recv()
            xgb_host.tree_structure[t + 1] = xgb_host.xgb_tree(X_host, GH_guest, gh, f_t, 0)  # noqa
            y_hat = y_hat + xgb_host.learning_rate * f_t

        output_path = ph.context.Context.get_output()
        return xgb_host.predict_prob(data_test).to_csv(output_path)


@ph.function(role='guest', protocol='xgboost', datasets=["guest_dataset"], next_peer="localhost:5555")
def xgb_guest_logic(cry_pri):
    print("start xgb guest logic...")

    ios = IOService()
    next_peer = ph.context.Context.nodes_context["guest"].next_peer
    ip, port = next_peer.split(":")
    client = Session(ios, ip, port, "client")
    channel = client.addChannel()
    channel.send(b'guest ready')
    X_guest = ph.dataset.read(dataset_key="guest_dataset").df_data
    # X_guest = data[['Clump Thickness', 'Uniformity of Cell Size']]

    if cry_pri == "paillier":
        xgb_guest = XGB_GUEST_EN(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear',
                                 channel=channel)  # noqa
        for t in range(xgb_guest.n_estimators):
            pub = xgb_guest.channel.recv()
            xgb_guest.channel.send(b'recved pub')
            gh_host = xgb_guest.channel.recv()
            X_guest_gh = pd.concat([X_guest, gh_host], axis=1)
            print(X_guest_gh)
            gh_sum = xgb_guest.get_GH(X_guest_gh, pub)
            xgb_guest.channel.send(gh_sum)
            xgb_guest.cart_tree(X_guest_gh, 0, pub)
    elif cry_pri == "plaintext":
        xgb_guest = XGB_GUEST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear',
                              channel=channel)  # noqa
        for t in range(xgb_guest.n_estimators):
            gh_host = xgb_guest.channel.recv()
            X_guest_gh = pd.concat([X_guest, gh_host], axis=1)
            gh_sum = xgb_guest.get_GH(X_guest_gh)
            xgb_guest.channel.send(gh_sum)
            xgb_guest.cart_tree(X_guest_gh, 0)
