# !/usr/bin/env python
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

import math
import pytest
import numpy as np
from primihub.primitive.opt_paillier_c2py_warpper import *
from phe import paillier
from os import path
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
from primihub.FL.model.logistic_regression.vlr.arbiter_phe import Arbiter
from primihub.FL.model.logistic_regression.vlr.guest_phe import Guest
from primihub.FL.model.logistic_regression.vlr.host_phe import Host
from  multiprocessing import Process

dir = path.join(path.dirname(__file__), '../../tests/data')
data_guest = np.loadtxt(path.join(dir, "wisconsin_guest.data"), str, delimiter=',')
data_host = np.loadtxt(path.join(dir, "wisconsin_host.data"), str, delimiter=',')
data_test=np.loadtxt(path.join(dir, "wisconsin_test.data"), str, delimiter=',')


def vfl_arbiter_logic():
    print("start vfl_arbiter logic...")
    proxy_server_guest = ServerChannelProxy("10094")  # arbiter接收guest消息
    proxy_server_host = ServerChannelProxy("10092")  # arbiter接收host消息
    proxy_client_guest = ClientChannelProxy(
        "127.0.0.1", "10090")  # arbiter发送消息给guest
    proxy_client_host = ClientChannelProxy(
        "127.0.0.1", "10091")  # arbiter发送消息给host
    config = {
        'epochs': 1,
        'batch_size': 200
    }
    proxy_server_host.StartRecvLoop()
    batch_num = proxy_server_host.Get("batch_num")
    client_arbiter = Arbiter(config['batch_size'])
    proxy_server_guest.StartRecvLoop()

    for i in range(config['epochs']):
        print("##### epoch %s ##### " % i)
        for j in range(batch_num):
            print("-----epoch=%s, batch=%s-----" % (i, j))
            client_arbiter.send_pub()
            client_arbiter.dec_gradient()
            print("batch=%s done" % j)
        print("epoch=%i done" % i)
    print("All process done.")

    client_arbiter.send_pub()
    client_arbiter.dec_re()
    proxy_server_guest.StopRecvLoop()
    proxy_server_host.StopRecvLoop()

def vfl_guest_logic():
    proxy_server_arbiter = ServerChannelProxy("10090")  # guest接收arbiter消息
    proxy_server_host = ServerChannelProxy("10093")  # guest接收Host消息
    proxy_client_arbiter = ClientChannelProxy("127.0.0.1", "10094")  # guest发送消息给arbiter
    proxy_client_host = ClientChannelProxy("127.0.0.1", "10095")  # guest发送消息给host
    config = {
        'epochs': 1,
        'lambda': 10,
        'threshold': 0.5,
        'lr': 0.05,
        'batch_size': 200
    }
    x = data_guest[1:, :]
    x = x.astype(np.float)
    count = x.shape[0]
    batch_num = count // config['batch_size'] + 1
    proxy_server_arbiter.StartRecvLoop()
    proxy_server_host.StartRecvLoop()
    client_guest = Guest(x, config)
    batch_gen_guest = client_guest.batch_generator(
        [x], config['batch_size'], False)
    for i in range(config['epochs']):
        print("##### epoch %s ##### " % i)
        for j in range(batch_num):
            print("-----epoch=%s, batch=%s-----" % (i, j))
            batch_guest_x = next(batch_gen_guest)[0]
            print("batch_guest_x.shape", batch_guest_x.shape)
            client_guest.cal_uz(batch_guest_x)
            client_guest.cal_dJ(batch_guest_x)
            client_guest.update(batch_guest_x)
            print("batch=%s done" % j)
        print("epoch=%i done" % i)
    print("guest training process done.")

    # load test data
    x_test = data_test[1:, 1:3]
    x_test = x_test.astype(float)
    print("x_test.shape", x_test.shape)
    print(x_test[0])
    client_guest.predict(client_guest.weights, x_test)
    proxy_server_host.StopRecvLoop()
    proxy_server_arbiter.StopRecvLoop()

def vfl_host_logic():
    proxy_server_arbiter = ServerChannelProxy("10091")  # host接收来自arbiter消息
    proxy_server_guest = ServerChannelProxy("10095")  # host接收来自guest消息
    proxy_client_arbiter = ClientChannelProxy("127.0.0.1", "10092")  # host发送消息给arbiter
    proxy_client_guest = ClientChannelProxy("127.0.0.1", "10093")  # host发送消息给guest
    config = {
        'epochs': 1,
        'lambda': 10,
        'threshold': 0.5,
        'lr': 0.05,
        'batch_size': 200
    }
    # load train data
    x = data_host[1:, 1:-1]
    x = x.astype(float)
    label = data_host[1:, -1]
    label = label.astype(int)
    for i in range(len(label)):
        if label[i] == 2:
            label[i] = 0
        else:
            label[i] = 1
    count = x.shape[0]
    batch_num = count // config['batch_size'] + 1
    proxy_client_arbiter.Remote(batch_num, "batch_num")

    proxy_server_arbiter.StartRecvLoop()
    proxy_server_guest.StartRecvLoop()
    client_host = Host(x, label, config)
    batch_gen_host = client_host.batch_generator(
        [x, label], config['batch_size'], False)
    for i in range(config['epochs']):
        print("##### epoch %s ##### " % i)
        for j in range(batch_num):
            print("-----epoch=%s, batch=%s-----" % (i, j))
            batch_host_x, batch_host_y = next(batch_gen_host)
            print("batch_host_x.shape", batch_host_x.shape)
            print("batch_host_y.shape", batch_host_y.shape)
            client_host.cal_u(batch_host_x, batch_host_y)
            client_host.cal_dJ_loss(batch_host_x, batch_host_y)
            client_host.update(batch_host_x)
            print("batch=%s done" % j)
        print("epoch=%i done" % i)
    print("host training process done.")
    client_host.data.update(
        proxy_server_guest.Get(
            "encrypted_z_guest_test", 10000))
    assert "encrypted_z_guest_test" in client_host.data.keys(
    ), "Error: 'encrypted_z_guest_test' from guest  not successfully received."
    guest_re = client_host.data["encrypted_z_guest_test"]
    # weights = np.concatenate(
    #     [client_host.data["guest_weights"], client_host.weights], axis=0)
    # print("weights.shape", weights.shape)
    # print(weights)
    # load test data
    x = data_test[1:, 3:-1]
    x = x.astype(float)
    print("x.shape", x.shape)
    print(x[0])
    label = data_test[1:, -1]
    label = label.astype(int)
    for i in range(len(label)):
        if label[i] == 2:
            label[i] = 0
        else:
            label[i] = 1
    client_host.predict(client_host.weights, x, label, guest_re)
    proxy_server_arbiter.StopRecvLoop()
    proxy_server_guest.StopRecvLoop()

def test_main():
    arbiter_process = Process(target=vfl_arbiter_logic)
    guest_process = Process(target=vfl_guest_logic)
    host_process = Process(target=vfl_host_logic)
    arbiter_process.start()
    guest_process.start()
    host_process.start()
    arbiter_process.join()
    guest_process.join()
    host_process.join()


if __name__ == "__main__":
    # test_main()
    pytest.main(['-q', path.dirname(__file__)])
